#include <iostream>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

#include <signal.h>
#include <unistd.h>
#include <error.h>
#include <netdb.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <vector>

#include "src/service2/thread-pool.h"
#include "src/util/log-message.h"
#include "src/service2/socket-class.h"
#include "src/v2-asr/v2-asr-work-thread.h"
#include "src/v2-asr/v2-asr-task.h"
#include "src/service2/pthread-util.h"

using namespace std;

int main(int argc, char *argv[])
{
	#ifdef NAMESPACE
	using namespace datemoon;
	#endif
	// Block SIGPIPE before starting any other threads; other threads
	// created by main() will inherit a copy of the signal mask.
	ThreadSigPipeIng();
	const char *usage = "This is a v2 asr service.\n"
		"Usage: v2-asr-service [options] <nnet3-in> "
		"<fst-in> <word-symbol-table>\n";

	kaldi::ParseOptions po(usage);
	std::string config_socket="";
	int nthread = 1;
	po.Register("config-socket", &config_socket, "Socket config file.(default NULL)");
	po.Register("nthread", &nthread, "service thread number.(default 1)");
	// init online featuer and nnet config
	OnlineDecoderConf online_conf;
	online_conf.Register(&po);

	po.Read(argc, argv);
	if (po.NumArgs() != 3)
   	{
		po.PrintUsage();
		return 1;
	}

	std::string nnet3_rxfilename = po.GetArg(1),
		fst_in_filename = po.GetArg(2),
		word_syms_filename = po.GetArg(3);

	// online decoder info
	OnlineDecoderInfo online_info(online_conf, nnet3_rxfilename,
			fst_in_filename, word_syms_filename);

	LOG_COM << "OnlineDecoderInfo init ok.";
	//signal(SIGPIPE, SIG_IGN); // ignore SIGPIPE to avoid crashing when socket forcefully disconnected

	//ConfigParseOptions conf(usage);
	SocketConf net_conf;
	datemoon::ReadConfigFromFile(config_socket, &net_conf);
	net_conf.Info();
	SocketBase net_io(&net_conf);
	//net_io.Register(&conf);
	//conf.Read(argc, argv);
	//conf.PrintUsage();

	LOG_COM << "SocketBase init ok.";
	if(net_io.Init() < 0)
	{
		LOG_ERR << "net_io.Init failed!!!";
		return -1;
	}

	if (0 != net_io.Bind())
	{
		LOG_ERR << "net_io.Bind failed !!!";
		return -1;
	}

	if ( 0 != net_io.Listen())
	{
		LOG_ERR << "net_io.Listen failed!!!";
		return -1;
	}

	LOG_COM << "create ThreadPool start.";
	ThreadPoolBase<ThreadBase> pool(nthread);
	{
		// create thread
		vector<ThreadBase*> tmp_threads;
		for(int i =0;i<nthread;++i)
		{
			V2ASRWorkThread *asr_t = new V2ASRWorkThread(&pool, online_info, i);
			tmp_threads.push_back(asr_t);
		}
		pool.Init(tmp_threads);
	}
	LOG_COM << "init thread pool ok";

	while(1)
	{
		int connectfd = net_io.Accept();
		if(connectfd < 0)
		{
			if(connectfd == -2)
			{
				std::cout << ".";
				fflush(stdout);
				pool.Info();
				//printf("no cli connect requst %d %d %d %d.\n", connectfd,errno,EINPROGRESS,EAGAIN);
			}
			else
				LOG_WARN << "cli connect error!!!";

		}
		else
		{
			VLOG_COM(2) << "connect " << connectfd;
			V2ASRServiceTask *ta = new V2ASRServiceTask(connectfd);
			pool.AddTask(ta);
		}
	}

	return 0;
}

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
#include "src/service2/socket-class.h"
#include "src/service2/pthread-util.h"
#include "src/util/log-message.h"
#include "src/v1-asr/kaldi-v1-asr-online.h" // 解码器
#include "src/v1-asr/v1-asr-work-thread.h"  // 工作线程
#include "src/v1-asr/v1-asr-task.h"         // 任务线程

using namespace std;

int main(int argc, char *argv[])
{
	#ifdef NAMESPACE
	using namespace datemoon;
	#endif
	using namespace kaldi;
	using namespace fst;

	// Block SIGPIPE before starting any other threads; other threads
	// created by main() will inherit a copy of the signal mask.
	ThreadSigPipeIng();

	const char *usage = "This is kaldi asr service.\n"
		"Usage: v1-asr-service [options] <nnet3-in> "
		"<fst> <word-symbol-table>\n";
	ParseOptions po(usage);
	V1AsrOpts v1_asr_opts;

	v1_asr_opts.Register(&po);

	std::string config_socket="";
	int nthread = 1;
	po.Register("config-socket", &config_socket, "Socket config file.(default NULL)");
	po.Register("nthread", &nthread, "service thread number.(default 1)");

	po.Read(argc, argv);
	if (po.NumArgs() != 3)
   	{
		po.PrintUsage();
		return 1;
	}

	std::string nnet3_rxfilename = po.GetArg(1),
		fst_rxfilename = po.GetArg(2),
		word_syms_filename = po.GetArg(3);

	V1AsrSource v1_asr_source(v1_asr_opts,
			nnet3_rxfilename, fst_rxfilename, word_syms_filename);

	//ConfigParseOptions conf(usage);
	SocketConf net_conf;
	datemoon::ReadConfigFromFile(config_socket, &net_conf);
	net_conf.Info();
	SocketBase net_io(&net_conf);

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

	ThreadPoolBase<ThreadBase> pool(nthread);
	{
		// create thread
		vector<ThreadBase*> tmp_threads;
		for(int i =0;i<nthread;++i)
		{
			V1ASRWorkThread *asr_t = new V1ASRWorkThread(&pool, 
					v1_asr_opts, v1_asr_source);
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
				printf(".");
				fflush(stdout);
				//printf("no cli connect requst %d %d %d %d.\n", connectfd,errno,EINPROGRESS,EAGAIN);
				pool.Info();
			}
			else
				KALDI_WARN << "cli connect error!!!";

		}
		else
		{
			KALDI_VLOG(5) << "connect " << connectfd;
			V1ASRServiceTask *ta = new V1ASRServiceTask(connectfd);
			pool.AddTask(ta);
		}
	}

	return 0;
}

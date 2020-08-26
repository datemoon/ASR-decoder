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

using namespace std;

int main(int argc, char *argv[])
{
	#ifdef NAMESPACE
	using namespace datemoon;
	#endif
	const char *usage = "This is a test v2 asr service test code.\n"
		"Usage: v2-asr-service [options] <nnet3-in> "
		"<fst-in> <hmm-fst-in> <word-symbol-table> <wavfile>\n";

	kaldi::ParseOptions po(usage);
	std::string config_socket="";
	po.Register("config-socket", &config_socket, "Socket config file.(default NULL)");
	// init online featuer and nnet config
	OnlineDecoderConf online_conf;
	online_conf.Register(&po);

	po.Read(argc, argv);
	if (po.NumArgs() != 5)
   	{
		po.PrintUsage();
		return 1;
	}

	std::string nnet3_rxfilename = po.GetArg(1),
		fst_in_filename = po.GetArg(2),
		hmm_in_filename = po.GetArg(3),
		word_syms_filename = po.GetArg(4),
		wavfilelist = po.GetArg(5);

	// online decoder info
	OnlineDecoderInfo online_info(online_conf, nnet3_rxfilename,
			fst_in_filename, hmm_in_filename,word_syms_filename);

	signal(SIGPIPE, SIG_IGN); // ignore SIGPIPE to avoid crashing when socket forcefully disconnected

	//ConfigParseOptions conf(usage);
	SocketBase net_io;
	ReadConfigFromFile(config_socket, &net_io);
	//net_io.Register(&conf);
	//conf.Read(argc, argv);
	//conf.PrintUsage();

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

	int nthread = 3;
	ThreadPoolBase<ThreadBase> pool(nthread);
	{
		// create thread
		vector<ThreadBase*> tmp_threads;
		for(int i =0;i<nthread;++i)
		{
			V2ASRWorkThread *asr_t = new V2ASRWorkThread(online_info);
			if (asr_t->Create() != 0)
			{
				printf("init thread failed.\n");
				return -1;
			}
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
			}
			else
				printf("cli connect error.\n");

		}
		else
		{
			printf("connect %d.\n",connectfd);
			V2ASRServiceTask *ta = new V2ASRServiceTask(connectfd);
			pool.AddTask(ta);
		}
	}

	return 0;
}

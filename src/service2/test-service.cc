#include <iostream>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/tcp.h>


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
#include "src/service2/test-work-thread.h"
#include "src/service2/test-task.h"
#include "src/util/log-message.h"
#include "src/service2/socket-class.h"
#include "src/service2/pthread-util.h"

using namespace std;

#ifdef NAMESPACE
using namespace datemoon;
#endif
int main(int argc, char *argv[])
{
	ThreadSigPipeIng();
	const char *usage = "This is a test service code.\n";
	ConfigParseOptions conf(usage);
	SocketConf net_conf;
	net_conf.Register(&conf);
	conf.Read(argc, argv);
	//conf.PrintUsage();
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

	int nthread = 3;
	ThreadPoolBase<ThreadBase> pool(nthread);
	{
		// create thread
		vector<ThreadBase*> tmp_threads;
		for(int i =0;i<nthread;++i)
		{
			TestWorkThread *asr_t = new TestWorkThread(&pool);
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
			TestServiceTask *ta = new TestServiceTask(connectfd);
			pool.AddTask(ta);
		}
	}

	return 0;
}

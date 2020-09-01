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
#include "src/service2/pthread-util.h"

using namespace std;
#ifdef NAMESPACE
using namespace datemoon;
#endif

int main(int argc, char *argv[])
{
	ThreadSigPipeIng();

	int sockfd = socket(AF_INET, SOCK_STREAM, 0 );
	if(sockfd == -1)
	{
		LOG_COM << "socket error.";
		return -1;
	}
	
	struct sockaddr_in ser,cli;
	memset(&ser, 0, sizeof(ser));
	ser.sin_family = AF_INET;
	inet_aton("127.0.0.1",&ser.sin_addr);
	ser.sin_port = htons(8000);

	int keepalive = 1; // open keepalive
	int keeptime = 5; // 5s no data
	int keepinterval = 1;
	int keepcount = 3;

	setsockopt(sockfd,SOL_SOCKET, SO_KEEPALIVE, (void*)&keepalive,sizeof(keepalive));
	setsockopt(sockfd, SOL_TCP, TCP_KEEPIDLE, (void*)&keeptime, sizeof(keeptime));
	setsockopt(sockfd, SOL_TCP, TCP_KEEPINTVL, (void*)&keepinterval, sizeof(keepinterval));
	setsockopt(sockfd, SOL_TCP, TCP_KEEPCNT, (void*)&keepcount, sizeof(keepcount));

	struct timeval rec_timeout;
	rec_timeout.tv_sec = 5;
	rec_timeout.tv_usec =0;

	if (-1 ==setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (void*)&rec_timeout, sizeof(rec_timeout)))
	{
		printf("Cannot set socket option SO_RCVTIMEO!");
		return -1;
	}

	int flag = 1;
	int len = sizeof(int);
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, len) == -1)
	{
		printf("Cannot set socket option SO_REUSEADDR!");
		return -1;
	}

	int res=bind(sockfd, (struct sockaddr *) & ser, sizeof(ser));
	if(res != 0)
	{
		LOG_COM << "bind error.";
		return -1;
	}
	int nthread = 1;
	listen(sockfd, 5);

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
	printf("init thread pool ok\n");

	while(1)
	{
		socklen_t len = sizeof(cli);
		int connectfd = accept(sockfd, (struct sockaddr *)&cli, &len);
		if(connectfd < 0)
		{
			if(errno == EAGAIN)
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

	close(sockfd);
	return 0;
}

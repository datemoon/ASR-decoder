#ifndef __TASK_H__
#define __TASK_H__

#include <sys/types.h>
#include <sys/socket.h>
#include "src/service/thread-pool.h"
#include "src/util/log-message.h"

class ASRServiceTask:public TaskBase
{
public:
	typedef TaskBase::int32 int32;
public:
	ASRServiceTask(int32 connfd, std::string task_name = ""):
		TaskBase(task_name), _connfd(connfd) {}

	int32 Run(void *data)
	{
		int n=0; // read times
		while(1)
		{
			char recvbuf[1024];
			char sendbuf[1024];
			memset(recvbuf,0x00,sizeof(recvbuf));
			memset(sendbuf,0x00, sizeof(sendbuf));

			int len = recv(_connfd, recvbuf, sizeof(recvbuf), 0 );
			if(len <= 0)
			{
				LOG_COM << "on buf " << len << "!!!";
				if(errno == EAGAIN || errno == EINPROGRESS)
				{
					LOG_COM << "|" << _connfd << "| timeout and continue";
					n++;
					if(n>2)
						break;
					continue;
				}
				break;
			}

			n=0;
			VLOG_COM(0) << "from |" << _connfd << "| receive \"" << recvbuf << "\"";
			if(strncmp(recvbuf,"end",3) == 0)
			{
				break;
			}
			sprintf(sendbuf, "from |%d| receive \"%s\" ", _connfd, recvbuf);
			send(_connfd, sendbuf, sizeof(sendbuf), 0);
			printf("send |%d| : %s ok\n",_connfd, sendbuf);
		}
		close(_connfd);
		printf("close |%d| ok.\n",_connfd);
		return 0;
	}

	int32 Stop()
	{
		char *str = "no idle thread.";
		send(_connfd, str, strlen(str), 0);
		printf("send |%d| : %s ok\n",_connfd, str);
		return 0;
	}
	void GetInfo()
	{
		LOG_COM << "Task name is : " << _task_name ;
		LOG_COM << "Task connetid is : " << _connfd;
	}

private:
	int32 _connfd; // connect fd
	void SetConnFd(int32 connfd); // set socket id
};

#endif

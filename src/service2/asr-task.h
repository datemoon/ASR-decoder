#ifndef __ASR_TASK_H__
#define __ASR_TASK_H__

#include <sys/types.h>
#include <sys/socket.h>
//#include "src/service2/thread-pool.h"
#include "src/service2/asr-work-thread.h"
#include "src/util/log-message.h"

#include "src/util/namespace-start.h"

class ASRServiceTask:public TaskBase
{
public:
	typedef TaskBase::int32 int32;
public:
	ASRServiceTask(int32 connfd, std::string task_name = ""):
		TaskBase(task_name), _connfd(connfd) {}

	int32 Run(void *data);

	int32 Stop()
	{
		const char *str = "no idle thread.";
		send(_connfd, str, strlen(str), 0);
		printf("send |%d| : %s ok\n",_connfd, str);
		close(_connfd);
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

int32 ASRServiceTask::Run(void *data)
{
	ASRWorkThread * asr_work_thread = static_cast<ASRWorkThread *>(data);
	C2SPackageAnalysis &ser_c2s = asr_work_thread->_ser_c2s_package_analysys;
	S2CPackageAnalysis &ser_s2c = asr_work_thread->_ser_s2c_package_analysys;
	kaldi::ASRWorker *asr_work = asr_work_thread->_asr_work;
	int n=0; // read timeout times
	while(1)
	{

		if(true != ser_c2s.C2SRead(_connfd))
		{
			// read data
			if(errno == EAGAIN || errno == EINPROGRESS)
			{
				LOG_COM << "|" << _connfd << "| timeout and continue";
				n++;
				if(n>2)
				{
					LOG_COM << "|" << _connfd << "| timeout and disconnet";
					break;
				}
				continue;
			}
			LOG_COM << "C2SRead error." << std::endl;
			break;
		}
		n=0;
		uint data_len;
		char *data = ser_c2s.GetData(&data_len);
		VLOG_COM(0) << "from |" << _connfd << "| receive \"" << data_len << "\"";
		// 8k ,16bit
		bool eos=false;
		if(ser_c2s.IsEnd())
		{
			eos = true;
		}
		int32 ret = asr_work->ProcessData(data, data_len, eos);

		if(eos == true || ret == 1)
		{
			if(eos == true)
			{
				if(true != ser_s2c.S2CWrite(_connfd, S2CPackageAnalysis::S2CEND))
				{
					LOG_COM << "S2CWrite all end error.";
					break;
				}
			}
			else
			{
				if(true != ser_s2c.S2CWrite(_connfd, S2CPackageAnalysis::S2CMIDDLEEND))
				{
					LOG_COM << "S2CWrite middle end error.";
					break;
				}
			}
			asr_work->Reset(eos);
		}
		else
		{
			if(true != ser_s2c.S2CWrite(_connfd, S2CPackageAnalysis::S2CNOEND))
			{
				LOG_COM << "S2CWrite error.";
				break;
			}
		}
		ser_s2c.Reset();
		if(eos == true)
			break; // end
	}
	close(_connfd);
	printf("close |%d| ok.\n",_connfd);
	return 0;
}

#include "src/util/namespace-end.h"

#endif

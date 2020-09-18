#ifndef __ASR_TASK_H__
#define __ASR_TASK_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
//#include "src/service2/thread-pool.h"
#include "src/v1-asr/asr-work-thread.h"
#include "src/util/util-common.h"

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
	//signal(SIGPIPE, SIG_IGN); // ignore SIGPIPE to avoid crashing when socket forcefully disconnected
	ASRWorkThread * asr_work_thread = static_cast<ASRWorkThread *>(data);
	C2SPackageAnalysis &ser_c2s = asr_work_thread->_ser_c2s_package_analysys;
	S2CPackageAnalysis &ser_s2c = asr_work_thread->_ser_s2c_package_analysys;
	kaldi::ASRWorker *asr_work = asr_work_thread->_asr_work;
	size_t chunk=0;
	ser_c2s.Reset(true);
	ser_s2c.Reset();
	asr_work->Init(&chunk);
	int n=0; // read timeout times
	// calculate time
	Time keep_time;
	int total_wav_len = 0;
	double total_decoder_time = 0.0;
	uint dtype_len=0;
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
		ser_c2s.Print("ser_c2s");
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
		dtype_len = ser_c2s.GetDtypeLen();
		std::string msg;
		// time statistics
		total_wav_len += data_len;
		keep_time.Esapsed();
		
		int32 ret = asr_work->ProcessData(data, data_len, msg, eos, 2);
		
		total_decoder_time += keep_time.Esapsed();

		ser_s2c.SetNbest(msg);
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
		ser_s2c.Print("ser_s2c");
		ser_s2c.Reset();
		if(eos == true)
		{
			uint smaple_rate = 0;
			if(ser_c2s.GetSampleRate() == C2SPackageAnalysis::K_16)
				smaple_rate=16000;
			else if(ser_c2s.GetSampleRate() == C2SPackageAnalysis::K_8)
				smaple_rate=8000;
			else if(ser_c2s.GetSampleRate() == C2SPackageAnalysis::K_32)
				smaple_rate=32000;
			else
				break;
			double wav_time = total_wav_len*1.0/(smaple_rate*dtype_len);
			LOG_COM << "wav time(s)\t:" << wav_time;
			LOG_COM << "run time(s)\t:" << total_decoder_time;
			LOG_COM << "decoder rt\t: " << total_decoder_time/wav_time;
			// send time to work thread.
			asr_work_thread->SetTime(wav_time, total_decoder_time);
			break; // end
		}
	} // while(1) end one audio
	asr_work->Destory();
	close(_connfd);
	printf("close |%d| ok.\n",_connfd);
	return 0;
}

#include "src/util/namespace-end.h"

#endif

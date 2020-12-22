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
	
	EnergyVad<short> &energy_vad = asr_work_thread->_energy_vad;
    energy_vad.Reset();
	bool use_energy_vad = asr_work->GetOpt()->_use_energy_vad;

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
	std::vector<std::string> asr_result;
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
		total_wav_len += data_len;
		VLOG_COM(1) << "from |" << _connfd << "| receive \"" << data_len << "\"";
		// 8k ,16bit
		bool eos= ser_c2s.IsEnd();
		if(eos == false && data_len == 0)
		{
			LOG_WARN << "Recv data length is Zero and eos is false!!!"
				<< " It's not allow and eos will be change true.";
			eos = true;
		}

		// get audio data type len
		dtype_len = ser_c2s.GetDtypeLen();
		// get result
		uint nbest = ser_c2s.GetNbest();

		std::string msg;
		keep_time.Esapsed();
		if(use_energy_vad == true)
		{
			std::vector<short> tmp_data(reinterpret_cast<short*>(data), reinterpret_cast<short*>(data)+data_len/2);
			energy_vad.JudgeFramesRes(tmp_data, eos);
			int32 ret = 0 ;
			std::vector<std::string> nbest_result;
			while(1)
			{
				nbest_result.clear();
				int getdata_audio_flag = -1;
				std::vector<short> out_data;
				int vad_ret = energy_vad.GetData(out_data, getdata_audio_flag);
				if(out_data.size() > 0)
				{
					bool vad_end = false;
					if(getdata_audio_flag == 0) // sil
					{// 切语音了，是一整段语音，需要获取结果
						vad_end = true;
						ret = asr_work->ProcessData((char*)out_data.data(), (int)out_data.size()*2, msg, vad_end, 2);
						asr_work->Init(&chunk);
					}
					else if(getdata_audio_flag == 1) // audio
					{
						if(vad_ret == 0 && eos == true) // energy_vad.GetData 只>有在处理每次音频最后的时候才可能是语音段，所以vad_ret一定等于0
						{
							vad_end = true;
							ret = asr_work->ProcessData((char*)out_data.data(), (int)out_data.size()*2, msg, vad_end, 2);
						}
						else
						{
							vad_end = false;
							ret = asr_work->ProcessData((char*)out_data.data(), (int)out_data.size()*2, msg, vad_end, 2);
						}
					}

					if(vad_end == true || eos == true)
					{
						if(msg.size() > 0)
							asr_result.push_back(msg);
						msg.clear();
					}
				}
				if(vad_ret == 0)
					break;
			}
		} // (use_energy_vad == true)
		else
		{
			// time statistics
			int32 ret = asr_work->ProcessData(data, data_len, msg, eos, 2);
			if(eos == true)
			{
				asr_result.push_back(msg);
				msg.clear();
			}
		}
		total_decoder_time += keep_time.Esapsed();

		// 返回识别结果
		if(nbest > 0 || eos == true)
		{
			std::string best_result;
			for(size_t i=0;i<asr_result.size();++i)
				best_result = best_result + asr_result[i];
			if(msg.size() > 0)
				best_result += msg;
			ser_s2c.SetNbest(best_result);
		}

		{ // send result from service to client.
			if(eos == true)
			{
				if(true != ser_s2c.S2CWrite(_connfd, S2CPackageAnalysis::S2CEND))
				{
					LOG_COM << "S2CWrite all end error.";
					break;
				}
			}
			else if(nbest > 0)
			{
				if(true != ser_s2c.S2CWrite(_connfd, S2CPackageAnalysis::S2CNOEND))
				{
					LOG_COM << "S2CWrite error.";
					break;
				}
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
			int sil_frames=0,nosil_frames=0;
            energy_vad.GetSilAndNosil(sil_frames, nosil_frames);
			float nosil_time = nosil_frames/100.0;

			LOG_COM << "wav time(s)\t:" << wav_time;
			LOG_COM << "nosil time(s)\t:" << nosil_time;
			LOG_COM << "run time(s)\t:" << total_decoder_time;
			LOG_COM << "decoder rt\t: " << total_decoder_time/wav_time;
			// send time to work thread.
			asr_work_thread->SetTime(wav_time, total_decoder_time,
				   	static_cast<double>(nosil_time));
			break; // end
		}
	} // while(1) end one audio
	asr_work->Destory();
	close(_connfd);
	VLOG(2) << "close " << _connfd << " ok.";
	return 0;
}

#include "src/util/namespace-end.h"

#endif

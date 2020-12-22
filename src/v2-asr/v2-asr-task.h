#ifndef __V2_ASR_TASK_H__
#define __V2_ASR_TASK_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include "src/v2-asr/v2-asr-work-thread.h"
#include "src/util/util-common.h"

#include "src/util/namespace-start.h"

class V2ASRServiceTask:public TaskBase
{
public:
	typedef TaskBase::int32 int32;
public:
	V2ASRServiceTask(int32 connfd, std::string task_name = ""):
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
	template<typename T>
	void SendDataAndGetResult(std::vector<T> &data,
			OnlineClgLatticeFastDecoder &online_clg_decoder, 
			bool end_flag,
			uint dtype_len,
			std::vector<std::string> &nbest_result,
			int nbest);

	int32 _connfd; // connect fd
	void SetConnFd(int32 connfd); // set socket id
};

int32 V2ASRServiceTask::Run(void *data)
{
	//signal(SIGPIPE, SIG_IGN); // ignore SIGPIPE to avoid crashing when socket forcefully disconnected
	V2ASRWorkThread * asr_work_thread = static_cast<V2ASRWorkThread *>(data);
	C2SPackageAnalysis &ser_c2s = asr_work_thread->_ser_c2s_package_analysys;
	S2CPackageAnalysis &ser_s2c = asr_work_thread->_ser_s2c_package_analysys;
	OnlineClgLatticeFastDecoder &online_clg_decoder = asr_work_thread->_online_clg_decoder;

	EnergyVad<short> &energy_vad = asr_work_thread->_energy_vad;
	energy_vad.Reset();
	bool use_energy_vad = online_clg_decoder.UseEnergyVad();
	//size_t chung=0;
	// V2ASRWorkThread reset
	ser_c2s.Reset(true);
	ser_s2c.Reset();
	online_clg_decoder.InitDecoding(0, true);
	int n=0; // read timeout times
	// time calculate
	Time keep_time;
	int total_wav_len = 0;
	float total_decoder_time = 0.0;
	uint dtype_len = 0;
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
					LOG_WARN << "|" << _connfd << "| timeout and disconnet";
					break;
				}
				continue;
			}
			LOG_WARN << "C2SRead error!!!" <<  " connect fd is " << _connfd;
			break;
		}
		ser_c2s.Print("ser_c2s");
		n=0;
		uint data_len;
		char *data = ser_c2s.GetData(&data_len);
		total_wav_len += data_len;
		// first audio type
		// todo convert to pcm format.
		//
		VLOG_COM(2) << "from |" << _connfd << "| receive \"" << data_len << "\"";
		// 8k ,16bit
		// whether to end
		bool eos= ser_c2s.IsEnd();
		if(eos == false && data_len == 0)
		{
			LOG_WARN << "Recv data length is Zero and eos is false!!!"
			   << "	It's not allow and eos will be change true.";
			eos = true;
		}
		std::cout << "eos is " << eos << std::endl;
		// get audio data type len
		dtype_len = ser_c2s.GetDtypeLen();
		// get result
		uint nbest = ser_c2s.GetNbest();

		std::vector<std::string> nbest_result;
		int32 ret = 0;
		// vad
		if(use_energy_vad == true)
		{
			std::vector<short> tmp_data(reinterpret_cast<short*>(data), reinterpret_cast<short*>(data)+data_len/2);
			energy_vad.JudgeFramesRes(tmp_data, eos);
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
						SendDataAndGetResult(out_data, online_clg_decoder,
								true, dtype_len,
								nbest_result, nbest);
					}
					else if(getdata_audio_flag == 1) // audio
					{
						if(vad_ret == 0 && eos == true) // energy_vad.GetData 只有在处理每次音频最后的时候才可能是语音段，所以vad_ret一定等于0
						{
							vad_end = true;
							SendDataAndGetResult(out_data, online_clg_decoder,
									true, dtype_len,
									nbest_result, nbest);
							// get result
						}
						else
						{
							vad_end = false;
							SendDataAndGetResult(out_data, online_clg_decoder,
									false, dtype_len,
									nbest_result, nbest);
						}
					}

					if(vad_end == true || eos == true)
					{
						if(nbest_result.size() > 0)
						{
							asr_result.push_back(nbest_result[0]);
						}
						nbest_result.clear();
					}
				}

				if(vad_ret == 0)
					break;
			} // while(1)
		} // use_energy_vad
		else
		{
			std::vector<short> out_data(reinterpret_cast<short*>(data), 
					reinterpret_cast<short*>(data)+data_len/2);
			SendDataAndGetResult(out_data, online_clg_decoder,
					eos, dtype_len,
					nbest_result, nbest); 
			if(eos == true)
			{
				if(nbest_result.size() > 0)
				{
					asr_result.push_back(nbest_result[0]);
				}
				nbest_result.clear();
			}
		}
		// 返回识别结果
		if(nbest > 0 || eos == true)
		{
			std::string best_result; 
			for(size_t i=0;i<asr_result.size();++i)
				best_result = best_result + asr_result[i];
			if(nbest_result.size() > 0)
				best_result += nbest_result[0];
			ser_s2c.SetNbest(best_result);
		}

		total_decoder_time += keep_time.Esapsed();

		{
			// send result from service to client.
			if(eos == true)
			{
				if(true != ser_s2c.S2CWrite(_connfd, S2CPackageAnalysis::S2CEND))
				{
					LOG_WARN << "S2CWrite all end error.";
					break;
				}
				ser_s2c.Print("ser_s2c" + ser_c2s.GetKey());
			}
			else if(ret == 1)
			{
				// end point judge cut audio.
				if(true != ser_s2c.S2CWrite(_connfd, S2CPackageAnalysis::S2CMIDDLEEND))
				{
					LOG_WARN << "S2CWrite middle end error.";
					break;
				}
			}
			else
			{
				if(true != ser_s2c.S2CWrite(_connfd, S2CPackageAnalysis::S2CNOEND))
				{
					LOG_WARN << "S2CWrite error.";
					break;
				}
			}
		}
		// send ok.
		//ser_s2c.Print("ser_s2c" + ser_c2s.GetKey());
		// reset service to client package info.
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
			int sil_frames=0,nosil_frames=0;
		   	energy_vad.GetSilAndNosil(sil_frames, nosil_frames);
			std::cout << "****************" << sil_frames << "," << nosil_frames << std::endl;
			float nosil_time = nosil_frames/100.0;
			float wav_time = total_wav_len*1.0/(smaple_rate*dtype_len);
			VLOG_COM(1) << "wav time(s)\t:" << wav_time;
			VLOG_COM(1) << "nosil time(s)\t:" << nosil_time;
			VLOG_COM(1) << "run time(s)\t:" << total_decoder_time;
			VLOG_COM(1) << "decoder rt\t: " << total_decoder_time/wav_time ;
			// send time to work thread.
			asr_work_thread->SetTime(static_cast<double>(wav_time),
				   static_cast<double>(total_decoder_time),
				   static_cast<double>(nosil_time));
			break; // end
		}
	}
	close(_connfd);
	VLOG_COM(2) << "close " << _connfd << " ok.";
	return 0;
}
template<typename T>
void V2ASRServiceTask::SendDataAndGetResult(std::vector<T> &data,
		OnlineClgLatticeFastDecoder &online_clg_decoder, 
		bool end_flag,
		uint dtype_len,
		std::vector<std::string> &nbest_result,
		int nbest)
{
	nbest_result.clear();

	std::string best_result;
	int ret = -1;
	if(end_flag == true)
	{// 当前数据结束
	   	ret = online_clg_decoder.ProcessData(reinterpret_cast<char*>(data.data()), data.size()*sizeof(short), 2, dtype_len);
		// 获取识别结果
		if (online_clg_decoder.NumFramesDecoded() > 0)
		{
			if(nbest <= 1)
			{
				online_clg_decoder.GetBestPathTxt(best_result, true);
				nbest_result.push_back(best_result);
			}
			else
				online_clg_decoder.GetNbestTxt(nbest_result, nbest, true);
		}
		online_clg_decoder.InitDecoding(0, true);
	}
	else
	{
		ret = online_clg_decoder.ProcessData(reinterpret_cast<char*>(data.data()), data.size()*sizeof(short), 0, dtype_len);
		// 获取识别结果
		if (online_clg_decoder.NumFramesDecoded() > 0)
		{
			if(nbest == 1)
			{
				online_clg_decoder.GetBestPathTxt(best_result, false);
				nbest_result.push_back(best_result);
			}
			else if(nbest > 1)
			{
				online_clg_decoder.GetNbestTxt(nbest_result, nbest, false);
			}
		}
	}
}

#include "src/util/namespace-end.h"

#endif

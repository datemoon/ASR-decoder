#ifndef __V1_ASR_GPU_TASK_H__
#define __V1_ASR_GPU_TASK_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <mutex>

#include "fstext/fstext-lib.h"
#include "lat/lattice-functions.h"
#include "lat/kaldi-lattice.h"

#include "src/gpu-asr/v1-gpu-asr-work-thread.h"

#include "src/util/namespace-start.h"

#define GPU_CORRID_OFFSET 100
class V1GpuASRServiceTask:public TaskBase
{
public:
	typedef kaldi::cuda_decoder::BatchThreadCudaPipelineItf::CorrelationID CorrelationID;
	typedef TaskBase::int32 int32;
	V1GpuASRServiceTask(int32 connfd, CorrelationID corr_id, std::string task_name = ""):
		TaskBase(task_name), _connfd(connfd), _corr_id(corr_id) 
	{
		 KALDI_VLOG(1) << "connfd : " << _connfd << " corr_id : " << _corr_id <<" task start!!!";
	}

	~V1GpuASRServiceTask() 
	{
		KALDI_VLOG(1) << "connfd : " << _connfd << " corr_id : " << _corr_id <<" task end!!!";
		Stop();
   	}
	int32 RunVad(void *data)
	{
		V1GpuASRWorkThread *v1_gpu_asr_work_thread = 
			static_cast<V1GpuASRWorkThread*> (data);
		C2SPackageAnalysis &ser_c2s = v1_gpu_asr_work_thread->_ser_c2s_package_analysys;
		S2CPackageAnalysis &ser_s2c = v1_gpu_asr_work_thread->_ser_s2c_package_analysys;
		// energy vad
		kaldi::EnergyVadProcess<kaldi::BaseFloat> &energy_vad = v1_gpu_asr_work_thread->_energy_vad;
		//const V1GpuASRWorkThreadOpt &v1_gpu_asr_work_thread_opts 
		//	= v1_gpu_asr_work_thread->_v1_gpu_asr_work_thread_opts;

		kaldi::cuda_decoder::BatchThreadCudaPipelineItf &cuda_pipeline_warp 
			= v1_gpu_asr_work_thread->_cuda_pipeline_warp;
		const kaldi::cuda_decoder::V1CudaOnlineBinaryOptions &cudaopts 
			= cuda_pipeline_warp.GetOptions();
		
//		std::mutex callback_mutex;  // call back thread lock
//		callback_mutex.lock();
		pthread_mutex_t callback_mutex;  
		//const pthread_mutex_t *p_callback_mutex = &callback_mutex;
		if(pthread_mutex_init(&callback_mutex, NULL) != 0)
		{
			LOG_ERR << "pthread_mutex_init callback_mutex failed.";
			return -1;
		}
		// lock callback 
		//pthread_mutex_lock(&callback_mutex);
		ser_c2s.Reset(true);
		ser_s2c.Reset();
		energy_vad.Reset();
		bool pre_is_sil = true;

		bool end_flag = false;
		CorrelationID &corr_id = _corr_id;
		int32 connfd = _connfd;
		auto BestPathCallback = 
				[connfd, &corr_id, &ser_s2c, cudaopts, &callback_mutex, &end_flag](const std::string &best_str, bool partial, bool endpoint_detected)
				{
					if(partial)
						KALDI_LOG << "connfd :" << connfd << " corr_id #" << corr_id << " [partial] : " << best_str;
					if (endpoint_detected)
						KALDI_LOG << "connfd :" << connfd << " corr_id #" << corr_id << " [endpoint detected]";
				    if (!cudaopts.rec_use_final && !partial)
					{
					    KALDI_LOG << "connfd :" << connfd << " corr_id #" << corr_id << " : " << best_str;
						ser_s2c.SetNbest(best_str);
						if(true != ser_s2c.S2CWrite(connfd, 
									end_flag ? S2CEND : S2CMIDDLEEND))
						{
						    LOG_COM << "S2CWrite all end error.";
						}
						ser_s2c.Print("ser_s2c");
						//ser_s2c.Reset();
						//callback_mutex.unlock();
						pthread_mutex_unlock(&callback_mutex);
					}
				};
		std::string key(_task_name);
		const fst::SymbolTable *word_syms = cuda_pipeline_warp.GetSymbolTable();
		auto LatticeCallback =
				[connfd, &corr_id, &ser_s2c, &key, word_syms, cudaopts, &callback_mutex, &end_flag](kaldi::CompactLattice &clat)
				{
					KALDI_VLOG(3) << "LatticeCallback!!!";
					if (cudaopts.rec_use_final)
					{
						kaldi::CompactLattice clat_best_path;
						kaldi::CompactLatticeShortestPath(clat, &clat_best_path);
						kaldi::Lattice best_path;
						fst::ConvertLattice(clat_best_path, &best_path);
						std::vector<int32> alignment;
						std::vector<int32> words;
						kaldi::LatticeWeight weight;
						fst::GetLinearSymbolSequence(best_path, &alignment, &words, &weight);
						KALDI_LOG << "For utterance " << key
							<< " connfd #" << connfd
							<< " corr_id #" << corr_id
							<< ", best cost "
							<< weight.Value1() << " + " << weight.Value2() << " = "
							<< (weight.Value1() + weight.Value2())
							<< " over " << alignment.size() << " frames.";
						if(word_syms != NULL)
						{
							std::string res;
							for(size_t i = 0; i<words.size(); ++i)
								res += word_syms->Find(words[i]) + " ";
							KALDI_LOG << "connfd #" << connfd 
								<< " corr_id #" << corr_id
								<< " : " <<res;
							{
								ser_s2c.SetNbest(res);
								if(true != ser_s2c.S2CWrite(connfd, 
											end_flag ? S2CEND : S2CMIDDLEEND))
								{
									LOG_COM << "S2CWrite all end error.";
								}
								ser_s2c.Print("ser_s2c");
								//ser_s2c.Reset();
							}
						}
						//callback_mutex.unlock();
						pthread_mutex_unlock(&callback_mutex);
					}
				};
		
		int n=0; // read timeout times
		int nret = 0;
		uint dtype_len=2;
		int total_wav_len = 0;
		double total_decoder_time = 0.0;
		// 由于传入SubVector所以外部需要缓存音频数据，直到解码完成才能释放，
		// 这块其实可以将SubVector改成Vector这样就可以使用完立即释放
		std::vector<kaldi::Vector<kaldi::BaseFloat>* > tot_data;
		Time keep_time;
		keep_time.Esapsed();
		// 记录最后一次的非静音数据段个数，如果等于0，会在最后追加发送结束包
		size_t last_data_len = 0;
		while(1)
		{
			if(true != ser_c2s.C2SRead(_connfd))
			{ // read data
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
			key = ser_c2s.GetKey();
			ser_c2s.Print("ser_c2s");
			n = 0;
			uint data_len;
			char *data = ser_c2s.GetData(&data_len);
			total_wav_len += data_len;
			VLOG_COM(1) << "from |" << _connfd << "| receive \"" << data_len << "\"";
			// 8k ,16bit
			bool eos = ser_c2s.IsEnd();
			if(eos == false && data_len == 0)
			{
				LOG_WARN << "Recv data length is Zero and eos is false!!!"
					<< " It's not allow and eos will be change true.";
				eos = true;
			}
			// get audio data type len
			dtype_len = ser_c2s.GetDtypeLen();
			if(dtype_len != 2 || dtype_len != 4)
				dtype_len = 2;
			KALDI_VLOG(5) << "send in data: connfd : " << _connfd
				<< ", data_len : " << data_len/dtype_len;

			// process send vad data.
			kaldi::Vector<kaldi::BaseFloat> in_data;
			if(data_len%dtype_len != 0)
			{
				KALDI_WARN << "Data have some problem!!!";
			}
			int32 len = data_len/dtype_len;
			in_data.Resize(len);
			for(int32 i=0; i < len ; ++i)
			{
				if(dtype_len == 2)
					in_data(i) = ((short*)(data))[i];
				else if(dtype_len == 4)
					in_data(i) = ((BaseFloat*)(data))[i];
			}

			std::vector<std::vector<BaseFloat> > vad_data;
			energy_vad.PushData(in_data);
			bool cur_end = energy_vad.GetData(vad_data, eos);
			bool is_first_chunk=true, is_last_chunk = false;
			last_data_len = vad_data.size();
			for(std::vector<std::vector<BaseFloat> >::iterator 
					it = vad_data.begin(); it != vad_data.end(); ++it)
			{
				// 在设置BestPathCallback，LatticeCallback 之前必须设置end_flag
				// 因为end_flag是回调函数的可变参数
				if(it+1 == vad_data.end())
					end_flag = eos;
				else
					end_flag = false;

				if(pre_is_sil == true)
				{ // 之前是静音，说明当前段是新的一段语音，初始化callback
					pthread_mutex_lock(&callback_mutex);
					cuda_pipeline_warp.SetBestPathCallback(corr_id, BestPathCallback);
					cuda_pipeline_warp.SetLatticeCallback(corr_id, LatticeCallback);
					is_first_chunk=true;
					KALDI_VLOG(3) << "corr_id #" << corr_id << "==" << _corr_id << " Set call back!!!";
				}
				else
				{
					is_first_chunk=false;
				}

				if(it+1 == vad_data.end())
				{ // last wav
					if(cur_end == true)
						is_last_chunk = true;
					else
					{
						is_last_chunk = false;
						pre_is_sil = false;
					}
				}
				else
					is_last_chunk = true;
				// 送入数据
				kaldi::Vector<kaldi::BaseFloat> *tmp_data = new kaldi::Vector<kaldi::BaseFloat>(it->size());
				//tmp_data->Resize(it->size());
				memcpy(tmp_data->Data(), it->data(), it->size() *sizeof(kaldi::BaseFloat));
				tot_data.push_back(tmp_data);
				cuda_pipeline_warp.Push(_corr_id, is_first_chunk, is_last_chunk,
						*tmp_data);

				if(is_last_chunk == true)
				{ // wait asr callback end.
					pthread_mutex_lock(&callback_mutex);
					nret ++;
					KALDI_LOG << "Vad cut " << nret << " times";
					for(size_t i = 0 ; i < tot_data.size(); ++i)
						delete tot_data[i];
					tot_data.clear();
					_corr_id += nret%GPU_CORRID_OFFSET;
					// 每次解码结束，都必须设置pre_is_sil=true，
					// 代表重新解码需要初始化
					pre_is_sil = true;
					pthread_mutex_unlock(&callback_mutex);
				}
			}

			if(true == eos)
			{
				break;
			}
		}
		if(nret == 0 || (ser_c2s.IsEnd() == true && last_data_len == 0 ))
		{ //  vad判断全是静音，或者最后一次送入数据是静音
			ser_s2c.SetNbest("");
			if(true != ser_s2c.S2CWrite(_connfd, S2CEND))
			{
				LOG_COM << "S2CWrite all end error.";
			}
			ser_s2c.Print("ser_s2c");
		}

		total_decoder_time += keep_time.Esapsed();
		while(1)
		{
			uint smaple_rate = 16000;
			if(ser_c2s.GetSampleRate() == C2SPackageAnalysis::K_16)
				smaple_rate=16000;
			else if(ser_c2s.GetSampleRate() == C2SPackageAnalysis::K_8)
				smaple_rate=8000;
			else if(ser_c2s.GetSampleRate() == C2SPackageAnalysis::K_32)
				smaple_rate=32000;
			else
			{
				KALDI_LOG << "Sample rate is error!!!";
				break;
			}
			double wav_time = total_wav_len*1.0/(smaple_rate*dtype_len);
			
			int sil_frames=0, nosil_frames = 0;
			energy_vad.GetInfo(sil_frames, nosil_frames);
			float nosil_time = nosil_frames/100.0;

			KALDI_VLOG(1) << "wav time(s)\t:" << wav_time;
			KALDI_VLOG(1) << "nosil time(s)\t:" << nosil_time;
			KALDI_VLOG(1) << "run time(s)\t:" << total_decoder_time;
			KALDI_VLOG(1) << "decoder rt\t: " << total_decoder_time/wav_time;
			v1_gpu_asr_work_thread->SetTime(wav_time, total_decoder_time,
					static_cast<double>(nosil_time));
			break;
		}

		return 0;
	}

	int32 RunNoVad(void *data)
	{
		V1GpuASRWorkThread *v1_gpu_asr_work_thread = 
			static_cast<V1GpuASRWorkThread*> (data);
		C2SPackageAnalysis &ser_c2s = v1_gpu_asr_work_thread->_ser_c2s_package_analysys;
		S2CPackageAnalysis &ser_s2c = v1_gpu_asr_work_thread->_ser_s2c_package_analysys;
		kaldi::cuda_decoder::BatchThreadCudaPipelineItf &cuda_pipeline_warp = v1_gpu_asr_work_thread->_cuda_pipeline_warp;
		const kaldi::cuda_decoder::V1CudaOnlineBinaryOptions &cudaopts = cuda_pipeline_warp.GetOptions();
		// 由于传入SubVector所以外部需要缓存音频数据，直到解码完成才能释放，
		// 这块其实可以将SubVector改成Vector这样就可以使用完立即释放
		std::vector<kaldi::Vector<kaldi::BaseFloat>* > tot_data;
		
//		std::mutex callback_mutex;  // call back thread lock
//		callback_mutex.lock();
		pthread_mutex_t callback_mutex;  
		//const pthread_mutex_t *p_callback_mutex = &callback_mutex;
		if(pthread_mutex_init(&callback_mutex, NULL) != 0)
		{
			LOG_ERR << "pthread_mutex_init callback_mutex failed.";
			return -1;
		}
		// lock callback 
		pthread_mutex_lock(&callback_mutex);

		ser_c2s.Reset(true);
		ser_s2c.Reset();
		// 设置回调函数
		CorrelationID &corr_id = _corr_id;
		int32 connfd = _connfd;
		cuda_pipeline_warp.SetBestPathCallback(corr_id,
				[connfd, &corr_id, &ser_s2c, cudaopts, &callback_mutex](const std::string &best_str, bool partial, bool endpoint_detected)
				{
					if(partial)
						KALDI_LOG << "corr_id #" << corr_id << " [partial] : " << best_str;
					if (endpoint_detected)
						KALDI_LOG << "corr_id #" << corr_id << " [endpoint detected]";
				    if (!partial && !cudaopts.rec_use_final)
					{
					    KALDI_LOG << "corr_id #" << corr_id << " : " << best_str;
						ser_s2c.SetNbest(best_str);
						if(true != ser_s2c.S2CWrite(connfd, S2CEND))
						{
						    LOG_COM << "S2CWrite all end error.";
						}
						ser_s2c.Print("ser_s2c");
						//callback_mutex.unlock();
						pthread_mutex_unlock(&callback_mutex);
					}
				});
		std::string key(_task_name);
		const fst::SymbolTable *word_syms = cuda_pipeline_warp.GetSymbolTable();
		cuda_pipeline_warp.SetLatticeCallback(
				corr_id, [connfd, &corr_id, &ser_s2c, key, word_syms, cudaopts, &callback_mutex](kaldi::CompactLattice &clat)
				{
					KALDI_VLOG(3) << "LatticeCallback!!!";
					if (cudaopts.rec_use_final)
					{
						kaldi::CompactLattice clat_best_path;
						kaldi::CompactLatticeShortestPath(clat, &clat_best_path);
						kaldi::Lattice best_path;
						fst::ConvertLattice(clat_best_path, &best_path);
						std::vector<int32> alignment;
						std::vector<int32> words;
						kaldi::LatticeWeight weight;
						fst::GetLinearSymbolSequence(best_path, &alignment, &words, &weight);
						KALDI_LOG << "For utterance " << key
							<< "connfd #" << connfd
							<< "corr_id #" << corr_id
							<< ", best cost "
							<< weight.Value1() << " + " << weight.Value2() << " = "
							<< (weight.Value1() + weight.Value2())
							<< " over " << alignment.size() << " frames.";
						if(word_syms != NULL)
						{
							std::string res;
							for(size_t i = 0; i<words.size(); ++i)
								res += word_syms->Find(words[i]) + " ";
							KALDI_LOG << "corr_id #" << corr_id
								<< " : " <<res;
							{
								ser_s2c.SetNbest(res);
								if(true != ser_s2c.S2CWrite(connfd, S2CEND))
								{
									LOG_COM << "S2CWrite all end error.";
								}
								ser_s2c.Print("ser_s2c");
							}
						}
						//callback_mutex.unlock();
						pthread_mutex_unlock(&callback_mutex);
					}
				});

//#define SAVE_REC_FILE
#ifdef SAVE_REC_FILE
		// 保存接收的文件
		std::string filename("rec.pcm");
		FILE *rec_fp = fopen(filename.c_str(), "wb");
#endif
		int n=0; // read timeout times
		// calculate time
		uint dtype_len=2;
		int total_wav_len = 0;
		double total_decoder_time = 0.0;
		Time keep_time;
		keep_time.Esapsed();
		while(1)
		{
			if(true != ser_c2s.C2SRead(_connfd))
			{ // read data
				if(errno == EAGAIN || errno == EINPROGRESS)
				{
					LOG_COM << "|" << _connfd << "| timeout and continue";
					n++;
					if(n>2)
					{
						n = 0;
						LOG_COM << "|" << _connfd << "| timeout and disconnet";
						break;
					}
					continue;
				}
				LOG_COM << "C2SRead error." << std::endl;
				break;
			}
			ser_c2s.Print("ser_c2s");
			n = 0;
			uint data_len;
			char *data = ser_c2s.GetData(&data_len);
#ifdef SAVE_REC_FILE
			fwrite((const void*)data, sizeof(char), data_len, rec_fp);
#endif
			total_wav_len += data_len;
			VLOG_COM(1) << "from |" << _connfd << "| receive \"" << data_len << "\"";
			// 8k ,16bit
			bool eos = ser_c2s.IsEnd();
			if(eos == false && data_len == 0)
			{
				LOG_WARN << "Recv data length is Zero and eos is false!!!"
					<< " It's not allow and eos will be change true.";
				eos = true;
			}
			// get audio data type len
			dtype_len = ser_c2s.GetDtypeLen();
			if(dtype_len == -1)
				dtype_len = 2;
			// get result
			// uint nbest = ser_c2s.GetNbest();

			//uint n_package = ser_c2s.GetN();
			bool is_first_chunk = ser_c2s.IsStart();
			bool is_last_chunk = eos;
			KALDI_VLOG(5) << "send in data: _connfd : " << _connfd
				<< ", start : " << is_first_chunk 
				<< ", end : " << is_last_chunk 
				<< ", data_len : " << data_len/dtype_len;
				
			// 送入数据
			kaldi::Vector<kaldi::BaseFloat> *tmp_data = new kaldi::Vector<kaldi::BaseFloat>();
			kaldi::Vector<kaldi::BaseFloat> &in_data = *tmp_data;
			if(data_len%dtype_len != 0)
			{
				KALDI_WARN << "Data have some problem!!!";
			}
			int32 len = data_len/dtype_len;
			in_data.Resize(len);
			for(int32 i=0; i < len ; ++i)
			{
				if(dtype_len == 2)
					in_data(i) = ((short*)(data))[i];
				else if(dtype_len == 4)
					in_data(i) = ((BaseFloat*)(data))[i];
			}
			tot_data.push_back(tmp_data);
			cuda_pipeline_warp.Push(_corr_id, is_first_chunk, is_last_chunk,
					in_data);
			//cuda_pipeline_warp.Push(_corr_id, is_first_chunk, is_last_chunk,
			//		data, data_len, dtype_len);
			if(is_last_chunk)
				break;

		} // while(1)

		// 应该等待 callback function run end.
		pthread_mutex_lock(&callback_mutex);
		// 所有数据送入完毕，等待结束统计运行时间
		total_decoder_time += keep_time.Esapsed();
		while(1)
		{
			uint smaple_rate = 16000;
			if(ser_c2s.GetSampleRate() == C2SPackageAnalysis::K_16)
				smaple_rate=16000;
			else if(ser_c2s.GetSampleRate() == C2SPackageAnalysis::K_8)
				smaple_rate=8000;
			else if(ser_c2s.GetSampleRate() == C2SPackageAnalysis::K_32)
				smaple_rate=32000;
			else
			{
				KALDI_LOG << "Sample rate is error!!!";
				break;
			}
			double wav_time = total_wav_len*1.0/(smaple_rate*dtype_len);
			int sil_frames=0;//,nosil_frames=0;
			float nosil_time = wav_time - sil_frames/100.0;

			KALDI_VLOG(1) << "wav time(s)\t:" << wav_time;
			KALDI_VLOG(1) << "nosil time(s)\t:" << nosil_time;
			KALDI_VLOG(1) << "run time(s)\t:" << total_decoder_time;
			KALDI_VLOG(1) << "decoder rt\t: " << total_decoder_time/wav_time;
			// send time to work thread.
			v1_gpu_asr_work_thread->SetTime(wav_time, total_decoder_time,
					static_cast<double>(nosil_time));
			break;
		}

		//callback_mutex.lock();
		for(size_t i = 0 ; i < tot_data.size(); ++i)
			delete tot_data[i];
		tot_data.clear();
		KALDI_VLOG(3) << "Callback ok!!!";
		//callback_mutex.unlock();
		pthread_mutex_unlock(&callback_mutex);
		
#ifdef SAVE_REC_FILE
		fclose(rec_fp);
#endif
		return 0;
	}
	int32 Run(void *data)
	{
		V1GpuASRWorkThread *v1_gpu_asr_work_thread = 
			static_cast<V1GpuASRWorkThread*> (data);
		const V1GpuASRWorkThreadOpt &v1_gpu_asr_work_thread_opts 
			= v1_gpu_asr_work_thread->_v1_gpu_asr_work_thread_opts;
		if(v1_gpu_asr_work_thread_opts._use_energy_vad == true)
		{
			return RunVad(data);
		}
		else
		{
			return RunNoVad(data);
		}
	}

	int32 Stop()
	{
		//const char *str = "no idle thread.";
		//send(_connfd, str, strlen(str), 0);
		//printf("send |%d| : %s ok\n",_connfd, str);
		if(_connfd != -1)
		{
			close(_connfd);
			_connfd = -1;
		}
		return 0;
	}

	void GetInfo()
	{
		LOG_COM << "Task name is : " << _task_name ;
		LOG_COM << "Task connetid is : " << _connfd;
	}
private:
	int32 _connfd; // connect fd
	// 引入corr_id不同于_connfd的原因在于不知道什么时候注册的corr_id被释放，
	// 虽然在获取结果的同时corr_id就应该被释放，但实际上是不同步的，
	// 当使用vad的时候，音频被切割成多段，由于是同一个连接所以_connfd是不变的，
	// 所以为了防止同一个corr_id被同时使用，引入_corr_id
	CorrelationID _corr_id;
	void SetConnFd(int32 connfd); // set socket id
};

#include "src/util/namespace-end.h"
#endif

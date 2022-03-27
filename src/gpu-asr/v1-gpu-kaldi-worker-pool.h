#ifndef __V1_GPU_KALDI_WORKER_POOL_H__
#define __V1_GPU_KALDI_WORKER_POOL_H__

#include <iomanip>
#include <iostream>
#include <vector>
#include "cudadecoder/batched-threaded-nnet3-cuda-online-pipeline.h"
#include "cudadecoder/cuda-online-pipeline-dynamic-batcher.h"
#include "fstext/fstext-lib.h"
#include "nnet3/am-nnet-simple.h"
#include "nnet3/nnet-utils.h"

#include "src/gpu-asr/gpu-worker-pool-itf.h"

namespace kaldi {
namespace cuda_decoder {

// 这是一个kaldi cudadecoder的服务线程，完成解码和分配工作
// 这个类主要完成特征，声学和解码，网络相关的线程服务详见asr-service.
class BatchedThreadedNnet3CudaOnlinePipelineWarp:public BatchThreadCudaPipelineItf
{
public:
	typedef typename BatchThreadCudaPipelineItf::CorrelationID CorrelationID;
	typedef typename BatchThreadCudaPipelineItf::BestPathCallback BestPathCallback;
	typedef typename BatchThreadCudaPipelineItf::LatticeCallback LatticeCallback;
	BatchedThreadedNnet3CudaOnlinePipelineWarp(
			const V1CudaOnlineBinaryOptions &cuda_pipeline_opts,
			const BatchedThreadedNnet3CudaOnlinePipelineConfig &batched_decoder_config,
			const CudaOnlinePipelineDynamicBatcherConfig &dynamic_batcher_opts,
			const fst::Fst<fst::StdArc> &decode_fst,
			const nnet3::AmNnetSimple &am_nnet, 
			const TransitionModel &trans_model,
			fst::SymbolTable *word_syms=NULL):
		BatchThreadCudaPipelineItf(cuda_pipeline_opts, word_syms),
		//_cuda_pipeline_opts(cuda_pipeline_opts),
		_dynamic_batcher_opts(dynamic_batcher_opts),
		_cuda_pipeline(batched_decoder_config,
				decode_fst, am_nnet, trans_model),
		_dynamic_batcher(_dynamic_batcher_opts, _cuda_pipeline)
		//_word_syms(word_syms)
	{
	   if(word_syms != NULL)
		   _cuda_pipeline.SetSymbolTable(*word_syms);
	}

	void SetBestPathCallback(CorrelationID corr_id,
			const BestPathCallback &callback)
	{
		_cuda_pipeline.SetBestPathCallback(corr_id, callback);
	}

	void SetLatticeCallback(CorrelationID corr_id,
			const LatticeCallback &callback)
	{
		_cuda_pipeline.SetLatticeCallback(corr_id, callback);
	}

	void Push(CorrelationID corr_id, bool is_first_chunk, bool is_last_chunk,
			const SubVector<BaseFloat> &wave_part)
	{
		_dynamic_batcher.Push(corr_id, is_first_chunk, is_last_chunk,
				wave_part);
	}

	void Push(CorrelationID corr_id, bool is_first_chunk, bool is_last_chunk,
			const Vector<BaseFloat> &in_data)
	{
		KALDI_VLOG(3) << "send all in data: corr_id : " << corr_id 
			<< ", start : " << is_first_chunk
			<< ", end : " << is_last_chunk
			<< ", data_len : " << in_data.Dim();
		// 由于送入数据受--frames-per-chunk参数限制，所以在这里循环送入，
		// 如果data_len超过的话
		//int32 input_frames_per_chunk = _cuda_pipeline.GetConfig().compute_opts.frames_per_chunk;
		//int32 model_frequency = _cuda_pipeline.GetModelFrequency();

		//int32 samples_per_chunk = 1.5 * 16000; // 150 frames
		int32 samples_per_chunk = _cuda_pipeline.GetNSampsPerChunk();

		int32 offset_start = 0;
		bool start = is_first_chunk;
		bool end = is_last_chunk;
		while(1)
		{
			int32 offset_end = offset_start+samples_per_chunk ;
			if(offset_end >= in_data.Dim())
			{
				offset_end = in_data.Dim();
				end = is_last_chunk;
			}
			else
			{
				end = false;
			}
			SubVector<BaseFloat> wave_part(in_data, offset_start, offset_end-offset_start);
			if(0 == offset_start)
				start = is_first_chunk;
			else
				start = false;
			KALDI_VLOG(3) << "send in data: corr_id : " << corr_id 
				<< ", start : " << start
				<< ", end : " << end
				<< ", data_len : " << wave_part.Dim();
			Push(corr_id, start, end, wave_part);
			if(offset_end == in_data.Dim())
				break;
			else
				offset_start = offset_end;
		}
	}

	void Push(CorrelationID corr_id, bool is_first_chunk, bool is_last_chunk,
			const char *data, int32 data_len, int32 data_type)
	{
		Vector<BaseFloat> in_data;
		if(data_len%data_type != 0)
		{
			KALDI_WARN << "Data have some problem!!!";
		}
		int32 len = data_len/data_type;
		in_data.Resize(len);
		for(int32 i=0; i < len ; ++i)
		{
			if(data_type == 2)
				in_data(i) = ((short*)(data))[i];
			else if(data_type == 4)
				in_data(i) = ((BaseFloat*)(data))[i];
		}
		// 由于送入数据受--frames-per-chunk参数限制，所以在这里循环送入，
		// 如果data_len超过的话
		//int32 samples_per_chunk = 1.5 * 16000; // 150 frames
		int32 samples_per_chunk = _cuda_pipeline.GetNSampsPerChunk();
		int32 offset_start = 0;
		bool start = is_first_chunk;
		bool end = is_last_chunk;
//#define SAVE_REC_FILE
#ifdef SAVE_REC_FILE
		// 保存接收的文件
		std::string filename("true_rec.pcm");
		std::string filename1("in_rec.pcm");
		FILE *rec_fp = fopen(filename.c_str(), "ab+");
		FILE *in_rec_fp = fopen(filename1.c_str(), "ab+");
		fwrite((const void *)in_data.Data(), sizeof(BaseFloat), len, in_rec_fp);
		fclose(in_rec_fp);
#endif
		while(1)
		{
			int32 offset_end = offset_start+samples_per_chunk ;
			if(offset_end >= in_data.Dim())
			{
				offset_end = in_data.Dim();
				end = is_last_chunk;
			}
			else
			{
				end = false;
			}
			SubVector<BaseFloat> wave_part(in_data, offset_start, offset_end-offset_start);
#ifdef SAVE_REC_FILE
			fwrite((const void *)wave_part.Data(), sizeof(BaseFloat), offset_end-offset_start, rec_fp);
#endif
			if(0 == offset_start)
				start = is_first_chunk;
			else
				start = false;
			KALDI_VLOG(3) << "send in data: corr_id : " << corr_id 
				<< ", start : " << start
				<< ", end : " << end
				<< ", data_len : " << wave_part.Dim();
			Push(corr_id, start, end, wave_part);
			if(offset_end == in_data.Dim())
				break;
			else
				offset_start = offset_end;
		}
#ifdef SAVE_REC_FILE
		fclose(rec_fp);
#endif
	}

	void WaitForCompletion()
	{
		_dynamic_batcher.WaitForCompletion();
	}
/*
	const fst::SymbolTable *GetSymbolTable()
	{
		return _word_syms;
	}
	const V1CudaOnlineBinaryOptions &GetOptions()
	{
		return _cuda_pipeline_opts;
	}
*/
private:
	//const V1CudaOnlineBinaryOptions &_cuda_pipeline_opts;
	const CudaOnlinePipelineDynamicBatcherConfig &_dynamic_batcher_opts;

	// BatchedThreadedNnet3CudaOnlinePipeline io is thread safe ,
	// so here don't add mutex.
	BatchedThreadedNnet3CudaOnlinePipeline _cuda_pipeline;
	CudaOnlinePipelineDynamicBatcher _dynamic_batcher;
	//const fst::SymbolTable *_word_syms;
};

} // namespace cuda_decoder
} // namespace kaldi

#endif

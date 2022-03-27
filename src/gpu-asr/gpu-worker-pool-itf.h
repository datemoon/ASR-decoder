#ifndef __GPU_WORKER_POOL_ITF_H__
#define __GPU_WORKER_POOL_ITF_H__

#include <vector>
#include "cudadecoder/batched-threaded-nnet3-cuda-online-pipeline.h"
#include "fstext/fstext-lib.h"

namespace kaldi {
namespace cuda_decoder {


struct V1CudaOnlineBinaryOptions
{
public:
	bool write_lattice;
  	int num_todo;
	int niterations;
	int num_streaming_channels;
	bool print_partial_hypotheses;
	bool print_hypotheses;
	bool print_endpoints;
	bool generate_lattice;
	bool rec_use_final;
	std::string word_syms_rxfilename, nnet3_rxfilename, fst_rxfilename,
		wav_rspecifier, clat_wspecifier;

	std::string torch_model_filename;
	std::string word_map_token_file;


	V1CudaOnlineBinaryOptions()
	{
		write_lattice = false;
		num_todo = -1;
		niterations = 1;
		num_streaming_channels = 2000;
		print_partial_hypotheses = false;
		print_hypotheses = false;
		print_endpoints = false;
		generate_lattice = false;
		rec_use_final = false;
	}
	void Register(ParseOptions *opts)
	{
		opts->Register("print-hypotheses", &print_hypotheses,
				"Prints the final hypotheses");
		opts->Register("print-partial-hypotheses", &print_partial_hypotheses,
				"Prints the partial hypotheses");
		opts->Register("print-endpoints", &print_endpoints,
				"Prints the detected endpoints");
		opts->Register("word-symbol-table", &word_syms_rxfilename,
				"Symbol table for words [for debug output]");
		opts->Register("file-limit", &num_todo,
				"Limits the number of files that are processed by "
				"this driver. "
				"After N files are processed the remaining files "
				"are ignored. "
				"Useful for profiling");
		opts->Register("iterations", &niterations,
				"Number of times to decode the corpus. Output will "
				"be written "
				"only once.");
		opts->Register("num-parallel-streaming-channels", &num_streaming_channels,
				"Number of channels streaming in parallel");
		opts->Register("generate-lattice", &generate_lattice,
				"Generate full lattices");
		opts->Register("write-lattice", &write_lattice, "Output lattice to a file");
		opts->Register("rec-use-final", &rec_use_final,
				"receive decoder result use final");
	}
};

//template<typename CorrelationID = BatchedThreadedNnet3CudaOnlinePipeline::CorrelationID,
//	typename BestPathCallback = BatchedThreadedNnet3CudaOnlinePipeline::BestPathCallback,
//	typename LatticeCallback = BatchedThreadedNnet3CudaOnlinePipeline::LatticeCallback>
class BatchThreadCudaPipelineItf
{
public:
	//using CorrelationID = uint64_t;
	//typedef std::function<void(const std::string &, bool, bool)> BestPathCallback;
	//typedef std::function<void(CompactLattice &)> LatticeCallback;
	typedef typename BatchedThreadedNnet3CudaOnlinePipeline::CorrelationID CorrelationID;
	typedef typename BatchedThreadedNnet3CudaOnlinePipeline::BestPathCallback BestPathCallback;
	typedef typename BatchedThreadedNnet3CudaOnlinePipeline::LatticeCallback LatticeCallback;
	typedef std::vector<std::pair<std::string, std::pair<float, float> > > AlignStruct;
	typedef std::function<void(const std::string &, AlignStruct&, bool, bool)> BestPathAndAlignCallback;

	BatchThreadCudaPipelineItf(
			const V1CudaOnlineBinaryOptions &cuda_pipeline_opts,
			const fst::SymbolTable *word_syms)
		:_cuda_pipeline_opts(cuda_pipeline_opts), _word_syms(word_syms) { }

	virtual void SetBestPathCallback(CorrelationID corr_id,
			const BestPathCallback &callback) = 0;

	virtual void SetBestPathCallback(CorrelationID corr_id,
			const BestPathAndAlignCallback &callback) { }

	virtual void SetLatticeCallback(CorrelationID corr_id,
			const LatticeCallback &callback) = 0;

	virtual void Push(CorrelationID corr_id, bool is_first_chunk, bool is_last_chunk,
			const SubVector<BaseFloat> &wave_part) = 0;

	virtual void Push(CorrelationID corr_id, bool is_first_chunk, bool is_last_chunk,
			const Vector<BaseFloat> &in_data) = 0;

	virtual void Push(CorrelationID corr_id, bool is_first_chunk, bool is_last_chunk,
			const char *data, int32 data_len, int32 data_type) = 0;

	virtual void WaitForCompletion() = 0;

	const fst::SymbolTable *GetSymbolTable()
	{
		return _word_syms;
	}

	const V1CudaOnlineBinaryOptions &GetOptions()
	{
		return _cuda_pipeline_opts;
	}
protected:
	const V1CudaOnlineBinaryOptions &_cuda_pipeline_opts;
	const fst::SymbolTable *_word_syms;
};

} // namespace cuda_decoder
} // namespace kaldi

#endif

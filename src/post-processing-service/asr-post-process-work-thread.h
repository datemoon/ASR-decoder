#ifndef __ASR_POST_PROCESS_WORK_THREAD_H__
#define __ASR_POST_PROCESS_WORK_THREAD_H__

#include "src/service2/net-data-package.h"
#include "src/service2/thread-pool-work-thread.h"

#include "src/post-processing-service/post-package.h"
#include "src/post-processing-service/const-lm-rescore.h"
#include "src/post-processing-service/asr-post-process-task.h"
#include "src/post-processing-service/lattice-to-result.h"

#include "src/util/namespace-start.h"

struct AsrPostProcessWorkThreadOpt
{
	bool _null; // no use
	void Register(kaldi::OptionsItf *opts)
	{ ; }
};

class AsrPostProcessWorkThread: public ThreadPoolWorkThread
{
public:
	friend class AsrPostProcessServiceTask;
	typedef ThreadBase::int32 int32;

	AsrPostProcessWorkThread(ThreadPoolBase<ThreadBase> *tp,
			const AsrPostProcessWorkThreadOpt &opts,
			const kaldi::ConstArpaLm& oldlm, 
			const kaldi::ConstArpaLm& newlm,
			const fst::SymbolTable &word_syms,
		   	BaseFloat oldscale=-1.0, BaseFloat newscale=1.0,
			bool determinize=false):
		ThreadPoolWorkThread(tp),
		_opts(opts), 
		_old_kaldi_const_arpa_lmrescore(oldlm, oldscale, determinize), 
		_new_kaldi_const_arpa_lmrescore(newlm, newscale, determinize),
		_word_syms(word_syms) { }

	~AsrPostProcessWorkThread() {}

private:

	// composed_lat 会在内部申请内存，所以需要*&，在外部释放
	template<typename Arc>
	bool LmRescore(fst::VectorFst<Arc> &lat,
			fst::VectorFst<Arc> *composed_lat)
	{
		fst::VectorFst<Arc> compose_old_lat;
		if(_old_kaldi_const_arpa_lmrescore.Compose(lat, &compose_old_lat)
				!= true)
		{
			LOG_WARN << "_old_kaldi_const_arpa_lmrescore.Compose failed!!!";
			return false;
		}
		if(_new_kaldi_const_arpa_lmrescore.Compose(compose_old_lat, composed_lat) != true)
		{
			LOG_WARN << "_new_kaldi_const_arpa_lmrescore.Compose failed!!!";
			return false;
		}
		return true;
	}
	template<class Arc>
	void ConvertLatticeToResult(const fst::Fst<Arc> &fst,
			size_t n, std::vector<std::string> *nbest_str)
	{
		n = n > 0 ? n : 1;
		std::vector<std::vector<int32> > alignments;
		std::vector<std::vector<int32> > words;
		std::vector<typename Arc::Weight> weights;
		kaldi::FstToNbestVector(fst, n, &alignments, &words, &weights);

		for(size_t i = 0 ; i < words.size(); ++i)
		{
			nbest_str->push_back(
					kaldi::ConvertWordId2Str(_word_syms, words[i]));
		}
	}
private:
	// recv data from request
	AsrReturnPackageAnalysis _ser_recv_package_analysis;
	// send data to request
	AsrReturnPackageAnalysis _ser_send_package_analysis;
	const AsrPostProcessWorkThreadOpt &_opts;

	kaldi::KaldiConstArpaLmRescore _old_kaldi_const_arpa_lmrescore;
	kaldi::KaldiConstArpaLmRescore _new_kaldi_const_arpa_lmrescore;
	const fst::SymbolTable &_word_syms;
}; // class AsrPostProcessWorkThread ok

#include "src/util/namespace-end.h"

#endif

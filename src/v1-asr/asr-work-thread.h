#ifndef __ASR_WORK_THREAD_H__
#define __ASR_WORK_THREAD_H__
#include "src/service2/thread-class.h"
#include "src/service2/thread-pool.h"
#include "src/v1-asr/asr-source.h"
#include "src/service2/net-data-package.h"

#include "src/util/namespace-start.h"

class ASRWorkThread:public ThreadBase
{
public:
	friend class ASRServiceTask;
	typedef ThreadBase::int32 int32;
public:
	ASRWorkThread(ThreadPoolBase<ThreadBase> *tp, 
			kaldi::ASROpts *asr_opts, kaldi::ASRSource *asr_source):
		_thread_pool(tp),
		_asr_work(NULL)
	{ 
		InitASRSource(asr_opts, asr_source);
	}
	~ASRWorkThread() 
	{ 
		if(_asr_work != NULL)
			delete _asr_work;
		_asr_work = NULL;
		_thread_pool = NULL;
	}
	void Run();
private:
	bool InitASRSource(kaldi::ASROpts *asr_opts, kaldi::ASRSource *asr_source);
	ThreadPoolBase<ThreadBase> *_thread_pool;
	// recv data from client
	C2SPackageAnalysis _ser_c2s_package_analysys;
	// send data to client
	S2CPackageAnalysis _ser_s2c_package_analysys;
	// feature
	// nnet
	// fst
	kaldi::ASRWorker *_asr_work;
};

#include "src/util/namespace-end.h"
#endif
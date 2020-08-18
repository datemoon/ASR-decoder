#ifndef __ASR_WORK_THREAD_H__
#define __ASR_WORK_THREAD_H__

#include "service2/thread-class.h"
#include "service2/thread-pool.h"

class ASRWorkThread:public ThreadBase
{
public:
	typedef ThreadBase::int32 int32;
public:
	ASRWorkThread(ThreadPoolBase<ThreadBase> *tp)
	{ 
		_thread_pool = tp;
	}
	~ASRWorkThread() { }
	void Run();
	int32 InitASRSource();
private:
	ThreadPoolBase<ThreadBase> *_thread_pool;
	// feature
	// nnet
	// fst
	
};

#endif

#ifndef __TEST_WORK_THREAD_H__
#define __TEST_WORK_THREAD_H__

#include "src/service2/thread-class.h"
#include "src/service2/thread-pool.h"

#ifdef NAMESPACE
namespace datemoon {
#endif
class TestWorkThread:public ThreadBase
{
public:
	typedef ThreadBase::int32 int32;
public:
	TestWorkThread(ThreadPoolBase<ThreadBase> *tp)
	{ 
		_thread_pool = tp;
	}
	~TestWorkThread() { }
	void Run();
	int32 InitASRSource();
private:
	ThreadPoolBase<ThreadBase> *_thread_pool;
	// feature
	// nnet
	// fst
	
};
#ifdef NAMESPACE
} // namespace datemoon
#endif
#endif

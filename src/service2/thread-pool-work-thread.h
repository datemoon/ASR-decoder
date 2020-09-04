#ifndef __THREAD_POOL_WORK_THREAD_H__
#define __THREAD_POOL_WORK_THREAD_H__
#include "src/service2/thread-class.h"
#include "src/service2/thread-pool.h"

#include "src/util/namespace-start.h"

/// This class for the convenience of using thread-pool.h
class ThreadPoolWorkThread:public ThreadBase, public ThreadTimeInfo
{
public:
	typedef ThreadBase::int32 int32;
public:
	ThreadPoolWorkThread(ThreadPoolBase<ThreadBase> *tp)
		:_thread_pool(tp) { }
	virtual ~ThreadPoolWorkThread() { _thread_pool = NULL; }
	virtual void Run()
	{
		pthread_t tid = GetThreadId();
		// thread synchronization lock
		pthread_mutex_t *pthread_pool_mutex = _thread_pool->GetPthreadMutex();
		// thread condition lock
		pthread_cond_t *pthread_pool_cond = _thread_pool->GetPthreadCond();
		while(1)
		{
			pthread_mutex_lock(pthread_pool_mutex);
			while(_thread_pool->GetTaskSize() == 0 && !_thread_pool->Shutdown())
			{
				// set thread ready ok, because pthread_cond_t can be loss,
				// make sure pthread_pool_cond be get when task arrive.
				if(_ready_ok == false)
				{// run ok
					_ready_ok = true;
				}
				LOG_COM << "Thread " << tid << " wait task.";
				pthread_cond_wait(pthread_pool_cond, pthread_pool_mutex);
			}
			if(_thread_pool->Shutdown())
			{
				pthread_mutex_unlock(pthread_pool_mutex);
				LOG_COM << "Thread " << tid << " will exit.";
				pthread_exit(NULL);
			}
			LOG_COM << "tid " << tid << " run.";
		
			TaskBase *task = _thread_pool->GetTask();		
			_thread_pool->MoveToBusy(tid); // add busy list
			pthread_mutex_unlock(pthread_pool_mutex);
			// process task logic in Run.
			if(0 != task->Run((void*)this))
			{
				LOG_WARN << "task run error!!!";
			}
			delete task;

			pthread_mutex_lock(pthread_pool_mutex);
			_thread_pool->MoveToIdle(tid); // add idle list
			pthread_mutex_unlock(pthread_pool_mutex);
		}
	}
	virtual void SetTime(double data_time, double work_time)
	{
		_data_time += data_time;
		_work_time += work_time;
		_thread_pool->SetTime(data_time, work_time);
	}

	virtual void TimeInfo() { WorkEfficiencyInfo(); }
private:
	void WorkEfficiencyInfo()
	{
		LOG_COM << "ThreadIndex = " << GetThreadIndex()
			<< " -> wav data time is : " << _data_time;
		LOG_COM << "ThreadIndex = " << GetThreadIndex()
			<< " -> work time is : " << _work_time;
		LOG_COM << "ThreadIndex = " << GetThreadIndex()
			<< " -> WorkEfficiencyInfo is rt : " << GetEfficiency();
	}
private:
	// use thread pool mutex
	ThreadPoolBase<ThreadBase> *_thread_pool;
};

#include "src/util/namespace-end.h"
#endif

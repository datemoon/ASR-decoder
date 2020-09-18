#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include <deque>
#include <string>
#include <vector>
#include <pthread.h>
#include "src/service2/thread-class.h"
#include "src/service2/thread-info.h"
#include "src/util/util-common.h"

#include "src/util/namespace-start.h"
class TaskBase
{
public:
	typedef int int32;
protected:
	std::string _task_name;
public:
	TaskBase(std::string task_name=std::string("")): 
		_task_name(task_name) { }
	virtual int32 Run(void *data) = 0;
	virtual int32 Stop() = 0;
	virtual void GetInfo() = 0;
	virtual ~TaskBase() { }
};

// T is work thread.
template<class T>
class ThreadPoolBase:public ThreadTimeInfo
{
public:
	typedef int int32;
	enum ThreadPoolRetVal
	{
		TPERROR = -1,
		TPOK = 0
	};
protected:
	std::deque<TaskBase *> _task_list; // task list
	bool _shutdown;                    // thread exit
	int32 _thread_num;                 // thread pool thread number.
	std::vector<T *> _all_threads; // all threads
	std::vector<pthread_t> _idle_pthread_id; // idle thread id
	std::vector<pthread_t> _busy_pthread_id; // busy thread id

	pthread_mutex_t _pthread_pool_mutex;  // thread synchronization lock
	pthread_cond_t _pthread_pool_cond;    // thread condition lock

	pthread_mutex_t _time_mutex;
	bool _wait_thread;

	void OneToTwo(pthread_t tid, std::vector<pthread_t> &one, std::vector<pthread_t> &two)
	{
		std::vector<pthread_t>::iterator iter = one.begin();
		while(iter != one.end())
		{
			if (tid == *iter)
				break;
			iter++;
		}
		one.erase(iter);
		two.push_back(tid);
	}
//	static void *ThreadFunc(void *thread_para); // thread callback function

public:
	// pthread move to idle list
	void MoveToBusy(pthread_t tid)
	{
		OneToTwo(tid, _idle_pthread_id, _busy_pthread_id);
	}
	// pthread move to busy list
	void MoveToIdle(pthread_t tid)
	{
		OneToTwo(tid, _busy_pthread_id, _idle_pthread_id);
	}
	// thread synchronization lock
	pthread_mutex_t *GetPthreadMutex()
	{
		return &_pthread_pool_mutex;
	}
	// thread condition lock
	pthread_cond_t *GetPthreadCond()
	{
		return &_pthread_pool_cond;
	}

	TaskBase *GetTask()
	{
		TaskBase *task = _task_list.front();
		_task_list.pop_front();
		return task;
	}

	bool Shutdown()
	{
		return _shutdown;
	}


	ThreadPoolBase(int32 thread_num=10,bool wait_thread=false);
	// release thread pool source
	~ThreadPoolBase();
	// add task
	int32 AddTask(TaskBase *task);
	// stop all thread
	int32 StopAll();
	int32 WaitStopAll();

	int32 GetTaskSize()
	{
		return _task_list.size();
	}
	
	// it's only at start use ,because isn't use mutex.
	int32 AddThread(T *th);

	int32 Init(std::vector<T *> &t_v);

	void Info();

	double GetDataTime() 
	{ 
		pthread_mutex_lock(&_time_mutex);
		double t = _data_time;
		pthread_mutex_unlock(&_time_mutex);
		return t;
	}
	double GetWorkTime() 
	{ 
		pthread_mutex_lock(&_time_mutex);
		double t = _work_time;
		pthread_mutex_unlock(&_time_mutex);
		return t;
	}
	virtual void SetTime(double data_time, double work_time)
	{
		pthread_mutex_lock(&_time_mutex);
		_data_time += data_time;
		_work_time += work_time;
		pthread_mutex_unlock(&_time_mutex);
	}
	
	void TimeInfo()
	{
		pthread_mutex_lock(&_time_mutex);
		LOG_COM << "Total data time is : " << _data_time;
		LOG_COM << "Total work time is : " << _work_time;
		LOG_COM << "Work rt is : " << _work_time/_data_time;
		pthread_mutex_unlock(&_time_mutex);
	}
};
#include "src/util/namespace-end.h"

#include "src/service2/thread-pool-inl.h"
//template class ThreadPoolBase<ThreadBase>;

#endif

#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include <deque>
#include <string>
#include <vector>
#include <pthread.h>

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

class ThreadPoolBase
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
	std::vector<pthread_t> _pthread_id; // all thread id
	std::vector<pthread_t> _idle_pthread_id; // idle thread id
	std::vector<pthread_t> _busy_pthread_id; // busy thread id

	pthread_mutex_t _pthread_pool_mutex;  // thread synchronization lock
	pthread_cond_t _pthread_pool_cond;    // thread condition lock

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
	static void *ThreadFunc(void *thread_para); // thread callback function

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


	ThreadPoolBase(int32 thread_num=10);
	// release thread pool source
	~ThreadPoolBase();
	// add task
	int32 AddTask(TaskBase *task);
	// stop all thread
	int32 StopAll();

	int32 GetTaskSize()
	{
		return _task_list.size();
	}
};

#endif

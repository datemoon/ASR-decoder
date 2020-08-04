#include "service2/thread-pool.h"
#include "util/log-message.h"

template<class T>
ThreadPoolBase<T>::ThreadPoolBase(int32 thread_num):
	_shutdown(false),
	_thread_num(thread_num)
{
	if(pthread_mutex_init(&_pthread_pool_mutex, NULL) != 0)
	{
		LOG_ERR << "pthread_mutex_init failed.";
		return ;
	}
	if(pthread_cond_init(&_pthread_pool_cond, NULL) != 0)
	{
		LOG_ERR << "pthread_cond_init failed.";
		return ;
	}
}

template<class T>
typename ThreadPoolBase<T>::int32 ThreadPoolBase<T>::AddThread(T *th)
{
	_all_threads.push_back(th);
	_idle_pthread_id.push_back(th->GetThreadId());
	return _all_threads.size();
}

template<class T>
typename ThreadPoolBase<T>::int32 ThreadPoolBase<T>::Init(std::vector<T *> &t_v)
{
	for(int i=0; i<t_v.size(); ++i)
	{
		int ret = AddThread(t_v[i]);
		if(ret != i+1)
		{
			LOG_ERR << "Init thread pool error!!!";
			return TPERROR;
		}
	}
	return TPOK;
}

template<class T>
ThreadPoolBase<T>::~ThreadPoolBase()
{
	if(pthread_mutex_destroy(&_pthread_pool_mutex) != 0)
	{
		LOG_WARN << "pthread_mutex_destroy failed.";
		return;
	}
	if(pthread_cond_destroy(&_pthread_pool_cond) != 0)
	{
		LOG_WARN << "pthread_cond_destroy failed.";
		return;
	}
}

template<class T>
typename ThreadPoolBase<T>::int32 ThreadPoolBase<T>::AddTask(TaskBase *task)
{
	// judge queue full or not and process
	pthread_mutex_lock(&_pthread_pool_mutex);
	if(_idle_pthread_id.size() != 0)
	{ // have idle pthread
		_task_list.push_back(task);
	}
	else
	{
		LOG_WARN << "No idle pthread wait...";
		task->Stop();
		// task cancel
	}
	pthread_mutex_unlock(&_pthread_pool_mutex);

	pthread_cond_signal(&_pthread_pool_cond);
	return 0;
}

template<class T>
typename ThreadPoolBase<T>::int32 ThreadPoolBase<T>::StopAll()
{
	if(_shutdown)
		return TPERROR;
	LOG << "*****************************\n"
		<< "Now I will end all threads!!"
		<< "*****************************";
	// wake up all thread distory thread pool
	_shutdown = true;
	pthread_cond_broadcast(&_pthread_pool_cond);
	for(int32 i =0 ; i< _thread_num ; ++i)
	{
		pthread_join(_all_threads[i]->GetThreadId(), NULL);
	}

	pthread_mutex_destroy(&_pthread_pool_mutex);
	pthread_cond_destroy(&_pthread_pool_cond);
	LOG << "*****************************\n"
		<< "End all threads ok!!"
		<< "*****************************";
	return TPOK;
}

/*
template<class T>
void *ThreadPoolBase<T>::ThreadFunc(void *thread_para)
{
	pthread_t tid = pthread_self();

	ThreadPoolBase *thread_pool = static_cast<ThreadPoolBase*>(thread_para);
	// thread synchronization lock
	pthread_mutex_t *pthread_pool_mutex = thread_pool->GetPthreadMutex();
	// thread condition lock
	pthread_cond_t *pthread_pool_cond = thread_pool->GetPthreadCond(); 
	while(true)
	{
		pthread_mutex_lock(pthread_pool_mutex);
		while(thread_pool->GetTaskSize() == 0 && !thread_pool->Shutdown())
		{
			LOG << "Thread " << tid << " wait task.";
			pthread_cond_wait(pthread_pool_cond, pthread_pool_mutex);
		}
		if(thread_pool->Shutdown())
		{
			pthread_mutex_unlock(pthread_pool_mutex);
			LOG << "Thread " << tid << " will exit.";
			pthread_exit(NULL);
		}
		LOG << "tid " << tid << " run.";

		TaskBase *task = thread_pool->GetTask();

		thread_pool->MoveToBusy(tid); // add busy list
		pthread_mutex_unlock(pthread_pool_mutex);
	
		task->Run(NULL);

		pthread_mutex_lock(pthread_pool_mutex);
		thread_pool->MoveToIdle(tid); // add idle list
		pthread_mutex_unlock(pthread_pool_mutex);
	}
}
*/

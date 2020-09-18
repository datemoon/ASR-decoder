#include <unistd.h>
#include "src/service2/thread-pool.h"
#include "src/util/log-message.h"

#include "src/util/namespace-start.h"

template<class T>
ThreadPoolBase<T>::ThreadPoolBase(int32 thread_num, bool wait_thread):
	_shutdown(false),
	_thread_num(thread_num),
	_wait_thread(wait_thread)
{
	if(pthread_mutex_init(&_pthread_pool_mutex, NULL) != 0)
	{
		LOG_ERR << "pthread_mutex_init _pthread_pool_mutex failed.";
		return ;
	}
	if(pthread_cond_init(&_pthread_pool_cond, NULL) != 0)
	{
		LOG_ERR << "pthread_cond_init _pthread_pool_cond failed.";
		return ;
	}

	if(pthread_mutex_init(&_time_mutex, NULL) != 0)
	{
		LOG_ERR << "pthread_mutex_init _time_mutex failed.";
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
	for(size_t i=0; i < t_v.size(); ++i)
	{
		t_v[i]->Create();
		int ret = AddThread(t_v[i]);
		if(ret != i+1)
		{
			LOG_ERR << "Init thread pool error!!!";
			return TPERROR;
		}
	}

	for(int i=0; i<_all_threads.size(); ++i)
	{
		LOG_COM << "Recv ready cond start." << i;
		while(true)
		{
			pthread_mutex_lock(&_pthread_pool_mutex);
			if(_all_threads[i]->IsReady() == false)
			{
				pthread_mutex_unlock(&_pthread_pool_mutex);
				//LOG_COM << "Wait thread " << _all_threads[i]->GetThreadId() << " ready ok.";
				usleep(1000);
			}
			else
			{
				pthread_mutex_unlock(&_pthread_pool_mutex);
				break;
			}
		}
		LOG_COM << "Recv ready cond ok." << i;
	}
	LOG_COM << "Init thread pool ok.";
	return TPOK;
}

template<class T>
ThreadPoolBase<T>::~ThreadPoolBase()
{
	StopAll();
	/*
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
	*/
}

template<class T>
typename ThreadPoolBase<T>::int32 ThreadPoolBase<T>::AddTask(TaskBase *task)
{
	int ntimes = 0;
	while(1)
	{
		// judge queue full or not and process
		pthread_mutex_lock(&_pthread_pool_mutex);
		if(_idle_pthread_id.size() != 0 || _wait_thread == true)
		{ // have idle pthread
			_task_list.push_back(task);
			pthread_cond_signal(&_pthread_pool_cond);
			pthread_mutex_unlock(&_pthread_pool_mutex);
			break;
		}
		else
		{
			pthread_mutex_unlock(&_pthread_pool_mutex);
			if(ntimes < 3)
			{
				ntimes ++;
				usleep(10*1000); // 10ms
				continue;
			}
			LOG_WARN <<  "Wait " << ntimes << " times and timeout. No idle pthread wait...";
			task->Stop();
			return -1;
			// task cancel
		}
	}

	return 0;
}

template<class T>
typename ThreadPoolBase<T>::int32 ThreadPoolBase<T>::WaitStopAll()
{
	if(_shutdown)
		return TPERROR;
	while(1)
	{
		pthread_mutex_lock(&_pthread_pool_mutex);
		if(_task_list.size() == 0)
		{
			pthread_mutex_unlock(&_pthread_pool_mutex);
			return StopAll();
		}
		pthread_mutex_unlock(&_pthread_pool_mutex);
		usleep(5000);// sleep 0.5s
	}
}
template<class T>
typename ThreadPoolBase<T>::int32 ThreadPoolBase<T>::StopAll()
{
	if(_shutdown)
		return TPERROR;
	LOG_COM << "*****************************";
	LOG_COM << "Now I will end all threads!!";
	LOG_COM << "*****************************";
	// wake up all thread distory thread pool
	pthread_mutex_lock(&_pthread_pool_mutex);
	_shutdown = true;
	pthread_cond_broadcast(&_pthread_pool_cond);
	pthread_mutex_unlock(&_pthread_pool_mutex);
	LOG_COM << "_shutdown set true.";
	for(size_t i =0 ; i< _all_threads.size() ; ++i)
	{
		LOG_COM << "thread " << _all_threads[i]->GetThreadId() << " : " << i;
		pthread_join(_all_threads[i]->GetThreadId(), NULL);
	}

	pthread_mutex_destroy(&_pthread_pool_mutex);
	pthread_cond_destroy(&_pthread_pool_cond);
	for(size_t i = 0 ; i < _all_threads.size(); ++i)
	{
		delete _all_threads[i];
	}
	LOG_COM << "*****************************";
	LOG_COM << "End all threads ok!!";
	LOG_COM << "*****************************";
	return TPOK;
}

template<class T>
void ThreadPoolBase<T>::Info()
{
	pthread_mutex_lock(&_pthread_pool_mutex);
	LOG_COM << "******************Info start*****************";
	LOG_COM << "busy thread number: " << _busy_pthread_id.size();
	for(size_t i=0;i<_busy_pthread_id.size();++i)
	{
		LOG_COM << "thread id : " << _busy_pthread_id[i];
	}
	pthread_mutex_unlock(&_pthread_pool_mutex);

	TimeInfo();
	LOG_COM << "******************Info end*******************";
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
			LOG_COM << "Thread " << tid << " wait task.";
			pthread_cond_wait(pthread_pool_cond, pthread_pool_mutex);
		}
		if(thread_pool->Shutdown())
		{
			pthread_mutex_unlock(pthread_pool_mutex);
			LOG_COM << "Thread " << tid << " will exit.";
			pthread_exit(NULL);
		}
		LOG_COM << "tid " << tid << " run.";

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
#include "src/util/namespace-end.h"

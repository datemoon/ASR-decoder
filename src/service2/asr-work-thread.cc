#include "src/util/log-message.h"
#include "src/service2/asr-work-thread.h"

#include "src/util/namespace-start.h"

void ASRWorkThread::Run()
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

		task->Run((void*)this);

		pthread_mutex_lock(pthread_pool_mutex);
		_thread_pool->MoveToIdle(tid); // add idle list
		pthread_mutex_unlock(pthread_pool_mutex);
	}
}

#include "src/util/namespace-end.h"

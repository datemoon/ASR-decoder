#include "service/thread-pool.h"
#include "util/log-message.h"

ThreadPoolBase::ThreadPoolBase(ThreadPoolBase::int32 thread_num):
	_shutdown(false),
	_thread_num(thread_num),
	_pthread_id(thread_num),
	_idle_pthread_id(thread_num)
{
	if(pthread_mutex_init(&_pthread_pool_mutex, NULL) != 0)
	{
		LOG_WARN << "pthread_mutex_init failed.";
		return ;
	}
	if(pthread_cond_init(&_pthread_pool_cond, NULL) != 0)
	{
		LOG_WARN << "pthread_cond_init failed.";
		return ;
	}
	for(int32 i=0; i<_thread_num;++i)
	{
		pthread_create(&(_pthread_id[i]), NULL, ThreadFunc, (void*)this);
		_idle_pthread_id[i] = _pthread_id[i];
	}
}
ThreadPoolBase::~ThreadPoolBase()
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
ThreadPoolBase::int32 ThreadPoolBase::AddTask(TaskBase *task)
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
		// task cancel
	}
	pthread_mutex_unlock(&_pthread_pool_mutex);

	pthread_cond_signal(&_pthread_pool_cond);
	return 0;
}

ThreadPoolBase::int32 ThreadPoolBase::StopAll()
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
		pthread_join(_pthread_id[i], NULL);
	}

	pthread_mutex_destroy(&_pthread_pool_mutex);
	pthread_cond_destroy(&_pthread_pool_cond);
	LOG << "*****************************\n"
		<< "End all threads ok!!"
		<< "*****************************";
	return TPOK;
}

void *ThreadPoolBase::ThreadFunc(void *thread_para)
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

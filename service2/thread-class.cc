#include "service2/thread-class.h"
#include "util/log-message.h"

void* ThreadBase::ThreadFunc(void *para)
{
	ThreadBase *th = static_cast<ThreadBase *>(para);
	th->Run();
	return NULL;
}

ThreadBase::int32 ThreadBase::Join(void **ret)
{
	return pthread_join(_thread_id, ret);
}

ThreadBase::int32 ThreadBase::Create()
{
	if(pthread_create(&_thread_id ,NULL ,&ThreadBase::ThreadFunc ,(void*)this) != 0)
	{
		LOG << "Create pthread failed!!!";
		return TERROR;
	}
	return TOK;
}

ThreadBase::int32 ThreadBase::GetThreadId() const
{
	return _thread_id;
}

ThreadBase::int32 ThreadBase::GetThreadIndex() const
{
	return _index;
}

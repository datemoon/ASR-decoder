#ifndef __THREAD_CALSS_H__
#define __THREAD_CALSS_H__

#include <pthread.h>

class ThreadBase
{
public:
	typedef int int32;
	enum ThreadRetVal
	{
		TERROR = -1,
		TOK = 0
	};
public:
	ThreadBase(){}
	ThreadBase(int32 index):_index(index){ }

	virtual ~ThreadBase() {}
	static void* ThreadFunc(void* para); // thread function
	virtual void Run() = 0;              // real process data 
	int32 Create();                      // create thread
	int32 Join(void **ret);              // join thread
	int32 GetThreadId() const;           // get pthread id
	int32 GetThreadIndex() const;        // get pthread index
private:
	pthread_t _thread_id;
	int32 _index;
};

#endif

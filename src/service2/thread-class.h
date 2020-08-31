#ifndef __THREAD_CALSS_H__
#define __THREAD_CALSS_H__

#include <pthread.h>

#include "src/util/namespace-start.h"

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
	ThreadBase():_thread_id(0),_index(-1),_ready_ok(false){}
	ThreadBase(int32 index):_thread_id(0),_index(index),_ready_ok(false){ }

	virtual ~ThreadBase() {}
	static void* ThreadFunc(void* para); // thread function
	virtual void Run() = 0;              // real process data 
	int32 Create();                      // create thread
	int32 Join(void **ret);              // join thread
	pthread_t GetThreadId() const;           // get pthread id
	int32 GetThreadIndex() const;        // get pthread index
	bool IsReady() { return _ready_ok;}
	void Reset();
protected:
	pthread_t _thread_id;
	int32 _index;
	bool _ready_ok;
};

#include "src/util/namespace-end.h"

#endif

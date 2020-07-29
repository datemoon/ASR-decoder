#ifndef __THREAD_CALSS_H__
#define __THREAD_CALSS_H__



class ThreadClass
{
public:
	ThreadClass(){}
	ThreadClass(int id):_id(id){ }

	virtual ~CThread() {}
	static void* ThreadFunc(void* para);
	int Run();
	int Join();
	int GetThreadId() const;
	int Start();
private:
	pthread_t _thread_id;
	int _id;
};



#endif

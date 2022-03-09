#ifndef __CLIENT_ASR_POST_PROCESS_WORK_THREAD_H__
#define __CLIENT_ASR_POST_PROCESS_WORK_THREAD_H__

#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include "src/service2/thread-pool.h"
#include "src/service2/thread-pool-work-thread.h"

#include "src/util/namespace-start.h"

class AsrPostProcessClientWorkThread: public ThreadPoolWorkThread
{
public:
	template<class Arc> friend class AsrPostProcessClinetTask;
	typedef ThreadBase::int32 int32;

	AsrPostProcessClientWorkThread(ThreadPoolBase<ThreadBase> *thread_pool,
			int32 port=8000,
			std::string ip="127.0.0.1"):
		ThreadPoolWorkThread(thread_pool)
	{
		memset(&_ser, 0x00, sizeof(_ser));
		_ser.sin_family = AF_INET;
		inet_aton(ip.c_str(), &_ser.sin_addr);
		_ser.sin_port = htons(port);
	}

	~AsrPostProcessClientWorkThread() { }
private:
	struct sockaddr_in _ser;

};

#include "src/util/namespace-end.h"

#endif

#ifndef __ASR_CLIENT_THREAD_H__
#define __ASR_CLIENT_THREAD_H__
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include "src/service2/thread-pool.h"
#include "src/service2/net-data-package.h"
#include "src/service2/thread-pool-work-thread.h"

#include "src/util/namespace-start.h"

/// This is class use kaldi feature , nnet and mydecoder 
/// package asr service class work thread.
class ASRClinetThread:public ThreadPoolWorkThread
{
public:
	friend class ASRClientTask;
	typedef ThreadBase::int32 int32;
public:
	// create connect.
	ASRClinetThread(ThreadPoolBase<ThreadBase> *thread_pool,
			int32 port=8000,
			std::string ip="127.0.0.1")
		:ThreadPoolWorkThread(thread_pool), _sockfd(-1)
	{
		_sockfd = socket(AF_INET, SOCK_STREAM, 0);
		struct sockaddr_in ser;
		memset(&ser, 0x00, sizeof(ser));
		ser.sin_family = AF_INET;
		inet_aton(ip.c_str(), &ser.sin_addr);
		ser.sin_port = htons(port);
		int res = connect(_sockfd, (struct sockaddr *) &ser, sizeof(ser));
		assert(res >=0);
	}

	~ASRClinetThread()
	{
	   close(_sockfd);
	}
private:
	// recv data from service
	C2SPackageAnalysis _client_c2s_package_analysys;
	// send data to service
	S2CPackageAnalysis _client_s2c_package_analysys;

	int _sockfd;
};

#include "src/util/namespace-end.h"
#endif

#ifndef __ASR_CLIENT_TASK_H__
#define __ASR_CLIENT_TASK_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <signal.h>
#include "src/v2-asr/asr-client-thread.h"
#include "src/util/log-message.h"

#include "src/util/namespace-start.h"

bool CreateResultHandle(std::string result_file = "result.txt", std::string mode = "a");

void CloseResultHandle();

class ASRClientTask:public TaskBase
{
public:
	typedef TaskBase::int32 int32;
public:
	ASRClientTask(std::string wav_file, std::string task_name=""):
		TaskBase(task_name), _wav_file(wav_file), _sockfd(-1) {}

	int32 Run(void *data);

	int32 Stop()
	{
		return 0;
	}
	void GetInfo()
	{
		LOG_COM << "Task name is : " << _task_name ;
		LOG_COM << "wav file is : " << _wav_file;
	}

private:
	static pthread_mutex_t _outfile_mutex;
	static void *RecvThreadFunc(void *data);

	std::string _wav_file;
	int _sockfd;
};
#include "src/util/namespace-end.h"
#endif

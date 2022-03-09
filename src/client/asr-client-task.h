#ifndef __ASR_CLIENT_TASK_H__
#define __ASR_CLIENT_TASK_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>

#include "src/client/asr-client-thread.h"
#include "src/util/log-message.h"

#include "src/util/namespace-start.h"

bool CreateResultHandle(std::string result_file = "result.txt", 
		std::string mode = "a", std::string align_file = "align.txt");

void CloseResultHandle();

struct ASRClientOpts
{
public:
	ASRClientOpts(bool realtime_res=false,
			int32 nbest=0,
			bool do_align=false,
			bool do_lattice=false): 
		_realtime_res(realtime_res), _nbest(nbest), 
		_do_align(do_align), _do_lattice(do_lattice) { }

	void Check()
	{
		if(_realtime_res == true)
			if (_nbest <= 0)
				_nbest = 1;
	}

	bool _realtime_res;
	int32 _nbest;
	bool _do_align;
	bool _do_lattice;

};

class ASRClientTask:public TaskBase
{
public:
	typedef TaskBase::int32 int32;
public:
	ASRClientTask(const ASRClientOpts &opts,
			std::string wav_file,
			int32 wavhead_len=0,
			int32 send_len = 16000*2,
			std::string task_name=""):
		TaskBase(task_name), _wav_file(wav_file),
	   	_wavhead_len(wavhead_len), _send_len(send_len), 
		_sockfd(-1), _cli_opts(opts) {}

	~ASRClientTask()
	{
		Stop();
	}
	int32 Run(void *data);

	int32 Stop()
	{
		if(_sockfd != -1)
		{
			close(_sockfd);
			_sockfd = -1;
		}
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
	int32 _wavhead_len;
	int32 _send_len;
	int _sockfd;
	const ASRClientOpts &_cli_opts;
};
#include "src/util/namespace-end.h"
#endif

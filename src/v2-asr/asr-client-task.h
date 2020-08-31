#ifndef __ASR_CLIENT_TASK_H__
#define __ASR_CLIENT_TASK_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <signal.h>
#include "src/v2-asr/asr-client-thread.h"
#include "src/util/log-message.h"

#include "src/util/namespace-start.h"

class ASRClientTask:public TaskBase
{
public:
	typedef TaskBase::int32 int32;
public:
	ASRClientTask(std::string wav_file, std::string task_name=""):
		TaskBase(task_name), _wav_file(wav_file) {}

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
	std::string _wav_file;
};
void *RecvThreadFunc(void *connect_fd)
{
	int sockfd = *(int*)connect_fd;
	S2CPackageAnalysis s2c_cli;
	while(1)
	{
		if(true != s2c_cli.S2CRead(sockfd))
		{
			LOG_WARN << "S2CRead failed!!!";
			return NULL;
		}
		s2c_cli.Print("s2c_cli");
		if(true == s2c_cli.IsAllEnd())
		{
			return NULL;
		}
	}
}
#define LEN 4096

int32 ASRClientTask::Run(void *data)
{
	signal(SIGPIPE, SIG_IGN); // ignore SIGPIPE to avoid crashing when socket forcefully disconnected
	ASRClinetThread * asr_client_thread = static_cast<ASRClinetThread *>(data);
	C2SPackageAnalysis cli_c2s;

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in &ser = asr_client_thread->_ser;
	int res = connect(sockfd, (struct sockaddr *) &ser, sizeof(struct sockaddr));
	if(res < 0)
	{
		LOG_WARN << "Client connect service failed!!!";
		return -1;
	}

	// create recv thread
	pthread_t thread_id;
	if(pthread_create(&thread_id ,NULL ,&RecvThreadFunc ,(void*)&sockfd) != 0)
	{
		LOG_WARN << "Create RecvThreadFunc thread failed!!!";
		return -1;
	}

	//size_t chunk=0;
	int n=0; // read timeout times
	int total_wav_len = 0;
	float total_decoder_time = 0.0;
	float wav_time = 0.0;
	struct timeval decoder_start, decoder_end;
	FILE *fp = fopen(_wav_file.c_str(), "r");
	if(fp == NULL)
	{
		LOG_WARN << "Open wav : "  << _wav_file << " failed!!!";
		return -1;
	}
	gettimeofday(&decoder_start, NULL);
	while(1)
	{
		char sentbuf[LEN];
		char recvbuf[LEN];
		memset(sentbuf,0x00,sizeof(sentbuf));
		memset(recvbuf,0x00,sizeof(recvbuf));

		size_t sent_len = fread((void*)sentbuf, 1, LEN, fp);
		total_wav_len += sent_len;
		if(sent_len > 0)
		{
			cli_c2s.SetNbest(1);
			if(true != cli_c2s.C2SWrite(sockfd, sentbuf, sent_len, 0))
			{
				LOG_WARN << "C2SWrite failed.";
				return -1;
			}
			cli_c2s.Print("cli_c2s");
		}
		else
		{
			cli_c2s.SetNbest(5);
			if(true != cli_c2s.C2SWrite(sockfd, sentbuf, sent_len, 1))
			{
				LOG_WARN << "C2SWrite end failed.";
				return -1;
			}
			cli_c2s.Print("cli_c2s");		
			uint smaple_rate = 0;
			if(cli_c2s.GetSampleRate() == C2SPackageAnalysis::K_16)
				smaple_rate=16000;
			else if(cli_c2s.GetSampleRate() == C2SPackageAnalysis::K_8)
				smaple_rate=8000;
			else if(cli_c2s.GetSampleRate() == C2SPackageAnalysis::K_32)
				smaple_rate=32000;
			else
				break;
			uint dtype_len = cli_c2s.GetDtypeLen();
			wav_time = total_wav_len*1.0/(smaple_rate*dtype_len);
			break; // end
		}
	}
	fclose(fp);
	LOG_COM << "Send wav: " << _wav_file << " ok.";
	pthread_join(thread_id,NULL);
	close(sockfd);
	LOG_COM << "Recv wav: " << _wav_file << " result and colse scoket " << sockfd << " ok.";
	gettimeofday(&decoder_end, NULL);
	total_decoder_time = (decoder_end.tv_sec- decoder_start.tv_sec) +
		(decoder_end.tv_usec - decoder_start.tv_usec)*1.0/1000000;

	float rt = total_decoder_time/wav_time;

	LOG_COM << "wav time(s)\t:" << wav_time;
	LOG_COM << "run time(s)\t:" << total_decoder_time;
	LOG_COM << "decoder rt\t: " << rt ;
	return 0;
}

#include "src/util/namespace-end.h"
#endif

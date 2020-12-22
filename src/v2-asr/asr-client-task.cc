
#include "src/v2-asr/asr-client-task.h"

#include "src/util/namespace-start.h"

FILE *_result_fp = NULL;
std::string _result_file("result.txt");

bool CreateResultHandle(std::string result_file, std::string mode)
{
	_result_file = result_file;

	_result_fp = fopen(_result_file.c_str(), mode.c_str());

	if(_result_fp == NULL)
	{
		LOG_WARN << "fopen " << _result_file << " failed!!!";
		return false;
	}
	return true;
}

void CloseResultHandle()
{
	if(_result_fp != NULL)
		fclose(_result_fp);
}
pthread_mutex_t ASRClientTask::_outfile_mutex = PTHREAD_MUTEX_INITIALIZER;

// asynchronous receive result, and print in result file.
void *ASRClientTask::RecvThreadFunc(void *data)
{
	ASRClientTask *asrclient_task = (ASRClientTask*) data;

	pthread_mutex_lock(&_outfile_mutex);
	int sockfd = asrclient_task->_sockfd;
	std::string &filename = asrclient_task->_wav_file;
	pthread_mutex_unlock(&_outfile_mutex);

	S2CPackageAnalysis s2c_cli;
	std::vector<std::string> all_result;
	int nrecv = 0;
	while(1)
	{
		if(true != s2c_cli.S2CRead(sockfd))
		{
			LOG_WARN << "S2CRead failed!!! file -> " << filename << " sockfd is " << sockfd; 
			return NULL;
		}
		nrecv ++;
		s2c_cli.Print(filename);
		// middle result or end result.
		if( s2c_cli.IsMiddleEnd() == true || true == s2c_cli.IsAllEnd())
		{

			std::vector<std::string> nbest;
			s2c_cli.GetData(&nbest);
			// save result in all_result
			if(nbest.size() == 0)
				all_result.push_back("");
			else
				all_result.push_back(nbest[0]);
			if(true == s2c_cli.IsAllEnd())
			{ // true send end and receive the last result.
				FILE *fp = stdout;
				if(_result_fp != NULL)
				{
					fp = _result_fp;
				}
				pthread_mutex_lock(&_outfile_mutex);
				for(size_t i =0;i<all_result.size(); ++i)
				{
					fprintf(fp, "%s %ld %s\n", filename.c_str(), i,
							all_result[i].c_str());
				}
				pthread_mutex_unlock(&_outfile_mutex);
				return NULL;
			} // send end.
		}// is send middle end or send end.
	}// while(1) receive result
}

#define LEN (16000*10)

ASRClientTask::int32 ASRClientTask::Run(void *data)
{
	//signal(SIGPIPE, SIG_IGN); // ignore SIGPIPE to avoid crashing when socket forcefully disconnected
	ASRClinetThread * asr_client_thread = static_cast<ASRClinetThread *>(data);
	C2SPackageAnalysis cli_c2s;

	_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	// in centos signal(SIGPIPE, SIG_IGN); it still happends. so I set socket
//	int set_nosignal = 1;
//	if(setsockopt(_sockfd, SOL_SOCKET, MSG_NOSIGNAL, (void*)&set_nosignal, sizeof(int) ) != 0)
//	{
//		LOG_WARN << "set MSG_NOSIGNAL failed!!!";
//		LOG_WARN << "errno: " << errno;
//		return -1;
//	} // ENOPROTOOPT     92      Protocol not available

	struct sockaddr_in &ser = asr_client_thread->_ser;
	int res = connect(_sockfd, (struct sockaddr *) &ser, sizeof(struct sockaddr));
	if(res < 0)
	{
		LOG_WARN << "Client connect service failed!!!";
		return -1;
	}

	//size_t chunk=0;
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
	// create recv thread ,it must be create after connect.
	pthread_t thread_id;
	if(pthread_create(&thread_id ,NULL ,&RecvThreadFunc ,(void*)this) != 0)
	{
		LOG_WARN << "Create RecvThreadFunc thread failed!!!";
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
			cli_c2s.SetNbest(0);
			if(cli_c2s.GetN() == 0)
			{ // frist package and send key_string
				if(true != cli_c2s.C2SWrite(_sockfd, sentbuf, sent_len, 0, _wav_file))
				{
					LOG_WARN << "C2SWrite first failed.";
					break;
				}
			}
			else
			{
				if(true != cli_c2s.C2SWrite(_sockfd, sentbuf, sent_len, 0))
				{
					LOG_WARN << "C2SWrite failed.";
					break;
				}
			}
			cli_c2s.Print("cli_c2s");
		}
		else
		{
			cli_c2s.SetNbest(0);
			if(true != cli_c2s.C2SWrite(_sockfd, sentbuf, sent_len, 1))
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
	close(_sockfd);
	LOG_COM << "Recv wav: " << _wav_file << " result and colse scoket " << _sockfd << " ok.";
	gettimeofday(&decoder_end, NULL);
	total_decoder_time = (decoder_end.tv_sec- decoder_start.tv_sec) +
		(decoder_end.tv_usec - decoder_start.tv_usec)*1.0/1000000;

	float rt = total_decoder_time/wav_time;
	
	asr_client_thread->SetTime(wav_time, total_decoder_time);
	VLOG_COM(1) << "wav time(s)\t:" << wav_time;
	VLOG_COM(1) << "run time(s)\t:" << total_decoder_time;
	VLOG_COM(1) << "decoder rt\t: " << rt ;
	return 0;
}

#include "src/util/namespace-end.h"

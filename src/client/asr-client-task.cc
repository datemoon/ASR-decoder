
#include "src/client/asr-client-task.h"

#include "src/util/namespace-start.h"

FILE *_result_fp = NULL;
std::string _result_file("result.txt");
std::string _align_file("align.txt");
std::ofstream _align_stream;

bool CreateResultHandle(std::string result_file, std::string mode, std::string align_file)
{
	_result_file = result_file;

	_result_fp = fopen(_result_file.c_str(), mode.c_str());

	if(_result_fp == NULL)
	{
		LOG_WARN << "fopen " << _result_file << " failed!!!";
		return false;
	}
	_align_file = align_file;
	_align_stream.open(_align_file);
	if(_align_stream.is_open())
	{
		LOG_WARN << "open " << _align_file << " failed!!!";
		return false;
	}
	return true;
}

void CloseResultHandle()
{
	if(_result_fp != NULL)
		fclose(_result_fp);
	if(_align_stream.is_open())
		_align_stream.close();
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
//	std::vector<std::vector<std::string> > all_result;
	std::vector<std::string> all_result;
	int nrecv = 0;
	int nsentence = -1;
	while(1)
	{
		if(true != s2c_cli.S2CRead(sockfd))
		{
			LOG_WARN << "S2CRead failed!!! file -> " << filename << " sockfd is " << sockfd; 
			int *ret = new int;
			*ret = -1;
			return (void*)ret;
		}
		nrecv ++;
		s2c_cli.Print(filename);
		// middle result or end result.
		if( s2c_cli.IsMiddleEnd() == true || true == s2c_cli.IsAllEnd())
		{
			nsentence++;
			std::vector<std::string> nbest;
			s2c_cli.GetData(&nbest);
			const AlignTime &align = s2c_cli.GetAlign();
//			all_result.push_back(std::move(nbest));
			// save result in all_result
			if(nbest.size() == 0)
				all_result.push_back("");
			else
				all_result.push_back(nbest[0]);
			{// write align
				pthread_mutex_lock(&_outfile_mutex);
				if(_align_stream.is_open())
				{ // write
					_align_stream << filename << " " << nsentence << " ";
					PrintContainerType(_align_stream, align);
				}
				else
				{
					std::cout << filename << " " << nsentence << " ";
					PrintContainerType(std::cout, align);
				}
				pthread_mutex_unlock(&_outfile_mutex);
			}

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
//					std::vector<std::string> &cur_nbest = all_result[i];
//					for(size_t j=0;j<cur_nbest.size(); ++j)
//					{
//						fprintf(fp, "%s-%ld %ld %s\n", filename.c_str(), j, i,
//								cur_nbest[j].c_str());
//					}
				}
				pthread_mutex_unlock(&_outfile_mutex);
				return NULL;
			} // send end.
		}// is send middle end or send end.
	}// while(1) receive result
}

#define LEN (16000*2*20) // 20s

ASRClientTask::int32 ASRClientTask::Run(void *data)
{
	//signal(SIGPIPE, SIG_IGN); // ignore SIGPIPE to avoid crashing when socket forcefully disconnected
	ASRClinetThread * asr_client_thread = static_cast<ASRClinetThread *>(data);

	// in centos signal(SIGPIPE, SIG_IGN); it still happends. so I set socket
//	int set_nosignal = 1;
//	if(setsockopt(_sockfd, SOL_SOCKET, MSG_NOSIGNAL, (void*)&set_nosignal, sizeof(int) ) != 0)
//	{
//		LOG_WARN << "set MSG_NOSIGNAL failed!!!";
//		LOG_WARN << "errno: " << errno;
//		return -1;
//	} // ENOPROTOOPT     92      Protocol not available

	int times = 3;
	while(times > 0)
	{
		C2SPackageAnalysis cli_c2s;
		_sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if(false)
		{
			int keepalive=1;
			int keeptime=5;
			int keepinterval = 1;
			int keepcount = 3;
			if(setsockopt(_sockfd, SOL_SOCKET, SO_KEEPALIVE,
						(void*)&keepalive,sizeof(keepalive)) != 0)
			{
				LOG_WARN << "socket set keepalive failed!!!";
				return -1;
			}
			if(setsockopt(_sockfd, SOL_TCP, TCP_KEEPIDLE,
						(void*)&keeptime, sizeof(keeptime)) != 0)
			{
				LOG_WARN << "socket set TCP_KEEPIDLE failed!!!";
				return -1;
			}
			if(setsockopt(_sockfd, SOL_TCP, TCP_KEEPINTVL,
						(void*)&keepinterval, sizeof(keepinterval)) != 0)
			{
				LOG_WARN << "socket set TCP_KEEPINTVL failed!!!";
				return -1;
			}
			if(setsockopt(_sockfd, SOL_TCP, TCP_KEEPCNT,
						(void*)&keepcount, sizeof(keepcount)) != 0)
			{
				LOG_WARN << "socket set TCP_KEEPCNT failed!!!";
				return -1;
			}
			struct timeval rec_timeout;
			rec_timeout.tv_sec = 10;
			rec_timeout.tv_usec = 0;
			if (setsockopt(_sockfd, SOL_SOCKET, SO_RCVTIMEO,
						(void*)&rec_timeout, sizeof(rec_timeout)) != 0)
			{
				LOG_WARN << "socket set SO_RCVTIMEO failed!!!";
				return -1;
			}
		}
		struct sockaddr_in &ser = asr_client_thread->_ser;
		int res = connect(_sockfd, (struct sockaddr *) &ser, sizeof(struct sockaddr));
		if(res < 0)
		{
			LOG_WARN << "Client connect service failed!!! and try reconnect.";
			times--;
			usleep(500000);
			continue;
			//return -1;
		}
		//size_t chunk=0;
		int total_wav_len = 0;
		float total_decoder_time = 0.0;
		float wav_time = 0.0;
		struct timeval decoder_start, decoder_end;
		int socket_error = 0; // no error.
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
		char sentbuf[LEN];
		char recvbuf[LEN];
		size_t sent_len = fread((void*)sentbuf, 1, _wavhead_len, fp);
		uint smaple_rate = 0;
		if(cli_c2s.GetSampleRate() == C2SPackageAnalysis::K_16)
			smaple_rate=16000;
		else if(cli_c2s.GetSampleRate() == C2SPackageAnalysis::K_8)
			smaple_rate=8000;
		else if(cli_c2s.GetSampleRate() == C2SPackageAnalysis::K_32)
			smaple_rate=32000;
		else
			return -1;
		while(1)
		{
			memset(sentbuf,0x00,sizeof(sentbuf));
			memset(recvbuf,0x00,sizeof(recvbuf));
	
			if(_send_len < LEN)
				sent_len = fread((void*)sentbuf, 1, _send_len, fp);
			else
				sent_len = fread((void*)sentbuf, 1, LEN, fp);
			total_wav_len += sent_len;
			int32 end_flag = 0;
			if(feof(fp) != 0)
				end_flag = 1;
			//if(sent_len > 0)
			{
				if(_cli_opts._realtime_res == true || end_flag == 1)
				{
					if(_cli_opts._do_align)
						cli_c2s.SetAliInfo(1);
					if(_cli_opts._do_lattice)
						cli_c2s.SetLattice(1);
					cli_c2s.SetNbest(_cli_opts._nbest);
				}
				else
					cli_c2s.SetNbest(0);
				if(cli_c2s.GetN() == 0)
				{ // frist package and send key_string
					if(true != cli_c2s.C2SWrite(_sockfd, sentbuf, sent_len, end_flag, _wav_file))
					{
						LOG_WARN << "C2SWrite first failed.";
						socket_error = 1;
						break;
					}
				}
				else
				{
					if(true != cli_c2s.C2SWrite(_sockfd, sentbuf, sent_len, end_flag))
					{
						LOG_WARN << "C2SWrite failed.";
						socket_error = 1;
						break;
					}
				}
				//usleep(sent_len*1.0/2/smaple_rate*1e6/2);
				cli_c2s.Print("cli_c2s");
				//usleep(500000);
			}
			if(end_flag != 0)
			{ // 结束
			//	cli_c2s.SetNbest(0);
			//	if(true != cli_c2s.C2SWrite(_sockfd, sentbuf, sent_len, 1))
			//	{
			//		LOG_WARN << "C2SWrite end failed.";
			//		return -1;
			//	}
			//	cli_c2s.Print("cli_c2s");		
				uint dtype_len = cli_c2s.GetDtypeLen();
				wav_time = total_wav_len*1.0/(smaple_rate*dtype_len);
				break; // end
			}
		}
		fclose(fp);
		fp = NULL;
		LOG_COM << "Send wav: " << _wav_file << " ok.";
		int *ret = NULL;
		pthread_join(thread_id, (void**)&ret);
		if(ret != NULL)
		{
			socket_error = 1;
			delete ret;
			LOG_COM << "Recv wav: " << _wav_file << " failed!!! and resend";
		}
		times--;
		if(socket_error == 0)
			LOG_COM << "Recv wav: " << _wav_file << " result and colse scoket " << _sockfd << " ok.";
		else
			LOG_COM << "wav: " << _wav_file << " failed!!! close socket " << _sockfd ;
		Stop();
		if(socket_error != 0)
			continue;
		//close(_sockfd);
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
	LOG_COM << "send wav: " << _wav_file << " failed!!!";
	return -1;
}

#include "src/util/namespace-end.h"

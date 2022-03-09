#ifndef __CLIENT_ASR_POST_PROCESS_TASH_H__
#define __CLIENT_ASR_POST_PROCESS_TASH_H__

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <signal.h>
#include "src/post-processing-service/client-asr-post-process-work-thread.h"
#include "src/post-processing-service/post-package.h"
#include "src/util/log-message.h"

#include "src/util/namespace-start.h"

template<class Arc>
class AsrPostProcessClinetTask : public TaskBase
{
public:
	typedef TaskBase::int32 int32;

	// fst must be free
	AsrPostProcessClinetTask(fst::Fst<Arc> *fst, 
			pthread_mutex_t *outfile_mutex,
			uint binary = 1,
			FILE *result_fp = NULL,
			std::string task_name=""):
		TaskBase(task_name),
	   	_fst(fst),
		_binary(binary),
		_outfile_mutex(outfile_mutex), 
		_result_fp(result_fp) { }

	~AsrPostProcessClinetTask() 
	{
		VLOG_COM(1) << "delete AsrPostProcessClinetTask";
		delete _fst;
		_fst = NULL;
	}

	int32 Run(void *data)
	{
		AsrPostProcessClientWorkThread *asr_post_process_thread =
			static_cast<AsrPostProcessClientWorkThread*>(data);

		AsrReturnPackageAnalysis cli_send;

		_sockfd = socket(AF_INET, SOCK_STREAM, 0);
		struct sockaddr_in &ser = asr_post_process_thread->_ser;
		int res = connect(_sockfd, (struct sockaddr *) &ser, sizeof(struct sockaddr));
		if(res <0)
		{
			close(_sockfd);
			LOG_WARN << "Client connect service failed and close sockfd!!!";
			return -1;
		}
		pthread_t thread_id;
		if(pthread_create(&thread_id ,NULL ,
					&RecvThreadFunc ,
					(void*)this) != 0)
		{
			LOG_WARN << "Create RecvThreadFunc thread failed!!!";
			return -1;
		}

		struct timeval decoder_start, decoder_end;
		float total_decoder_time = 0.0;

		gettimeofday(&decoder_start, NULL);

		fst::VectorFst<Arc>* ifst = static_cast<fst::VectorFst<Arc>* >(_fst);
		cli_send.SetLattice(ifst, _binary);
		if(cli_send.SetDoRescore(1) != true)
		{
			LOG_WARN << "SetDoRescore failed!!!";
		}

		if(cli_send.SetLatticeNbest(5) != true)
		{
			LOG_WARN << "SetLatticeNbest failed!!!";
		}

		if(cli_send.Write(_sockfd, S2CEND) < 0)
		{
			LOG_WARN << "ser.Write failed!!!";
			return -1;
		}
#ifdef DEBUG
		cli_send.Print(_task_name);
#endif
		LOG_COM << "Send lattice " << _task_name << " ok.";
		pthread_join(thread_id,NULL);
		close(_sockfd);
		LOG_COM << "Recv post process " << _task_name 
			<< " result and colse scoket " << _sockfd << " ok.";
		gettimeofday(&decoder_end, NULL);
		total_decoder_time = (decoder_end.tv_sec- decoder_start.tv_sec) +
			(decoder_end.tv_usec - decoder_start.tv_usec)*1.0/1000000;

		float rt = total_decoder_time;
		BaseFloat ntimes = 1;
		asr_post_process_thread->SetTime(ntimes, total_decoder_time);

		VLOG_COM(1) << "ntimes\t:" << ntimes;
		VLOG_COM(1) << "run time(s)\t:" << total_decoder_time;
		VLOG_COM(1) << "decoder rt\t: " << rt ;
		return 0;
	}

	int32 Stop()
	{
		return 0;
	}

	void GetInfo()
	{
		LOG_COM << "Task name is : " << _task_name ;
	}


private:


	// asynchronous receive result, and print in result file.
	static void *RecvThreadFunc(void *data)
	{
		AsrPostProcessClinetTask<Arc> *cli_task = 
			static_cast<AsrPostProcessClinetTask<Arc>*>(data);
		pthread_mutex_t *outfile_mutex = cli_task->_outfile_mutex;
		FILE *result_fp = cli_task->_result_fp;
		
		pthread_mutex_lock(outfile_mutex);
		int sockfd = cli_task->_sockfd;
		std::string key = cli_task->_task_name;
		pthread_mutex_unlock(outfile_mutex);

		AsrReturnPackageAnalysis cli_recv;
		std::vector<std::string> all_result;
		int nrecv = 0;

		while(1)
		{
			if(cli_recv.Read(sockfd) < 0)
			{
				LOG_WARN << "client receive data failed!!! file -> " << key 
					<< " sockfd is " << sockfd;
				return NULL;
			}
			nrecv++;
#ifdef DEBUG
			cli_recv.Print(key);
#endif
			if(cli_recv.IsMiddleEnd() == true || true == cli_recv.IsAllEnd())
			{
				std::vector<std::string> nbest;
				kaldi::CompactLattice *clat = NULL;
				kaldi::Lattice *lat = NULL;
				cli_recv.GetData(&nbest, clat, lat);

				if(nbest.size() == 0)
					all_result.push_back("");
				else
					all_result.push_back(nbest[0]);

				if(true == cli_recv.IsAllEnd())
				{ // true send end and receive the last result.
					FILE *fp = stdout;
					if(result_fp != NULL)
					{
						fp = result_fp;
					}
					pthread_mutex_lock(outfile_mutex);
					for(size_t i =0;i<all_result.size(); ++i)
					{
						fprintf(fp, "%s %ld %s\n", key.c_str(), i,
								all_result[i].c_str());
					}
					pthread_mutex_unlock(outfile_mutex);
					return NULL;
				} // send end.
			} // 
			break;
		} // while(1) receive result
		return NULL;
	}
private:
	fst::Fst<Arc> *_fst;
	uint _binary;
	pthread_mutex_t *_outfile_mutex;
	FILE *_result_fp;
	int _sockfd;
};

#include "src/util/namespace-end.h"

#endif

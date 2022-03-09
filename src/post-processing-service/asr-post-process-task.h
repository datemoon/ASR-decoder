#ifndef __ASR_POST_PROCESS_TASH_H__
#define __ASR_POST_PROCESS_TASH_H__

#include <sys/types.h>
#include <sys/socket.h>

#include "src/post-processing-service/lattice-to-result.h"

#include "src/post-processing-service/asr-post-process-work-thread.h"

#include "src/util/namespace-start.h"

// 这是一个后处理服务的任务，完成后处理相关任务：
// 如 二遍，加标点...
class AsrPostProcessServiceTask:public TaskBase
{
public:
	typedef TaskBase::int32 int32;
	AsrPostProcessServiceTask(int32 connfd, std::string task_name = ""):
		TaskBase(task_name), _connfd(connfd) { }

	~AsrPostProcessServiceTask() 
	{
		Stop();
	}

	int32 Run(void *data)
	{
		AsrPostProcessWorkThread *asr_post_porcess_work_thread =
			static_cast<AsrPostProcessWorkThread *> (data);

		AsrReturnPackageAnalysis &ser_recv = 
			asr_post_porcess_work_thread->_ser_recv_package_analysis;
		// send data to request
		AsrReturnPackageAnalysis &ser_send =
			asr_post_porcess_work_thread->_ser_send_package_analysis;

		ser_recv.Reset();
		ser_send.Reset();

		int32 n = 0;
		// calculate time
		double total_decoder_time = 0;
		Time keep_time;
		keep_time.Esapsed();
		while(1)
		{
			if(ser_recv.Read(_connfd) < 0)
			{
				if(errno == EAGAIN || errno == EINPROGRESS)
				{
					LOG_COM << "|" << _connfd << "| timeout and continue";
					n++;
					if(n > 2)
					{
						n = 0;
						LOG_COM << "|" << _connfd << "| timeout and disconnet";
						break;
					}
					continue;
				}
				LOG_COM << "C2SRead error." << std::endl;
				break;
			}
			// 获取接受的数据
			std::vector<std::string> nbest_res;
			kaldi::CompactLattice *rec_clat=NULL;
			kaldi::Lattice *rec_lat=NULL;
			std::vector<std::vector<int32> > words;
			std::vector<std::vector<int32> > alignments;

			ser_recv.GetData(&nbest_res, rec_clat, rec_lat);
			
			kaldi::CompactLattice composed_clat;
			kaldi::Lattice composed_lat;
			if(ser_recv.DoRescore())
			{ // do lattice rescore
				if(rec_clat != NULL)
				{
					std::vector<typename kaldi::CompactLattice::Arc::Weight> weights;
					asr_post_porcess_work_thread->LmRescore(*rec_clat, &composed_clat);
					asr_post_porcess_work_thread->ConvertLatticeToResult(
							composed_clat, ser_recv.GetLatticeNbest(), 
							&nbest_res);
					ser_send.SetLattice(&composed_clat, 1);
				}
				else if(rec_lat != NULL)
				{
					std::vector<typename kaldi::Lattice::Arc::Weight> weights;
					asr_post_porcess_work_thread->LmRescore(*rec_lat, &composed_lat);
					asr_post_porcess_work_thread->ConvertLatticeToResult(
							composed_lat, ser_recv.GetLatticeNbest(),
						   	&nbest_res);
					ser_send.SetLattice(&composed_lat, 1);
				}
				else
				{
					LOG_WARN << "No lattice but need rescore and don't rescore!!!";
				}
			}
			if(ser_recv.DoPunctuate())
			{ // do Punctuate
				// modefy nbest_res
			}
			ser_send.SetNbest(nbest_res);
			if(ser_send.Write(_connfd, ser_recv.GetEndflag()) < 0)
			{
				// 设置lattice pointer NULL,but not delete
				// 成功会设置lattice pointer is NULL
				ser_send.Reset(false);
				LOG_WARN << "service send data failed!!!";
				return -1;
			}
			ser_send.Print("post-process -> ");
			break;
		}
		ser_recv.Reset();
		ser_send.Reset();
		total_decoder_time += keep_time.Esapsed();
		asr_post_porcess_work_thread->SetTime(1, total_decoder_time, 1);


		return 0;
	}
	int32 Stop()
	{
		//const char *str = "no idle thread.";
		//send(_connfd, str, strlen(str), 0);
		LOG_COM << "Stop and connfd " << _connfd << " ok.";
		close(_connfd);
		return 0;
	}

	void GetInfo()
	{
		LOG_COM << "Task name is : " << _task_name ;
		LOG_COM << "Task connetid is : " << _connfd;
	}

private:
	int32 _connfd; // connect fd
	void SetConnFd(int32 connfd) // set socket id
	{
		_connfd = connfd;
	}
};


#include "src/util/namespace-end.h"

#endif

#include <memory>
#include <iostream>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/tcp.h>

#include <signal.h>
#include <unistd.h>
#include <error.h>
#include <netdb.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <vector>

#include "src/service2/thread-pool.h"
#include "src/service2/socket-class.h"
#include "src/service2/pthread-util.h"
#include "src/util/log-message.h"

#include "src/post-processing-service/asr-post-process-task.h"
#include "src/post-processing-service/asr-post-process-work-thread.h"

int main(int argc, char *argv[])
{
try
{
#ifdef NAMESPACE
	using namespace datemoon;
#endif
	using namespace kaldi;

	// Block SIGPIPE before starting any other threads; other threads
	// created by main() will inherit a copy of the signal mask.
	ThreadSigPipeIng();
	const char * usage =
		"Reveive decode result and post result"
		"Usage: asr-post-process-service [options]";

	ParseOptions po(usage);

	std::string config_socket="",
		word_syms_rxfilename;
	bool print_thread_pool_info = true;
	int nthread=1;
	int timeout_times = -1;

	po.Register("timeout-times", &timeout_times, "if timeout times no request, will be stop service,if < 0 will waiting for a request.");
	po.Register("config-socket", &config_socket, "Socket config file.(default NULL)");
	po.Register("nthread", &nthread, "service thread number.(default 1)");
	po.Register("print-thread-pool-info", &print_thread_pool_info, "print thread pool info");
	
	LatticeRescoreOpt lattice_rescore_opts;
	lattice_rescore_opts.Register(&po);
	AsrPostProcessWorkThreadOpt work_thread_opts;

	po.Read(argc, argv);
	if(po.NumArgs() != 0)
	{
		po.PrintUsage();
		return -1;
	}

	if(!lattice_rescore_opts.Check())
	{
		KALDI_LOG << "big lm rescore is failed!!!";
		return -1;
	}

	// lattice二遍模型
	ConstArpaLm old_const_arpa, new_const_arpa;

	fst::SymbolTable *word_syms;
	if(lattice_rescore_opts.word_syms_rxfilename.empty())
	{
		KALDI_ERR << "no word syms file!!!";
		return -1;
	}
	else
	{
		word_syms = fst::SymbolTable::ReadText(lattice_rescore_opts.word_syms_rxfilename);
		if(word_syms == NULL)
		{
			KALDI_ERR << "Could not read symbol "
				"table from file "
				<< lattice_rescore_opts.word_syms_rxfilename;
			return -1;
		}
	}
	ReadKaldiObject(lattice_rescore_opts.old_const_arpa_lm_rxfilename,
		   	&old_const_arpa);
	ReadKaldiObject(lattice_rescore_opts.new_const_arpa_lm_rxfilename,
		   	&new_const_arpa);

	SocketConf net_conf;
	datemoon::ReadConfigFromFile(config_socket, &net_conf);
	net_conf.Info();
	//SocketBase net_io(&net_conf);
	SocketEpoll net_io(&net_conf);
	if(net_io.Init() < 0)
	{
		LOG_ERR << "net_io.Init failed!!!";
		return -1;
	}

	if (0 != net_io.Bind())
	{
		LOG_ERR << "net_io.Bind failed !!!";
		return -1;
	}

	if ( 0 != net_io.Listen())
	{
		LOG_ERR << "net_io.Listen failed!!!";
		return -1;
	}

	ThreadPoolBase<ThreadBase> pool(nthread);
	{ // create thread
		std::vector<ThreadBase*> tmp_threads;
		for(int i =0;i<nthread;++i)
		{
			std::unique_ptr<AsrPostProcessWorkThread > post_process_t(
					new AsrPostProcessWorkThread(&pool,
						work_thread_opts, 
						old_const_arpa, new_const_arpa, 
						*word_syms,
						lattice_rescore_opts.oldscale,
						lattice_rescore_opts.newscale,
						lattice_rescore_opts.determinize));
			tmp_threads.push_back(
					static_cast<AsrPostProcessWorkThread*>(post_process_t.release()));
		}
		pool.Init(tmp_threads);
	} // init thread pool ok.
	LOG_COM << "init thread pool ok";
	int wait_times = 0;
	std::vector<int> connfds;
	while(1)
	{
		int ret = net_io.Accept(connfds);
		if(ret < 0)
		{
			if(ret == -2)
			{
				printf(".");
				fflush(stdout);
				if(print_thread_pool_info)
					pool.Info();
				wait_times++;
				if(timeout_times > 0)
				{
					if(wait_times > timeout_times) //终止服务
						break;
				}
			}
			else
				KALDI_WARN << "cli connect error!!!";
		}
		else
		{
			wait_times = 0;
			for(size_t i = 0 ; i < connfds.size() ; ++i)
			{
				int connectfd = connfds[i];
				KALDI_VLOG(5) << "connect " << connectfd;
				std::unique_ptr<AsrPostProcessServiceTask > task_ptr(
						new AsrPostProcessServiceTask(connectfd,
							std::to_string(connectfd)));
				pool.AddTask(
						static_cast<AsrPostProcessServiceTask *>(task_ptr.release()));
			}
		}
	}
	pool.WaitStopAll();
	pool.Info();

	KALDI_LOG << "Done.";
	if(word_syms) delete word_syms;
	return 0;
} // try
catch (const std::exception &e)
{
	std::cerr << e.what();
	return -1;
} // catch
}

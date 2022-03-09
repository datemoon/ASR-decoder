#include <memory>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "src/service2/thread-pool.h"
#include "src/util/log-message.h"
#include "src/util/config-parse-options.h"
#include "src/service2/pthread-util.h"
#include "src/post-processing-service/client-asr-post-process-task.h"
#include "src/post-processing-service/client-asr-post-process-work-thread.h"


int main(int argc, char *argv[])
{
#ifdef NAMESPACE
	using namespace datemoon;
#endif
	// Block SIGPIPE before starting any other threads; other threads
	// created by main() will inherit a copy of the signal mask.
	ThreadSigPipeIng();
	const char *usage = "This is a multithreading asr post process client test code.\n"
		"Usage: client-asr-post-process-service [options] <ark:lat.ark>\n";

	ConfigParseOptions opt(usage);
	std::string ip = "127.0.0.1";
	int port=8000;
	int nthread=1;
	int lat_binary = 1;

	std::string outfile = "result.txt";
	opt.Register("ip", &ip, "service address(default 127.0.0.1)");
	opt.Register("port", &port, "service port(default 8000)");
	opt.Register("lat-binary", &lat_binary, "lattice binary send.");
	opt.Register("nthread", &nthread, "client thread number less then service thread number.(default 1)");
	opt.Register("outfile", &outfile, "outfile name.(default result.txt)");

	opt.Read(argc, argv);
	if (opt.NumArgs() != 2)
	{
		opt.PrintUsage();
		return 1;
	}
	std::string lats_rspecifier = opt.GetArg(1);

	bool wait_thread=true;
	ThreadPoolBase<ThreadBase> pool(nthread, wait_thread);
	{ // create thread
		std::vector<ThreadBase*> tmp_threads;
		for(int i=0;i<nthread; ++i)
		{
#ifdef NOUSE_UNIQUE_PTR
			AsrPostProcessClientWorkThread * client_t = new AsrPostProcessClientWorkThread(&pool, port, ip);
			tmp_threads.push_back(client_t);
#else
			std::unique_ptr<AsrPostProcessClientWorkThread > client_t(
					new AsrPostProcessClientWorkThread(&pool, port, ip));
			tmp_threads.push_back(
					static_cast<AsrPostProcessClientWorkThread*>(client_t.release()));
#endif
		}
		pool.Init(tmp_threads);
	}
	LOG_COM << "init client thread pool ok";
	
	// load output file
	pthread_mutex_t outfile_mutex = PTHREAD_MUTEX_INITIALIZER;
	FILE *result_fp = NULL;
	if(!outfile.empty())
	{
		std::string mode("a");
		result_fp = fopen(outfile.c_str(), mode.c_str());
		if(result_fp == NULL)
		{
			LOG_WARN << "fopen " << outfile << " failed!!!";
			return -1;
		}
	}

	kaldi::SequentialCompactLatticeReader lattice_reader(lats_rspecifier);
	int32 n_done = 0;
	for (; !lattice_reader.Done(); lattice_reader.Next(), n_done++)
	{
#ifdef NOUSE_UNIQUE_PTR
		kaldi::CompactLattice * lat_ptr = new kaldi::CompactLattice(lattice_reader.Value());
		AsrPostProcessClinetTask<kaldi::CompactLatticeArc> *task = 
			new AsrPostProcessClinetTask<kaldi::CompactLatticeArc>(
					lat_ptr,
					&outfile_mutex,
					(uint)lat_binary, result_fp, lattice_reader.Key());

		if(0 != pool.AddTask(task))
#else
		std::unique_ptr<kaldi::CompactLattice > lat_ptr(
				new kaldi::CompactLattice(lattice_reader.Value()));

		std::unique_ptr<AsrPostProcessClinetTask<kaldi::CompactLatticeArc> > task(
				new AsrPostProcessClinetTask<kaldi::CompactLatticeArc>(
					static_cast<kaldi::CompactLattice*>(lat_ptr.release()),
					&outfile_mutex,
					(uint)lat_binary, result_fp, lattice_reader.Key()));
		if(0 != pool.AddTask(
					static_cast<AsrPostProcessClinetTask<kaldi::CompactLatticeArc> *>(task.release())))
#endif
		{
			LOG_WARN << "pool AddTask " << lattice_reader.Key() << " failed!!!";
		}
	}
	pool.WaitStopAll();
	pool.Info();

	if(result_fp != NULL)
		fclose(result_fp);

	return 0;
}

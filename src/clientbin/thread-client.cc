
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "src/client/asr-client-thread.h"
#include "src/client/asr-client-task.h"
#include "src/service2/thread-pool.h"
#include "src/util/log-message.h"
#include "src/util/config-parse-options.h"
#include "src/service2/pthread-util.h"

int main(int argc,char *argv[])
{
#ifdef NAMESPACE
	using namespace datemoon;
#endif
	// Block SIGPIPE before starting any other threads; other threads
	// created by main() will inherit a copy of the signal mask.
	ThreadSigPipeIng();
	const char *usage = "This is a multithreading asr client test code.\n"
		        "Usage: thread-client [options] <wav-list>\n";
	ConfigParseOptions opt(usage);
	std::string ip = "127.0.0.1";
	int port=8000;
	int nthread=1;
	int wavhead_len = 0;
	int send_len = 16000 * 2;
	bool do_align = false;
	bool do_lattice = false;
	std::string outfile = "result.txt";
	std::string alignfile = "align.txt";
	opt.Register("ip", &ip, "service address(default 127.0.0.1)");
	opt.Register("port", &port, "service port(default 8000)");
	opt.Register("nthread", &nthread, "client thread number less then service thread number.(default 1)");
	opt.Register("outfile", &outfile, "outfile name.(default result.txt)");
	opt.Register("alignfile", &alignfile, "alignfile name.(default align.txt)");
	opt.Register("wavhead-len", &wavhead_len, "wav head len");
	opt.Register("send-len", &send_len, "enery package send len");
	
	int nbest=0;
	bool realtime_res=false;
	opt.Register("nbest", &nbest, "service return nbest result.");
	opt.Register("realtime-result", &realtime_res, "service return result realtime or not.");
	opt.Register("do-align", &do_align, "service return result with align.");
	opt.Register("do-lattice", &do_lattice, "service return result with lattice.");

	opt.Read(argc, argv);
	if (opt.NumArgs() != 2)
	{
		opt.PrintUsage();
		return 1;
	}
	ASRClientOpts cli_opts(realtime_res, nbest, do_align, do_lattice);
	std::string wavlist = opt.GetArg(1);

	//sigset_t set;
	//sigemptyset(&set);
	//sigaddset(&set, SIGPIPE);
	//pthread_sigmask(SIG_BLOCK, &set, NULL);

	//signal(SIGPIPE, SIG_IGN); // ignore SIGPIPE to avoid crashing when socket forcefully disconnected


	if(CreateResultHandle(outfile.c_str(), "w", alignfile) != true)
	{
		LOG_WARN << "Open outfile " << outfile << " failed!!!, Will be printf stdout.";
	}
	bool wait_thread=true;
	ThreadPoolBase<ThreadBase> pool(nthread, wait_thread);
	{
		// create thread
		std::vector<ThreadBase*> tmp_threads;
		for(int i=0;i<nthread; ++i)
		{
			ASRClinetThread *client_t = new ASRClinetThread(&pool, port, ip);
			tmp_threads.push_back(client_t);
		}
		pool.Init(tmp_threads);
	}
	LOG_COM << "init client thread pool ok";

	FILE *fp = fopen(wavlist.c_str(), "r");
	if(fp == NULL)
	{
		LOG_ERR << "Open wav: " << wavlist <<" failed!!!";
		return -1;
	}
#define FILE_LEN 256
	char wavfile[FILE_LEN];
	memset(wavfile,0x00,sizeof(wavfile));
	while(fgets(wavfile, FILE_LEN, fp) != NULL)
	{
		// delete \n
		wavfile[strlen(wavfile)-1] = '\0';
		ASRClientTask *task = new ASRClientTask(cli_opts, wavfile, wavhead_len, send_len);
		if(0 != pool.AddTask(task))
		{
			LOG_WARN << "pool AddTask failed!!!";
		}
		memset(wavfile,0x00,sizeof(wavfile));
	}
	pool.WaitStopAll();
	pool.Info();

	fclose(fp);
	CloseResultHandle();
	//sleep(10);
	return 0;
}

#include <iostream>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
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

#include "src/gpu-asr/v1-gpu-asr-work-thread.h"
#include "src/gpu-asr/v1-gpu-kaldi-worker-pool.h"
#include "src/gpu-asr/v1-gpu-asr-task.h"

using namespace std;

namespace kaldi
{
namespace cuda_decoder
{
// 这里将要注册gpudecoder相关的所有选项并加载选项和模型
int InitAsrSource(int argc, char *argv[],
		ParseOptions *opts,
		V1CudaOnlineBinaryOptions *cuda_online_opts,
		BatchedThreadedNnet3CudaOnlinePipelineConfig *batched_decoder_config,
		TransitionModel *trans_model, nnet3::AmNnetSimple *am_nnet,
		fst::Fst<fst::StdArc> **decode_fst,
		fst::SymbolTable **word_syms)
{
	// gpu 相关设置
	g_cuda_allocator.SetOptions(g_allocator_options);
	CuDevice::Instantiate().SelectGpuId("yes");
	CuDevice::Instantiate().AllowMultithreading();

	CuDevice::RegisterDeviceOptions(opts);
	RegisterCuAllocatorOptions(opts);
	cuda_online_opts->Register(opts);
	batched_decoder_config->Register(opts);

	opts->Read(argc, argv);
	if(opts->NumArgs() != 2)
	{
		opts->PrintUsage();
		return 1;
	}

	batched_decoder_config->num_channels = 
		std::max(batched_decoder_config->num_channels,
				2 * cuda_online_opts->num_streaming_channels);

	cuda_online_opts->nnet3_rxfilename = opts->GetArg(1);
	cuda_online_opts->fst_rxfilename = opts->GetArg(2);
	//cuda_online_opts->wav_rspecifier = opts->GetArg(3);
	//cuda_online_opts->clat_wspecifier = opts->GetArg(4);

	// read transition model and nnet
	bool binary;
	Input ki(cuda_online_opts->nnet3_rxfilename, &binary);
	trans_model->Read(ki.Stream(), binary);
	am_nnet->Read(ki.Stream(), binary);
	SetBatchnormTestMode(true, &(am_nnet->GetNnet()));
	SetDropoutTestMode(true, &(am_nnet->GetNnet()));
	nnet3::CollapseModel(nnet3::CollapseModelConfig(), &(am_nnet->GetNnet()));

	*decode_fst = fst::ReadFstKaldiGeneric(cuda_online_opts->fst_rxfilename);
	if (!cuda_online_opts->word_syms_rxfilename.empty())
	{
		if (!(*word_syms = fst::SymbolTable::ReadText(cuda_online_opts->word_syms_rxfilename)))
		{
			KALDI_ERR << "Could not read symbol "
				"table from file "
				<< cuda_online_opts->word_syms_rxfilename;
		}
	}
	return 0;
}
} // cuda_decoder
} // kaldi

int main(int argc, char*argv[])
{
try
{
#ifdef NAMESPACE
	using namespace datemoon;
#endif
	using namespace kaldi;
	using namespace cuda_decoder;
//	using namespace fst;

	// Block SIGPIPE before starting any other threads; other threads
	// created by main() will inherit a copy of the signal mask.
	ThreadSigPipeIng();
	const char * usage = 
		"Receive wav data and simulates online "
		"decoding with "
		"neural nets\n"
		"(nnet3 setup).  Note: some configuration values "
		"and inputs "
		"are\n"
		"set via config files whose filenames are passed "
		"as "
		"options\n"
		"\n"
		"Usage: nnet3-cuda-decoder-service [options] "
		"<nnet3-in> "
		"<fst-in> "
		"<wav-rspecifier> <lattice-wspecifier>\n";

	ParseOptions po(usage);

	// 记录被请求次数
	unsigned long long int ntimes = 0;

	std::string config_socket="";
	bool print_thread_pool_info = true;
	int nthread=1;
	int timeout_times = -1;
	po.Register("timeout-times", &timeout_times, "if timeout times no request, will be stop service,if < 0 will waiting for a request.");
	po.Register("config-socket", &config_socket, "Socket config file.(default NULL)");
	po.Register("nthread", &nthread, "service thread number.(default 1)");
	po.Register("print-thread-pool-info", &print_thread_pool_info, "print thread pool info");

	V1GpuASRWorkThreadOpt v1_gpu_asr_work_thread_opts;
	v1_gpu_asr_work_thread_opts.Register(&po);


	TransitionModel trans_model;
	nnet3::AmNnetSimple am_nnet;
	fst::Fst<fst::StdArc> *decode_fst;
	fst::SymbolTable *word_syms =  NULL;
	V1CudaOnlineBinaryOptions cuda_online_opts;
	BatchedThreadedNnet3CudaOnlinePipelineConfig batched_decoder_config;

	int ret = InitAsrSource(argc, argv, &po, &cuda_online_opts, &batched_decoder_config,
		   &trans_model, &am_nnet, &decode_fst, &word_syms);
	if(0 != ret)
	{
		return ret;
	}
	CudaOnlinePipelineDynamicBatcherConfig dynamic_batcher_config;

	BatchedThreadedNnet3CudaOnlinePipelineWarp cuda_decoder_service(
			cuda_online_opts,
			batched_decoder_config,
			dynamic_batcher_config,
			*decode_fst, am_nnet, trans_model, word_syms);
	delete decode_fst;

	if (cuda_online_opts.write_lattice)
		cuda_online_opts.generate_lattice = true;

	SocketConf net_conf;
	datemoon::ReadConfigFromFile(config_socket, &net_conf);
	net_conf.Info();
	SocketBase net_io(&net_conf);
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
		vector<ThreadBase*> tmp_threads;
		for(int i =0;i<nthread;++i)
		{
			V1GpuASRWorkThread * asr_t = new V1GpuASRWorkThread(&pool,
					cuda_decoder_service, v1_gpu_asr_work_thread_opts);
			tmp_threads.push_back(asr_t);
		}
		pool.Init(tmp_threads);
	}
	LOG_COM << "init thread pool ok";
	int wait_times = 0;
	while(1)
	{
		int connectfd = net_io.Accept();
		if(connectfd < 0)
		{
			if(connectfd == -2)
			{
				printf(".");
				fflush(stdout);
				if(print_thread_pool_info)
				{
					LOG_COM << "Service process " << ntimes << " request.";
					pool.Info();
				}
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
			uint64_t corr_id = ntimes++ * GPU_CORRID_OFFSET;
			wait_times = 0;
			KALDI_VLOG(5) << "connect " << connectfd;
			V1GpuASRServiceTask *ta = 
				new V1GpuASRServiceTask(connectfd,
						corr_id);
			pool.AddTask(ta);
		}
	}

	pool.WaitStopAll();
	pool.Info();

	cuda_decoder_service.WaitForCompletion();
	KALDI_LOG << "Done.";
	nvtxRangePop();

	cudaDeviceSynchronize();
	// end
	if(word_syms) delete word_syms;
	return 0;
} // try
catch (const std::exception &e)
{
	std::cerr << e.what();
	return -1;
} // catch
} // main

#ifndef __V1_GPU_ASR_WORK_THREAD_H__
#define __V1_GPU_ASR_WORK_THREAD_H__

#include "src/service2/net-data-package.h"
#include "src/service2/thread-pool-work-thread.h"
//#include "src/gpu-asr/v1-gpu-kaldi-worker-pool.h"

#include "src/gpu-asr/gpu-worker-pool-itf.h"

#include "src/online-vad/online-vad.h"

#include "src/util/namespace-start.h"

struct V1GpuASRWorkThreadOpt
{
	bool _use_energy_vad;
	kaldi::VadProcessOpts _vad_process_opts;


	V1GpuASRWorkThreadOpt(std::string vad_judge_prefix= "energy-vad-judge",
		   std::string energy_vad_prefix = "energy-vad"):
		_use_energy_vad(true),
		_vad_process_opts(vad_judge_prefix, energy_vad_prefix) { }

	void Register(kaldi::OptionsItf *opts)
	{
		_vad_process_opts.Register(opts);
		opts->Register("use-energy-vad", &_use_energy_vad,
				"Use energy vad or not.");
	}
};

class V1GpuASRWorkThread:public ThreadPoolWorkThread
{
public:
	friend class V1GpuASRServiceTask;
	friend class V1GpuASRServiceTask1;
	typedef ThreadBase::int32 int32;

	V1GpuASRWorkThread(ThreadPoolBase<ThreadBase> *tp,
			kaldi::cuda_decoder::BatchThreadCudaPipelineItf &cuda_pipeline_warp,
			const V1GpuASRWorkThreadOpt &v1_gpu_asr_work_thread_opts):
		ThreadPoolWorkThread(tp), _cuda_pipeline_warp(cuda_pipeline_warp), 
		_v1_gpu_asr_work_thread_opts(v1_gpu_asr_work_thread_opts),
		_energy_vad(_v1_gpu_asr_work_thread_opts._vad_process_opts){ }

	~V1GpuASRWorkThread() { }
private:
	kaldi::cuda_decoder::BatchThreadCudaPipelineItf &_cuda_pipeline_warp;
	// recv data from client
	C2SPackageAnalysis _ser_c2s_package_analysys;
	// send data to client
	S2CPackageAnalysis _ser_s2c_package_analysys;

	const V1GpuASRWorkThreadOpt &_v1_gpu_asr_work_thread_opts;
	kaldi::EnergyVadProcess<kaldi::BaseFloat> _energy_vad;
};

#include "src/util/namespace-end.h"

#endif

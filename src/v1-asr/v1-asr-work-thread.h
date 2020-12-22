#ifndef __V1_ASR_WORK_THREAD_H__
#define __V1_ASR_WORK_THREAD_H__
#include "src/v1-asr/kaldi-v1-asr-online.h"
#include "src/service2/net-data-package.h"
#include "src/service2/thread-pool-work-thread.h"
//#include "src/vad/energy-vad.h"
#include "src/v1-asr/online-vad.h"

#include "src/util/namespace-start.h"

class V1ASRWorkThread:public ThreadPoolWorkThread
{
public:
	friend class V1ASRServiceTask;
	typedef ThreadBase::int32 int32;
public:
	V1ASRWorkThread(ThreadPoolBase<ThreadBase> *tp,
			kaldi::V1AsrOpts &v1_asr_opts, kaldi::V1AsrSource &v1_asr_source):
		ThreadPoolWorkThread(tp),
		_v1_asr_worker(v1_asr_opts, v1_asr_source),
		_energy_vad(v1_asr_source._energy_vad_judge_opts, "sum_square_root", 16000, 0.025, 0.01, 0, 32768*0.005, 32768*0.05),
		_use_energy_vad(v1_asr_opts._use_energy_vad)
	{ }

	~V1ASRWorkThread() { }
//	void Run();
private:
	// recv data from client
	C2SPackageAnalysis _ser_c2s_package_analysys;
	// send data to client
	S2CPackageAnalysis _ser_s2c_package_analysys;
	// feature
	// nnet
	// fst
	kaldi::V1AsrWorker _v1_asr_worker;
	kaldi::V1EnergyVad<short> _energy_vad;
	bool _use_energy_vad;
};

#include "src/util/namespace-end.h"
#endif

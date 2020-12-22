#ifndef __V2_ASR_WORK_THREAD_H__
#define __V2_ASR_WORK_THREAD_H__
#include "src/service2/thread-class.h"
#include "src/service2/thread-pool.h"
#include "src/service2/net-data-package.h"
#include "src/service2/thread-info.h"

#include "src/vad/energy-vad.h"

#include "src/kaldi-nnet3/kaldi-online-nnet3-my-decoder.h"

#include "src/util/namespace-start.h"

/// This is class use kaldi feature , nnet and mydecoder 
/// package asr service class work thread.
class V2ASRWorkThread:public ThreadBase, public ThreadTimeInfo
{
public:
	friend class V2ASRServiceTask;
	typedef ThreadBase::int32 int32;
public:

	//
	V2ASRWorkThread(ThreadPoolBase<ThreadBase> *thread_pool, 
			OnlineDecoderInfo &online_info, int32 index=-1):
		ThreadBase(index),
		_thread_pool(thread_pool),
		_online_clg_decoder(online_info),
		_energy_vad(16000, 0.025, 0.01, -1, EnergyVad<short>::SIL, 0.3, 0.8, 50, 50, 32768*0.005, 32768*0.05, "sum_square_root") { }

	~V2ASRWorkThread() { }
	void Run();
	void TimeInfo() { WorkEfficiencyInfo(); }
	void SetTime(double data_time, double work_time, double nosil_time = -1)
	{
		_data_time += data_time;
		_work_time += work_time;
		if(nosil_time < 0)
			nosil_time = data_time;
		_effect_data_time += nosil_time;
		_thread_pool->SetTime(data_time, work_time, nosil_time);
	}
private:
	double GetEfficiency() 
	{ 
		if(_data_time == 0) 
			return -1;
		return _work_time/_data_time; 
	}

	void WorkEfficiencyInfo()
	{
		LOG_COM << "ThreadIndex = " << GetThreadIndex() 
			<< " -> wav data time is : " << _data_time;
		LOG_COM << "ThreadIndex = " << GetThreadIndex() 
			<< " -> nosil data time is : " << _effect_data_time;
		LOG_COM << "ThreadIndex = " << GetThreadIndex() 
			<< " -> work time is : " << _work_time;
		LOG_COM << "ThreadIndex = " << GetThreadIndex() 
			<< " -> WorkEfficiencyInfo is rt : " << GetEfficiency();
	}


	ThreadPoolBase<ThreadBase> *_thread_pool;
	// online decoder instance
	OnlineClgLatticeFastDecoder _online_clg_decoder;

	// vad
	EnergyVad<short> _energy_vad;
	// recv data from client
	C2SPackageAnalysis _ser_c2s_package_analysys;
	// send data to client
	S2CPackageAnalysis _ser_s2c_package_analysys;
};

#include "src/util/namespace-end.h"
#endif

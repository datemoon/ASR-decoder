#ifndef __V2_ASR_WORK_THREAD_H__
#define __V2_ASR_WORK_THREAD_H__
#include "src/service2/thread-class.h"
#include "src/service2/thread-pool.h"
#include "src/service2/net-data-package.h"

#include "src/kaldi-nnet3/kaldi-online-nnet3-my-decoder.h"

#include "src/util/namespace-start.h"

/// This is class use kaldi feature , nnet and mydecoder 
/// package asr service class work thread.
class V2ASRWorkThread:public ThreadBase
{
public:
	friend class V2ASRServiceTask;
	typedef ThreadBase::int32 int32;
public:

	//
	V2ASRWorkThread(OnlineDecoderInfo &online_info):
		_online_clg_decoder(online_info) { }

	~V2ASRWorkThread() { }
	void Run();
private:
	// online decoder instance
	OnlineClgLatticeFastDecoder _online_clg_decoder;

	ThreadPoolBase<ThreadBase> *_thread_pool;
	// recv data from client
	C2SPackageAnalysis _ser_c2s_package_analysys;
	// send data to client
	S2CPackageAnalysis _ser_s2c_package_analysys;
};

#include "src/util/namespace-end.h"
#endif

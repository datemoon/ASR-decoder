#ifndef __ONLINE_NNET3_FEATURE_PIPELINE_IO_H__
#define __ONLINE_NNET3_FEATURE_PIPELINE_IO_H__

#include "matrix/matrix-lib.h"
#include "util/common-utils.h"
#include "itf/online-feature-itf.h"
//#include "feat/online-feature.h"
//#include "online2/online-ivector-feature.h"


namespace kaldi
{
// 这个类主要完成接收特征，并保证流式提供特征
// 继承kaldi OnlineFeatureInterface 类的接口，
// 添加了AcceptFeature接口，
// 以方便利用特征做前端vad切断等操作
class OnlineFeatureMatrix: public OnlineBaseFeature
{
public:
	// max_feature_vectors 必须大于chunk，否则多的特征帧将会覆盖原来的帧
	explicit OnlineFeatureMatrix(int32 dim, BaseFloat frameshiftinseconds,
			int32 max_feature_vectors):
		_dim(dim),
		_frameshiftinseconds(frameshiftinseconds),
		_input_finished(false),
		_features(max_feature_vectors){ }

	virtual int32 Dim() const
	{
		return _dim;
	}

	virtual bool IsLastFrame(int32 frame) const
	{
		return _input_finished && frame == NumFramesReady() - 1;
	}
	virtual int32 NumFramesReady() const
	{
		return _features.Size();
	}
	virtual BaseFloat FrameShiftInSeconds() const
	{
		return _frameshiftinseconds;
	}
	virtual void GetFrame(int32 frame, VectorBase<BaseFloat> *feat)
	{
		feat->CopyFromVec(*(_features.At(frame)));
	}

	virtual ~OnlineFeatureMatrix() { }

	virtual void InputFinished()
	{
		_input_finished = true;
	}

	virtual void AcceptWaveform(BaseFloat sampling_rate,
			const VectorBase<BaseFloat> &waveform){ }

	virtual void AcceptFeature(const VectorBase<BaseFloat> *features)
	{
		if(features->Dim() == 0)
			return;
		if(Dim() != features->Dim())
			_dim = features->Dim();
		Vector<BaseFloat> *this_feature = new Vector<BaseFloat>(Dim(), kUndefined);
		this_feature->CopyFromVec(*(features));
		_features.PushBack(this_feature);
	}
	
	virtual void AcceptFeatureMatrix(const MatrixBase<BaseFloat> *features)
	{
		for(int32 i = 0; i < features->NumRows() ; ++i)
		{
			SubVector<BaseFloat> this_row(*features, i);
			AcceptFeature(&this_row);
//			Vector<BaseFloat> *this_feature = new Vector<BaseFloat>(Dim(), kUndefined);
//			this_feature->CopyFromVec(this_row);
//			_features.PushBack(this_feature);
		}
	}

private:
	int32 _dim;
	BaseFloat _frameshiftinseconds;
	bool _input_finished;

	RecyclingVector _features;
};


class OnlineNnet3FeaturePipelineIo: public OnlineFeatureInterface
{
public:
	explicit OnlineNnet3FeaturePipelineIo(
			const OnlineNnet2FeaturePipelineInfo &info,
			int32 dim):
		_info(info),_base_feature(NULL), _ivector_feature(NULL)
	{
		int32 max_feature_vectors = -1;
		if (_info.feature_type == "mfcc")
			max_feature_vectors = _info.mfcc_opts.frame_opts.max_feature_vectors;
		else if(_info.feature_type == "plp")
			max_feature_vectors = _info.plp_opts.frame_opts.max_feature_vectors;
		else if(_info.feature_type == "fbank")
			max_feature_vectors = _info.fbank_opts.frame_opts.max_feature_vectors;

		_base_feature = new OnlineFeatureMatrix(dim,
			   	_info.FrameShiftInSeconds(), max_feature_vectors);

		if(_info.use_ivectors)
		{
			_ivector_feature = new OnlineFeatureMatrix(dim,
					_info.FrameShiftInSeconds(), max_feature_vectors);
		}
		_dim = _base_feature->Dim();
	}

	virtual int32 Dim() const
	{
		return _dim;
	}

	virtual void AcceptFeature(const VectorBase<BaseFloat> *features)
	{
		if(features->Dim() == 0)
			return;
		_base_feature->AcceptFeature(features);
	}

	virtual void AcceptFeatureMatrix(const MatrixBase<BaseFloat> *features)
	{
		if(features->NumRows() == 0)
			return;
		_base_feature->AcceptFeatureMatrix(features);
	}

	virtual int32 NumFramesReady() const
	{
		return _base_feature->NumFramesReady();
	}

	virtual bool IsLastFrame(int32 frame) const
	{
		return _base_feature->IsLastFrame(frame);
	}

	virtual void GetFrame(int32 frame, VectorBase<BaseFloat> *feat)
	{
		return _base_feature->GetFrame(frame, feat);
	}

	virtual BaseFloat FrameShiftInSeconds() const 
	{
		return _info.FrameShiftInSeconds();
	}

	OnlineFeatureInterface *InputFeature()
	{
		return static_cast<OnlineFeatureInterface *>(_base_feature);
	}
	const OnlineFeatureInterface *IvectorFeature() const
	{
		return static_cast<OnlineFeatureInterface *>(_ivector_feature);
	}
	OnlineFeatureInterface *IvectorFeature()
	{
		return static_cast<OnlineFeatureInterface *>(_ivector_feature);
	}

	void InputFinished()
	{
		if(_base_feature != NULL)
			_base_feature->InputFinished();
		if(_ivector_feature != NULL)
			_ivector_feature->InputFinished();
	}
	virtual ~OnlineNnet3FeaturePipelineIo()
	{
		if(_base_feature != NULL)
			delete _base_feature;
		if(_ivector_feature != NULL)
			delete _ivector_feature;
	}

private:

	const OnlineNnet2FeaturePipelineInfo &_info;

	OnlineFeatureMatrix *_base_feature;
	OnlineFeatureMatrix *_ivector_feature;

	//OnlineFeatureInterface *nnet3_feature_;

	int32 _dim;
};

} // namespace kaldi
#endif

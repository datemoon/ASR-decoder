#ifndef __ONLINE_NNET_VAD_H__
#define __ONLINE_NNET_VAD_H__

#include <string>
#include <iostream>
#include <vector>
#include "online2/online-nnet2-feature-pipeline.h"
#include "nnet3/nnet-utils.h"
#include "nnet3/decodable-online-looped.h"

namespace kaldi
{
// audio
//   - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// [ -             - - - - -       -         - - - - -  - ] 
//  _left_context(6)          current frame             _right_context(6)
//                   _sil_left_context(3)        _nosil_right_context(3)
// audio2sil window  [_left_context(6), _nosil_right_context(3)]
// sil2audio window  [_sil_left_context(3), _right_context(6)]
// V2
//   - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// [ -             - - - - -       -         - - - - -  - ] - - - - -      - ]
//  _left_context(6)          current frame             _right_context(6)  _right_extra_context(12)
//                   _sil_left_context(3)        _nosil_right_context(3)
// audio2sil and sil2audio window  [_left_context(6), _right_context(6)]
// audio convert sil and sil convert audio [_left_context(6), _right_extra_context(6)]
//
struct VadJudgeOptions
{
	BaseFloat _sil2audio_audio_ratio;  // audio:win > 0.5 静音转语音时，窗内语音占比
	BaseFloat _audio2sil_sil_ratio;  // sil:win > 0.8   语音转静音时，窗内静音占比

	// vad context
	int32 _left_context;
	int32 _right_context;
	int32 _sil_left_context;
	int32 _nosil_right_context;

	// 这个参数为了防止窗过大，导致的短语音被删除，默认使用33(0.33s)
	int32 _win_nosil_frames_num;
	// 这个参数为了防止窗过大，导致比较长的静音被当做语音，默认使用100(1s)
	int32 _win_sil_frames_num;

	int32 _right_extra_context;

	BaseFloat _nosil_threadhold;
	bool _start_sil;
	bool _end_sil;
	std::string _smooth_version;
	std::string _prefix;

	VadJudgeOptions(std::string prefix="vad-judge"):
		_sil2audio_audio_ratio(0.33),
		_audio2sil_sil_ratio(0.8),
		_left_context(20),
		_right_context(20),
		_sil_left_context(10),
		_nosil_right_context(10),
		_win_nosil_frames_num(33),
		_win_sil_frames_num(100),
		_right_extra_context(0),
		_nosil_threadhold(0.5),
		_start_sil(true),
		_end_sil(true),
		_smooth_version("v2"),
		_prefix(prefix)
	{
		Check();
   	}

	void Check() 
	{
		if(_sil_left_context > _left_context)
		{
			KALDI_LOG << "_sil_left_context > _left_context : " 
				<< _sil_left_context << " > " << _left_context
				<< "set _sil_left_context = " << _left_context;
			_sil_left_context = _left_context;
		}
		if(_nosil_right_context > _right_context)
		{
			KALDI_LOG << "_nosil_right_context > _right_context : " 
				<< _nosil_right_context << " > " << _right_context
				<< "nosil_right_context = " << _right_context;
			_nosil_right_context = _right_context;
		}
		if(_right_extra_context < 0)
		{
			KALDI_LOG << "_right_extra_context < 0 : " 
			   	<< _right_extra_context << " < 0 "
				<<  "_right_extra_context = 0" ;
			_right_extra_context = 0;
		}
		// 这里计算一下静音和非静音之间转换的间隔
		int32 win_len = _left_context + _right_context + 1;
		int32 sil2audio_audio_frames = win_len * _sil2audio_audio_ratio;
		int32 audio2sil_audio_frames = win_len * (1 - _audio2sil_sil_ratio);
		int32 min_transition_interval = abs(sil2audio_audio_frames - audio2sil_audio_frames) / 2;
		KALDI_LOG << "Minimum silence and audio transition interval is : " << min_transition_interval ;
	}

	void Register(OptionsItf *opts)
	{
		// register the vad judge options with the prefix "vad-judge".
		ParseOptions vad_opts(_prefix, opts);
		vad_opts.Register("sil2audio-audio-ratio", &_sil2audio_audio_ratio,
				"vad silence convert audio, window audio frames ratio.");
		vad_opts.Register("audio2sil-sil-ratio", &_audio2sil_sil_ratio,
			"vad audio convert silence, window silence frames ratio.");
		vad_opts.Register("left-context", &_left_context,
			"window left-context audio2sil window use left boundary.");
		vad_opts.Register("nosil-right-context", &_nosil_right_context,
			"window nosil-right-context audio2sil window use right boundary.");
		vad_opts.Register("right-context", &_right_context,
			"window right-context sil2audio window use right boundary.");
		vad_opts.Register("sil-left-context", &_sil_left_context,
			"window sil-left-context sil2audio window use left boundary.");
		vad_opts.Register("win-nosil-frames-num", &_win_nosil_frames_num,
			"window nosil frames num.");
		vad_opts.Register("win-sil-frames-num", &_win_sil_frames_num,
			"window sil frames num.");
		vad_opts.Register("right-extra-context", &_right_extra_context,
			"window right extra context.");
		vad_opts.Register("nosil-threadhold", &_nosil_threadhold,
			"use judge frame confidence,big then is nosil.");
		vad_opts.Register("start-sil", &_start_sil,
			"assumption start is silence or not.");
		vad_opts.Register("end-sil", &_end_sil,
			"assumption end is silence or not.");
		vad_opts.Register("smooth-version", &_smooth_version,
			"smooth-version use vad strategy.");
}
};

enum { SIL = 0, AUDIO = 1};
//struct AliInfo
//{
//public:
//	int32 _data_flag;
//	int32 _start_frame;
//	int32 _end_frame;
//	AliInfo():_data_flag(SIL),_start_frame(0), _end_frame(0) {}
//	int32 Start() { return _start_frame;}
//	int32 End() { return _end_frame;}
//	int32 Flag() { return _data_flag;}
//	int32 CurFrames() { return _end_frame - _start_frame;}
//};
//typedef std::vector<std::pair<int32, std::pair<int32 ,int32> > > VadSeg;
//                audio_flag    start, end frame
typedef std::pair<int32, std::pair<int32 ,int32> > AliInfo;
typedef std::vector<AliInfo > VadSeg;

// 这是一个非实时的接口，你可以实时调用SmoothVad，
// 最后根据生产的_vad_ali，在进行一次合并，
// 将中间切割短于sil_frames的静音段
// 转换为语音段，从而合并前后2段语音，比如：
// sil 0 20
// nosil 20 60
// sil 60 80
// nosil 80 100
// sil 100 160
// nosil 160 230
// sil 230 240
// sil_frames=50，合并之后：
// sil 0 20
// nosil 20 100
// sil 100 160
// nosil 160 230
// sil 230 240
void CompressAlignVad(const VadSeg &vad_ali,
		VadSeg &compress_vad_ali, int32 sil_frames=50)
{
	compress_vad_ali.clear();
	for(size_t i = 0 ; i < vad_ali.size() ; i++)
	{
		if(compress_vad_ali.size() > 0 &&
				compress_vad_ali.back().first == vad_ali[i].first)
		{ // 如果当前段和之前段是同类型合并这一段到之前段上
			compress_vad_ali.back().second.second = vad_ali[i].second.second;
			continue;
		}
		if(vad_ali[i].first == SIL)
		{
			compress_vad_ali.push_back(vad_ali[i]);
		} // SIL
		else if(vad_ali[i].first == AUDIO)
		{
			if(compress_vad_ali.size() > 1)
			{ // 现在已经是中间的语音段
				KALDI_ASSERT(compress_vad_ali.back().first == SIL);
				int32 prev_sil_len = compress_vad_ali.back().second.second - compress_vad_ali.back().second.first;
				if(prev_sil_len < sil_frames)
				{ // 删掉这段静音，将其和前后2段语音合成一段语音
					compress_vad_ali.pop_back();
					compress_vad_ali.back().second.second = vad_ali[i].second.second;
				}
				else
				{
					compress_vad_ali.push_back(vad_ali[i]);
				}
			}
			else
			{
				compress_vad_ali.push_back(vad_ali[i]);
			}
		} // AUDIO
		else
		{
			KALDI_WARN << "Vad align flag error!!!";
		}
	} // for _vad_ali
}

void PrintVadSeg(VadSeg &vad_ali, std::string prefix = "current -> ", int32 verbose = 3)
{
	for(auto it = vad_ali.begin(); it != vad_ali.end(); ++it)
	{
		std::string type;
		if(it->first == SIL)
			type = "sil   ";
		else
			type = "nosil ";
		KALDI_VLOG(verbose) << prefix << type << " " << it->second.first << " "
			<< it->second.second << " "
			<< it->second.second - it->second.first;
	}
}
// 这是一个vad平滑类，根据vad初步标记结果，给出平滑之后的vad结果
// 支持流式处理
class VadJudge
{
public:
	VadJudge(const VadJudgeOptions &vad_opt):
		_vad_opt(vad_opt),
		_audio2sil_cache_sum(-1),
		_sil2audio_cache_sum(-1),
		_first_win_cache_sum(-1),
		_second_win_cache_sum(-1),
		_audio_flag(SIL),
		_nosil_frames(0),
		_sil_frames(0),
		_cur_offset(0)
	{ 
		// 如果是开始，补全左边能量和判断
		// Thr first frame, copy first frame to _left_context.
		if(_vad_opt._start_sil == true)
		{
			// 假设开始是静音
//			_data_flag = 1;
			_pre_frames_confidence = std::vector<BaseFloat>(_vad_opt._left_context, 0);
			_pre_frames_judge = std::vector<int32>(_vad_opt._left_context, 0);
		}
	}

	~VadJudge() { }
	
	void Reset()
	{
		// 如果是开始，补全左边能量和判断
		// Thr first frame, copy first frame to _left_context.
		if(_vad_opt._start_sil == true)
		{
			// 假设开始是静音
//			_data_flag = 1;
			_pre_frames_confidence = std::vector<BaseFloat>(_vad_opt._left_context, 0);
			_pre_frames_judge = std::vector<int32>(_vad_opt._left_context, 0);
		}
		_audio2sil_cache_sum = -1;
		_sil2audio_cache_sum = -1;
		_first_win_cache_sum = -1;
	   	_second_win_cache_sum = -1;
		_audio_flag = SIL;
		_nosil_frames = 0;
		_sil_frames = 0;
		_cur_offset = 0;
		_vad_ali.clear();
	}

	void GetInfo(int32 &sil_frames, int32 &nosil_frames, VadSeg **vad_ali = NULL)
	{
		sil_frames = _sil_frames;
		nosil_frames = _nosil_frames;
		if( vad_ali != NULL)
			*vad_ali = &_vad_ali;
	}

	template<typename T>
	void Smooth(std::vector<T> &current_value, std::vector<T> &smooth_value,
			int32 start_offset, int32 end_offset,
			int32 smooth_win_left_context, int32 smooth_win_right_context,
			BaseFloat &prev_win_cache_sum = -1)
	{
		if(current_value.size() == 0)
			return ;

		KALDI_ASSERT(current_value.size() >= end_offset);
		KALDI_ASSERT(start_offset == smooth_win_left_context+smooth_win_right_context);
		if(prev_win_cache_sum < 0)
		{
			prev_win_cache_sum = 0;
			for(size_t i=0; i<smooth_win_left_context+smooth_win_right_context;++i)
			{
				prev_win_cache_sum += current_value[i];
			}
		}
		int32 win_len = smooth_win_left_context + 1 + smooth_win_right_context;
		T average = 0;
		for(size_t i=start_offset; i < end_offset; ++i)
		{
			BaseFloat sum = (prev_win_cache_sum + current_value[i])*1.0/win_len;
			smooth_value.push_back(sum);
			prev_win_cache_sum = sum - current_value[i-start_offset];
		}
	}

	std::vector<int32> SmoothVad(std::vector<BaseFloat> &frame_confidence, int32 end_flag = 0)
	{
		if(_vad_opt._smooth_version == "v1" || _vad_opt._smooth_version == "V1")
			return V1SmoothVad(frame_confidence, end_flag);
		else if (_vad_opt._smooth_version == "v2" || _vad_opt._smooth_version == "V2")
			return V2SmoothVad(frame_confidence, end_flag);
		else
		{
			KALDI_LOG << "No this smooth vad";
		}
		return std::vector<int32>();
	}

	std::vector<int32> V2SmoothVad(std::vector<BaseFloat> &frame_confidence, int32 end_flag = 0)
	{
		for(typename std::vector<BaseFloat>::iterator it = frame_confidence.begin();
				it != frame_confidence.end(); ++it)
		{
			if(*it > _vad_opt._nosil_threadhold)
			{
				_pre_frames_judge.push_back(1);
			}
			else
			{
				_pre_frames_judge.push_back(0);
			}
			// 如果是开始并且假设开始不是静音，补全左边能量和判断
			// Thr first frame, copy first frame to _left_context.
			if(_pre_frames_confidence.size() == 0)
			{
//				_data_flag = 1;
				_pre_frames_confidence = std::vector<BaseFloat>(_vad_opt._left_context, frame_confidence[0]);
				_pre_frames_judge = std::vector<int32>(_vad_opt._left_context+1, _pre_frames_judge[0]);
			}
		}
		if(frame_confidence.size() > 0)
			_pre_frames_confidence.insert(_pre_frames_confidence.end(),
					frame_confidence.begin(), frame_confidence.end());

		// The end data, copy the last frame to _right_context + _right_extra_context.
		if(end_flag != 0 && _pre_frames_confidence.size() > 0)
		{
			int32 window_rigth_context = _vad_opt._right_context + _vad_opt._right_extra_context;
			if(_vad_opt._end_sil == true)
			{
				std::vector<BaseFloat> tmp_confidence(window_rigth_context, 0);
				_pre_frames_confidence.insert(_pre_frames_confidence.end(),
						tmp_confidence.begin(), tmp_confidence.end());
				std::vector<int32> tmp_judge(window_rigth_context, SIL);
				_pre_frames_judge.insert(_pre_frames_judge.end(),tmp_judge.begin(), tmp_judge.end());
			}
			else
			{
				std::vector<BaseFloat> tmp_confidence(window_rigth_context, _pre_frames_confidence.back());
				_pre_frames_confidence.insert(_pre_frames_confidence.end(),
						tmp_confidence.begin(), tmp_confidence.end());
				std::vector<int32> tmp_judge(window_rigth_context, _pre_frames_judge.back());
				_pre_frames_judge.insert(_pre_frames_judge.end(), tmp_judge.begin(), tmp_judge.end());
			}
		}

		// now smooth vad and judge vad
		std::vector<int32> judge_frames;

		// 无论当前是语音还是静音都使用这个窗
		int32 first_window_frames_num = _vad_opt._left_context + _vad_opt._right_context + 1;
		// 如果判定应该进行静音和非静音的转换，采用这个窗进行二次判定
		int32 second_window_frames_num = _vad_opt._left_context + _vad_opt._right_context + _vad_opt._right_extra_context + 1;
		
		if(_pre_frames_judge.size() > second_window_frames_num - 1)
		{// 如果积攒的vad置信度帧数大于second_window_frames_num - 1，就可以进行平滑了
			if(_first_win_cache_sum < 0)
			{ // 这是第一个开始判断的平滑窗
				_first_win_cache_sum = 0;
				_second_win_cache_sum = 0;
				for(int f = 0; f< first_window_frames_num - 1 ; ++f)
				{
					_first_win_cache_sum += _pre_frames_judge[f];
				}
				for(int f = 0; f < second_window_frames_num - 1 ; ++f)
				{
					_second_win_cache_sum += _pre_frames_judge[f];
				}
			}
			// 开始平滑窗
			// 第一个窗求和
			for(int32 i = _vad_opt._left_context+_vad_opt._right_context;
					i < _pre_frames_judge.size() - _vad_opt._right_extra_context; ++i)
			{
				// 记录当前窗的语音帧个数
				BaseFloat first_win_sum = _first_win_cache_sum + _pre_frames_judge[i];
				// 记录当前窗的语音帧个数
				BaseFloat second_win_sum = _second_win_cache_sum + _pre_frames_judge[i+_vad_opt._right_extra_context];
				// 更新窗求和的值，减去窗最左边的值
				// 修改cache_sum
				_first_win_cache_sum = first_win_sum - 
					_pre_frames_judge[i-(_vad_opt._left_context+_vad_opt._right_context)];
				_second_win_cache_sum = second_win_sum - 
					_pre_frames_judge[i-(_vad_opt._left_context+_vad_opt._right_context)];
				KALDI_VLOG(5) << i << " first_win_sum : " << first_win_sum
					<< " second_win_sum : " << second_win_sum;
				if(_audio_flag == SIL) // current is silent.
				{
					if(first_win_sum > _vad_opt._win_nosil_frames_num ||
							first_win_sum > first_window_frames_num * _vad_opt._sil2audio_audio_ratio ||
							second_win_sum > second_window_frames_num * (_vad_opt._sil2audio_audio_ratio * 1.5)) 
					{ // 语音帧数大于设定的语音占比
						{
							judge_frames.push_back(AUDIO);
							_nosil_frames++;
							_audio_flag = AUDIO;
						}
					}
					else
					{
						_sil_frames++;
						judge_frames.push_back(SIL);
					}
				}
				else if(_audio_flag == AUDIO) // current is audio.
				{
					if(first_win_sum > first_window_frames_num * (1-_vad_opt._audio2sil_sil_ratio) ||
							second_win_sum > second_window_frames_num * (1-_vad_opt._audio2sil_sil_ratio))
					{// 语音帧数大于语音占比
						_nosil_frames++;
						judge_frames.push_back(AUDIO);
					}
					else
					{
						_sil_frames++;
						_audio_flag = SIL;
						judge_frames.push_back(SIL);
					}
				}
				else
				{
					KALDI_WARN << "It should not happen!!!";
				}

			} // 平滑窗 end.
			// 移除已经平滑完的帧
			int32 ok_frames = judge_frames.size();
			_pre_frames_judge.erase(_pre_frames_judge.begin(), _pre_frames_judge.begin()+ok_frames);
			_pre_frames_confidence.erase(_pre_frames_confidence.begin(),_pre_frames_confidence.begin()+ok_frames);

			AlignVad(judge_frames, _vad_ali, _cur_offset);
			_cur_offset += ok_frames;
		}
		return judge_frames;
	}

	std::vector<int32> V1SmoothVad(std::vector<BaseFloat> &frame_confidence, int32 end_flag = 0)
	{
		for(typename std::vector<BaseFloat>::iterator it = frame_confidence.begin();
				it != frame_confidence.end(); ++it)
		{
			if(*it > _vad_opt._nosil_threadhold)
			{
				_pre_frames_judge.push_back(1);
			}
			else
			{
				_pre_frames_judge.push_back(0);
			}
			// 如果是开始并且假设开始不是静音，补全左边能量和判断
			// Thr first frame, copy first frame to _left_context.
			if(_pre_frames_confidence.size() == 0)
			{
//				_data_flag = 1;
				_pre_frames_confidence = std::vector<BaseFloat>(_vad_opt._left_context, frame_confidence[0]);
				_pre_frames_judge = std::vector<int32>(_vad_opt._left_context+1, _pre_frames_judge[0]);
			}
		}
		if(frame_confidence.size() > 0)
			_pre_frames_confidence.insert(_pre_frames_confidence.end(),
					frame_confidence.begin(), frame_confidence.end());

		// The end data, copy the last frame to _right_context.
		if(end_flag != 0 && _pre_frames_confidence.size() > 0)
		{
			if(_vad_opt._end_sil == true)
			{
				std::vector<BaseFloat> tmp_confidence(_vad_opt._right_context, 0);
				_pre_frames_confidence.insert(_pre_frames_confidence.end(),
						tmp_confidence.begin(), tmp_confidence.end());
				std::vector<int32> tmp_judge(_vad_opt._right_context, SIL);
				_pre_frames_judge.insert(_pre_frames_judge.end(),tmp_judge.begin(), tmp_judge.end());
			}
			else
			{
				std::vector<BaseFloat> tmp_confidence(_vad_opt._right_context, _pre_frames_confidence.back());
				_pre_frames_confidence.insert(_pre_frames_confidence.end(),
						tmp_confidence.begin(), tmp_confidence.end());
				std::vector<int32> tmp_judge(_vad_opt._right_context, _pre_frames_judge.back());
				_pre_frames_judge.insert(_pre_frames_judge.end(), tmp_judge.begin(), tmp_judge.end());
			}
		}

		// now smooth vad and judge vad
		std::vector<int32> judge_frames;
		// 当前是语音时使用这个窗
		// 语音往静音转，采用更严格的要求，窗内静音占比要远大于语音，使用_vad_opt._audio2sil_sil_ratio
		int32 audio2sil_win_frames_num = _vad_opt._left_context + _vad_opt._nosil_right_context + 1; // 
		// 当前是静音采用这个窗
		// 静音往语音转，采用比较容易，语音占比和静音占比持平，使用_vad_opt._sil2audio_audio_ratio
		int32 sil2audio_win_frames_num = _vad_opt._sil_left_context + _vad_opt._right_context + 1;   //
		
		if(_pre_frames_judge.size() > _vad_opt._left_context + _vad_opt._right_context)
		{// 如果积攒的vad置信度帧数大于_left_context+_right_context，就可以进行平滑了
			if(_audio2sil_cache_sum < 0)
			{ // 这是第一个开始判断的平滑窗
				_audio2sil_cache_sum = 0;
				_sil2audio_cache_sum = 0;
				for(int f = 0; 
						f< _vad_opt._left_context + _vad_opt._nosil_right_context ; ++f)
				{
					_audio2sil_cache_sum += _pre_frames_judge[f];
				}
				for(int f = _vad_opt._left_context - _vad_opt._sil_left_context; f< _vad_opt._left_context + _vad_opt._right_context ; ++f)
				{
					_sil2audio_cache_sum += _pre_frames_judge[f];
				}
			}
			// 开始平滑窗
			for(int32 i = _vad_opt._left_context+_vad_opt._right_context;
					i < _pre_frames_judge.size(); ++i)
			{
				// 当前是语音，记录当前窗的语音帧个数
				BaseFloat audio2sil_win_sum = _audio2sil_cache_sum + _pre_frames_judge[i-_vad_opt._right_context+_vad_opt._nosil_right_context];
				// 当前是静音，记录当前窗的语音帧个数
				BaseFloat sil2audio_win_sum = _sil2audio_cache_sum + _pre_frames_judge[i];
				if(_audio_flag == SIL) // current is silent.
				{
					if(sil2audio_win_sum > sil2audio_win_frames_num * _vad_opt._sil2audio_audio_ratio)
					{ // 语音帧数大于语音占比
						judge_frames.push_back(AUDIO);
						_nosil_frames++;
						_audio_flag = AUDIO;
					}
					else
					{
						_sil_frames++;
						judge_frames.push_back(SIL);
					}
				}
				else if(_audio_flag == AUDIO) // current is audio.
				{
					if(audio2sil_win_sum > audio2sil_win_frames_num * (1-_vad_opt._audio2sil_sil_ratio))
					{// 语音帧数大于语音占比
						_nosil_frames++;
						judge_frames.push_back(AUDIO);
					}
					else
					{
						_sil_frames++;
						_audio_flag = SIL;
						judge_frames.push_back(SIL);
					}
				}
				else
				{
					KALDI_WARN << "It should not happen!!!";
				}
				// 更新窗求和的值，减去窗最左边的值
				_audio2sil_cache_sum = audio2sil_win_sum - 
					_pre_frames_judge[i-_vad_opt._left_context - _vad_opt._right_context];
				_sil2audio_cache_sum = sil2audio_win_sum - 
					_pre_frames_judge[i-_vad_opt._sil_left_context-_vad_opt._right_context];

			} // 平滑窗 end.
			// 移除已经平滑完的帧
			int32 ok_frames = judge_frames.size();
			_pre_frames_judge.erase(_pre_frames_judge.begin(), _pre_frames_judge.begin()+ok_frames);
			_pre_frames_confidence.erase(_pre_frames_confidence.begin(),_pre_frames_confidence.begin()+ok_frames);

			AlignVad(judge_frames, _vad_ali, _cur_offset);
			_cur_offset += ok_frames;
		}
		return judge_frames;
	}


	void AlignVad(std::vector<int32> &judge_frames, VadSeg &vad_ali,
			int32 offset = 0)
	{
		int32 prev_last_frame_flag = -1; 
		if(vad_ali.size() > 0)
		{ // 之前语音标志
			prev_last_frame_flag = vad_ali.back().first;
		}

		int32 start_frame = offset;
		int32 end_frame = 0;
		for(int32 n = 0 ; n<judge_frames.size() ; ++n)
		{
			if(n==0 && prev_last_frame_flag == judge_frames[n])
			{ // 如果之前的结果和当前一样，继续补充帧数
				std::pair<int32, std::pair<int32 ,int32> > prev_res = vad_ali.back();
				vad_ali.pop_back();
				start_frame = prev_res.second.first;
			}
			if(prev_last_frame_flag == -1)
			{
				prev_last_frame_flag = judge_frames[0];
			}
			if(prev_last_frame_flag != judge_frames[n])
			{
				if(n == 0)
				{ //说明之前最后一个包和当前第一个包标记不一样
					start_frame = n + offset;
					prev_last_frame_flag = judge_frames[n];
				}
				else
				{
					end_frame = n + offset;
					// 静音非静音之间转换
					vad_ali.push_back(std::make_pair(prev_last_frame_flag,
								std::make_pair(start_frame, end_frame)));
					start_frame = n + offset;
					prev_last_frame_flag = judge_frames[n];
				}
			}
			if(n == judge_frames.size()-1)
			{
				end_frame = n + 1 + offset;
				vad_ali.push_back(std::make_pair(prev_last_frame_flag,
							std::make_pair(start_frame, end_frame)));
				prev_last_frame_flag = judge_frames[n];
			}
		}
	}
private:
	const VadJudgeOptions &_vad_opt;
	
	// 为了加速，_audio2sil_cache_sum和_sil2audio_cache_sum
	// 记录2中转换窗内的0 -> (n-1) 个自信度的和,
	// 每次记录之后，减去第一帧，再加上最新一帧
	// 之前一帧是语音采用 _audio2sil_cache_sum
	// 初始化为-1
	BaseFloat _audio2sil_cache_sum;
	// 之前一帧是静音采用 _sil2audio_cache_sum
	// 初始化为-1
	BaseFloat _sil2audio_cache_sum;

	// V2 use
	int32 _first_win_cache_sum;
	int32 _second_win_cache_sum;

	int32 _audio_flag; // 0 sil, 1 audio 记录当前帧是否为语音，为了判断

	int32 _nosil_frames;
	int32 _sil_frames;


	std::vector<BaseFloat> _pre_frames_confidence;
	std::vector<int32> _pre_frames_judge;
	//int32 _data_flag; // 0 is first, 1 is middle , 2 is end;
	
	VadSeg _vad_ali;
	int32 _cur_offset;

};

struct VadNnetSimpleLoopedComputationOptions
{
public:
	nnet3::NnetSimpleLoopedComputationOptions _vad_nnet3_config;
	bool _apply_exp;
public:
	VadNnetSimpleLoopedComputationOptions():_apply_exp(true) { }

	void Register(OptionsItf *opts)
	{
		// register the vad options with the prefix "vad-nnet"
		ParseOptions vad_opts("vad-nnet", opts);
		_vad_nnet3_config.Register(&vad_opts);
		vad_opts.Register("apply-exp", &_apply_exp, 
				"If true, apply exp function to vad model output");
	}
};

namespace nnet3
{
class Decodable2NnetLoopedOnline: public DecodableNnetLoopedOnline
{
public:
	// 因为需要获取特征，所以提供GetFeatureFrame接口
	Decodable2NnetLoopedOnline(
			const DecodableNnetSimpleLoopedInfo &info,
			OnlineFeatureInterface *input_features,
			OnlineFeatureInterface *ivector_features):
		DecodableNnetLoopedOnline(info, input_features, ivector_features), 
		_input_features(input_features)	{ }

	void GetOutputForFrame(int32 subsampled_frame, VectorBase<BaseFloat> *output)
	{
		subsampled_frame += frame_offset_;
		EnsureFrameIsComputed(subsampled_frame);
		output->CopyFromVec(current_log_post_.Row(
					subsampled_frame - current_log_post_subsampled_offset_));
	}

	void GetFeatureFrame(int32 frame, VectorBase<BaseFloat> *feat)
	{
		_input_features->GetFrame(frame, feat);
	}
	OnlineFeatureInterface *GetInputFeatures()
	{
		return _input_features;
	}
private:

	OnlineFeatureInterface *_input_features;
	KALDI_DISALLOW_COPY_AND_ASSIGN(Decodable2NnetLoopedOnline);
};

} // namespace nnet3
class VadNnetInfo
{
public:
	VadNnetInfo(const VadNnetSimpleLoopedComputationOptions &vad_nnet3_config,
			nnet3::AmNnetSimple *am_nnet):
		_decodable_info(vad_nnet3_config._vad_nnet3_config, am_nnet) { }

	VadNnetInfo(const VadNnetSimpleLoopedComputationOptions &vad_nnet3_config,
			nnet3::Nnet *nnet):
		_decodable_info(vad_nnet3_config._vad_nnet3_config, nnet) { }

	nnet3::DecodableNnetSimpleLoopedInfo _decodable_info;
};

class VadNnet3
{
public:
	VadNnet3(const VadNnetInfo &vad_nnet_info,
			OnlineFeatureInterface *base_feature, 
			const VadJudgeOptions &vad_opt,
			int32 vad_frame_offset = 0,
			int32 nnet_frame_offset = 0,
			bool apply_exp=true):
		_decodable_info(vad_nnet_info._decodable_info),
		_vad_model(vad_nnet_info._decodable_info, base_feature, NULL),
		_vad_frame_offset(vad_frame_offset),
		_nnet_frame_offset(nnet_frame_offset),
		_vad_opt(vad_opt), _vad_judge(_vad_opt), _apply_exp(apply_exp) { }

	VadSeg ComputerNnet(int32 end_flag=0, bool print_info=false)
	{
		int32 outdim = _decodable_info.output_dim;
		std::vector<BaseFloat> frame_confidence;
		int32 number_ready = _vad_model.NumFramesReady();

		for(int32 n = _nnet_frame_offset; n < number_ready; ++n)
		{
			Vector<BaseFloat> row(outdim);
			_vad_model.GetOutputForFrame(n, &row);
			_nnet_frame_offset++;
			if(_apply_exp == true)
				row.ApplyExp();


			if(true)
				frame_confidence.push_back(row(1));
			else
			{
				if(row(1) > _vad_opt._nosil_threadhold)
					frame_confidence.push_back(1);
				else
					frame_confidence.push_back(0);
			}
			if(false)
			{
				std::cout << n << " : " ;
				for(int32 i=0; i< row.Dim(); ++i)
				{
					std::cout << row(i) << " " ;
				}
				std::cout << std::endl;
			}
		}

		std::vector<int32> smooth_vad = _vad_judge.SmoothVad(frame_confidence, end_flag);
		VadSeg vad_ali;
		if(smooth_vad.size() > 0)
		{
			_vad_judge.AlignVad(smooth_vad, vad_ali, _vad_frame_offset);
			_vad_frame_offset += smooth_vad.size();
			PrintVadSeg(vad_ali,"current -> ", 3);
			if(end_flag != 0 && print_info == true)
			{
				int32 sil_frames =0 , nosil_frames = 0 ;
				VadSeg *pvad_ali;
				_vad_judge.GetInfo(sil_frames, nosil_frames, &pvad_ali);
				KALDI_VLOG(1) << "sil frames is : " << sil_frames 
					<< " nosil frames is : " << nosil_frames ;
				PrintVadSeg(*pvad_ali, "all-align -> ", 0);
			}
		}
		return vad_ali;
	}

	// 按照给出的begin_input_frame，end_input_frame给出特征，
	// 这里希望end_input_frame < input_features->NumFramesReady()
	void GetFeatureMatrix(Matrix<BaseFloat> &features, 
			int32 begin_input_frame, int32 end_input_frame)
	{
		OnlineFeatureInterface *input_features = _vad_model.GetInputFeatures();
		int32 num_feature_frames_ready = input_features->NumFramesReady();
		if(num_feature_frames_ready < end_input_frame)
		{// It shouldn't happen.
			return ;
		}
		Matrix<BaseFloat> this_feats(end_input_frame - begin_input_frame,
				input_features->Dim());
		for(int32 i=begin_input_frame; i<end_input_frame ; ++i)
		{
			SubVector<BaseFloat> this_row(this_feats, i - begin_input_frame);
			int32 input_frame = i;
			if (input_frame < 0) 
				input_frame = 0;
			if (input_frame >= num_feature_frames_ready)
				input_frame = num_feature_frames_ready - 1;
			input_features->GetFrame(input_frame, &this_row);
		}
		features.Swap(&this_feats);
	}

	void GetTotalVadAli(VadSeg &vad_ali, int32 &sil_frames, int32 &nosil_frames,
			bool compress_vad_ali=false, int32 min_sil_frames_interval = 50)
	{
		VadSeg *vad_ali_tmp;
		_vad_judge.GetInfo(sil_frames, nosil_frames, &vad_ali_tmp);
		if(compress_vad_ali == true)
		{
			CompressAlignVad(*vad_ali_tmp, vad_ali, min_sil_frames_interval);
			sil_frames = 0;
			nosil_frames = 0;
			for(size_t i=0 ; i < vad_ali.size(); ++i)
			{
				int32 seg_len = vad_ali[i].second.second - vad_ali[i].second.first;
				if(vad_ali[i].first == SIL)
					sil_frames += seg_len;
				else if(vad_ali[i].first == AUDIO)
					nosil_frames += seg_len;
			}
		}
		else
		{
			vad_ali = *vad_ali_tmp;
		}
	}

	void Reset()
	{
		_vad_judge.Reset();
	}
private:

	const nnet3::DecodableNnetSimpleLoopedInfo &_decodable_info;
	// 模型vad use Decodable2NnetLoopedOnline
	nnet3::Decodable2NnetLoopedOnline _vad_model;
	// 由于网络计算帧会比vad判断帧提前，所以分别记录，最后2个值应该一样
	int32 _vad_frame_offset;
	int32 _nnet_frame_offset;
	// vad context
	const VadJudgeOptions &_vad_opt;
	VadJudge _vad_judge;

	bool _apply_exp;
	int32 _left_context;
	int32 _right_context;

	int32 _sil_frames;
	int32 _nosil_frames;

};


// 能量vad选项
struct EnergyVadOpts
{
};

// 能量vad实现
template<class T>
class V1EnergyVad
{
public:
	V1EnergyVad(const VadJudgeOptions &vad_opt,
			std::string cal_method="sum_square_root",
			int32 samp_freq=16000, 
			BaseFloat frame_length_s=0.025,
			BaseFloat frame_shift_s = 0.01, 
			int32 vad_frame_offset = 0,
			BaseFloat energy_threshold1 = 32768*0.005,
			BaseFloat energy_threshold2 = 32768*0.05):
		_cal_method(cal_method),
		_samp_freq(samp_freq), _frame_length_s(frame_length_s), 
		_frame_shift_s(frame_shift_s),
		_vad_frame_offset(vad_frame_offset),
		_energy_threshold1(energy_threshold1),
		_energy_threshold2(energy_threshold2),
		_vad_opt(vad_opt),
		_vad_judge(vad_opt)
	{
		_frame_length_samp = _samp_freq * _frame_length_s;
		_frame_shift_samp = _samp_freq * _frame_shift_s;
		_cur_data.clear();
		_tot_data_cache.clear();
		_vad_judge.Reset();
	}

	void Reset(int32 vad_frame_offset = 0)
	{
		_cur_data.clear();
		_tot_data_cache.clear(); 
		_vad_judge.Reset();
		_vad_frame_offset = vad_frame_offset;
	}

public:
	typedef typename std::vector<T>::iterator Titer;

	void AcceptData(const VectorBase<T> &wav_data)
	{
		_cur_data.insert(_cur_data.end(), wav_data.Data(), wav_data.Data()+ wav_data.Dim());
		_tot_data_cache.insert(_tot_data_cache.end(), wav_data.Data(), wav_data.Data()+ wav_data.Dim());
	}

	VadSeg ComputerVad(int32 end_flag=0, bool print_info=false)
	{
		VadSeg cur_vad_ali;
		std::vector<BaseFloat> frames_energy = FramesEnergy();
		std::vector<BaseFloat> frame_confidence = FramesConfidence(frames_energy);
		std::vector<int32> smooth_vad = _vad_judge.SmoothVad(frame_confidence, end_flag);
		if(smooth_vad.size() > 0)
		{
			_vad_judge.AlignVad(smooth_vad, cur_vad_ali, _vad_frame_offset);
			_vad_frame_offset += smooth_vad.size();
			PrintVadSeg(cur_vad_ali,"current -> ", 3);
			if(end_flag != 0 && print_info == true)
			{
				int32 sil_frames =0 , nosil_frames = 0 ;
				VadSeg *pvad_ali;
				_vad_judge.GetInfo(sil_frames, nosil_frames, &pvad_ali);
				KALDI_VLOG(1) << "sil frames is : " << sil_frames
					<< " nosil frames is : " << nosil_frames ;
				PrintVadSeg(*pvad_ali, "all-align -> ", 0);				
			}
		}
		return cur_vad_ali;
	}

	// 如果是最后一段，将结尾的帧补齐，last_seg=true
	void GetData(std::vector<T> &wav_data,
			int32 start_frame, int32 end_frame,
		   	bool last_seg = false)
	{
		int32 start_offset = start_frame * _frame_shift_samp;
		int32 end_offset = end_frame * _frame_shift_samp;
		if(last_seg == true)
			end_offset += _frame_length_samp - _frame_shift_samp;
		wav_data.clear();
		wav_data.insert(wav_data.begin(), 
				_tot_data_cache.begin() + start_offset,
				_tot_data_cache.begin()+end_offset);
	}

	void GetTotalVadAli(VadSeg &vad_ali, int32 &sil_frames, int32 &nosil_frames,
			bool compress_vad_ali=false, int32 min_sil_frames_interval = 50)
	{
		VadSeg *vad_ali_tmp;
		_vad_judge.GetInfo(sil_frames, nosil_frames, &vad_ali_tmp);
		if(compress_vad_ali == true)
		{
			CompressAlignVad(*vad_ali_tmp, vad_ali, min_sil_frames_interval);
			sil_frames = 0;
			nosil_frames = 0;
			for(size_t i=0 ; i < vad_ali.size(); ++i)
			{
				int32 seg_len = vad_ali[i].second.second - vad_ali[i].second.first;
				if(vad_ali[i].first == SIL)
					sil_frames += seg_len;
				else if(vad_ali[i].first == AUDIO)
					nosil_frames += seg_len;
			}
		}
		else
		{
			vad_ali = *vad_ali_tmp;
		}
	}
private:
	std::vector<BaseFloat> FramesConfidence(const std::vector<BaseFloat> &frames_energy)
	{
		std::vector<BaseFloat> frames_confidence;
		for(typename std::vector<BaseFloat>::const_iterator it=frames_energy.begin();
				it != frames_energy.end(); ++it)
		{
			if(*it < _energy_threshold1)
				frames_confidence.push_back(0);
			else if(*it < _energy_threshold2)
				frames_confidence.push_back(1);
			else
				frames_confidence.push_back(2);
		}
		return frames_confidence;
	}
	std::vector<BaseFloat> FramesEnergy()
	{
		std::vector<BaseFloat> frames_energy;
		int outframes = ((int)(_cur_data.size())-_frame_length_samp+_frame_shift_samp)/_frame_shift_samp;
		if(outframes <= 0)
			return frames_energy;
		for(int i = 0 ; i < outframes ; ++i)
		{
			typename std::vector<T>::iterator it = _cur_data.begin();
			BaseFloat cur_energy = OneFrameEnergy(it+i*_frame_shift_samp, it+i*_frame_shift_samp + _frame_length_samp);
			frames_energy.push_back(cur_energy);
		}
		// erase energy frame data
		_cur_data.erase(_cur_data.begin(), _cur_data.begin()+ outframes*_frame_shift_samp);
		KALDI_VLOG(3) << "outframes : " << outframes ;
		return frames_energy;
	}

private:
	// 计算窗内能量
	BaseFloat OneFrameEnergy(Titer start,
			Titer end)
	{
		BaseFloat energy_sum = 0;
		for(Titer it = start; it != end; ++it)
		{
			if(_cal_method == "sum_square_root")
			{
				energy_sum += *it * *it;
			}
			else if(_cal_method == "sum_abs")
			{
				energy_sum += fabsf(*it);
			}
			else
			{
				KALDI_WARN << "No this cal_method -> " << _cal_method;
				return -1;
			}
		}
		if(_cal_method == "sum_square_root")
			return sqrtf(energy_sum/_frame_length_samp);
		else if(_cal_method == "sum_abs")
			return energy_sum / _frame_length_samp;
		else
			return -1;
	}
private:
	std::string _cal_method;
	int _samp_freq;
	float _frame_length_s;
	float _frame_shift_s;
	int _frame_length_samp;
	int _frame_shift_samp;

	// need reset
	int32 _vad_frame_offset;
	BaseFloat _energy_threshold1;
	BaseFloat _energy_threshold2;
	// 记录当前未处理数据
	std::vector<T> _cur_data;
	// 记录所有已经处理过的数据，这里不进行清空，直到当前vad结束
	std::vector<T> _tot_data_cache;

	// vad context
	const VadJudgeOptions &_vad_opt;
	VadJudge _vad_judge;
};

template <typename FST>
class SingleUtteranceNnet3DecoderVadTpl
{
public:
private:
	// 声学模型
	nnet3::DecodableAmNnetLoopedOnline _decodable;

	// 解码器
	const LatticeFasterDecoderConfig &_decoder_opts;
	LatticeFasterOnlineDecoderTpl<FST> _decoder;
	const TransitionModel &_trans_model;
};
} // namespace kaldi

#endif

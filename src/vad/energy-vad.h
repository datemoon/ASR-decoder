#ifndef __ENERGY_VAD_H__
#define __ENERGY_VAD_H__
#include <math.h>
#include <vector>
#include <string>
#include <iostream>
#include "src/util/log-message.h"
//#define LOG_WARN std::cerr
#include "src/util/namespace-start.h"

class VadInterface
{
public:
	VadInterface(int samp_freq, float frame_length_s, float frame_shift_s)
		:_samp_freq(samp_freq), _frame_length_s(frame_length_s),
		_frame_shift_s(frame_shift_s)
	{ 
		_frame_length_samp = _samp_freq * _frame_length_s;
		_frame_shift_samp = _samp_freq * _frame_shift_s;
   	}

	virtual ~VadInterface() { }
protected:
	int _samp_freq;
	float _frame_length_s;
	float _frame_shift_s;
	int _frame_length_samp;
	int _frame_shift_samp;
};

template<typename T>
class EnergyVad: public VadInterface
{
public:
	enum { SIL = 0, AUDIO = 1};
	EnergyVad(int samp_freq, float frame_length_s,
			float frame_shift_s,
			int prev_audio_flag = -1,
			int getdata_audio_flag = SIL,
			float sil2audio_ratio = 0.5,
			float audio2sil_ratio = 0.8,
			int left_frames=5,
			int right_frames=5,
			float energy_threshold1=32768*0.01,
			float energy_threshold2=32768*0.1,
			//std::string data_type="short",
			std::string cal_method = "sum_square_root"): // "sum_abs"
		VadInterface(samp_freq, frame_length_s, frame_shift_s), 
	   	_cal_method(cal_method),
		_data_flag(0),
		_audio2sil_cache_sum(-1),
		_sil2audio_cache_sum(-1),
		_audio_flag(SIL),
		_prev_audio_flag(prev_audio_flag),
		_getdata_audio_flag(getdata_audio_flag),
		_sil_frames(0),
		_nosil_frames(0),
		_frames_offset(0),
		_sil2audio_ratio(sil2audio_ratio),
		_audio2sil_ratio(audio2sil_ratio),
		_left_frames(left_frames),
		_sil_left_frames(_left_frames*0.5),
		_right_frames(right_frames),
		_energy_threshold1(energy_threshold1),
		_energy_threshold2(energy_threshold2)
	//	_data_type(data_type),
	//	_pre_judge_frames(left_frames, 0),
 	//	_pre_frames_energy(left_frames, 0.0) 
		{ }

	typedef typename std::vector<T>::iterator Titer;

	float OneFrameEnergy(Titer start,
			Titer end)
	{
		float energy_sum = 0.0;
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
				LOG_WARN << "No this cal_method -> " << _cal_method;
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

	std::vector<float> FramesEnergy(std::vector<T> &data)
	{
		std::vector<float> frames_energy;
		if(data.size() == 0)
			return frames_energy;
		// 新数据拼接到原有数据
		_data_cache.insert(_data_cache.end(), data.begin(), data.end());
		_tot_data_cache.insert(_tot_data_cache.end(), data.begin(), data.end());

		int outframes = ((int)(_data_cache.size())-_frame_length_samp+_frame_shift_samp)/_frame_shift_samp;
		if (outframes <= 0)
			return frames_energy;
		for(int i=0; i< outframes; ++i)
		{
			typename std::vector<T>::iterator it = _data_cache.begin();
			float cur_energy = OneFrameEnergy(it+i*_frame_shift_samp, it+i*_frame_shift_samp + _frame_length_samp);
			frames_energy.push_back(cur_energy);
		}
		// erase energy frames
		_data_cache.erase(_data_cache.begin(), _data_cache.begin() + outframes*_frame_shift_samp);
		VLOG_COM(2) << "outframes : " << outframes ;
		return frames_energy;
	}

	std::vector<int> JudgeFramesFromEnergy(std::vector<float> &frames_energy, int end_flag = 0)
	{
		for(typename std::vector<float>::iterator it=frames_energy.begin();
				it != frames_energy.end(); ++it)
		{
			if(*it < _energy_threshold1)
				_pre_judge_frames.push_back(0);
			else if(*it < _energy_threshold2)
				_pre_judge_frames.push_back(1);
			else
				_pre_judge_frames.push_back(2);
			// The first frame ,copy first frame to _left_frames
			if(_data_flag == 0)
			{
				_data_flag = 1;
				_pre_frames_energy = std::vector<float>(_left_frames, frames_energy[0]);
				_pre_judge_frames = std::vector<int>(_left_frames+1, _pre_judge_frames[0]);
			}
		}
		if(frames_energy.size() > 0)
			_pre_frames_energy.insert(_pre_frames_energy.end(),
					frames_energy.begin(), frames_energy.end());

		// The end data, copy the last frame to _right_frames
		if(end_flag != 0 && _pre_frames_energy.size() > 0)
		{
			_data_flag = 2;
			std::vector<float> tmp_energy(_right_frames, _pre_frames_energy.back());
			_pre_frames_energy.insert(_pre_frames_energy.end(), tmp_energy.begin(), tmp_energy.end());
			std::vector<int> tmp_judge(_right_frames, _pre_judge_frames.back());
			_pre_judge_frames.insert(_pre_judge_frames.end(), tmp_judge.begin(), tmp_judge.end());
		}

		// now judge vad
		std::vector<int> judge_frames;
		int audio2sil_win_frames_num = _left_frames+_right_frames+1;     // big win
		int sil2audio_win_frames_num = _sil_left_frames+_right_frames+1; // samll win
		if(_pre_judge_frames.size() > _left_frames + _right_frames )
		{
			if(_audio2sil_cache_sum < 0)
			{ // the first data package
				_audio2sil_cache_sum = 0;
				for(int f = 0; f< audio2sil_win_frames_num-1 ; ++f)
				{
					_audio2sil_cache_sum += _pre_judge_frames[f];
				}
				_sil2audio_cache_sum = 0;
				for(int f = _left_frames-_sil_left_frames; f< audio2sil_win_frames_num-1 ; ++f)
				{
					_sil2audio_cache_sum += _pre_judge_frames[f];
				}
			}
			for(int i = _left_frames+_right_frames; i < _pre_judge_frames.size(); ++i)
			{
				int audio2sil_win_sum = _audio2sil_cache_sum + _pre_judge_frames[i];
				int sil2audio_win_sum = _sil2audio_cache_sum + _pre_judge_frames[i];
				if(_audio_flag == 0) // current is silent
				{
					if(sil2audio_win_sum > sil2audio_win_frames_num * _sil2audio_ratio)
					{
						judge_frames.push_back(1);
						_nosil_frames++;
						_audio_flag = 1;
					}
					else
					{
						_sil_frames++;
						judge_frames.push_back(0);
					}
				}
				else if(_audio_flag == 1) // current is audio
				{
					// 语音占比大于1-_audio2sil_ratio，则还是语音
					if(audio2sil_win_sum > audio2sil_win_frames_num * (1-_audio2sil_ratio))
					{
						_nosil_frames++;
						judge_frames.push_back(1);
					}
					else
					{
						_sil_frames++;
						judge_frames.push_back(0);
						_audio_flag = 0;
					}
				}
				else
				{
					LOG_WARN << "It should not happen!!!";
				}
				// 更新窗求和的值，减去窗最左边的值
				_audio2sil_cache_sum = audio2sil_win_sum - _pre_judge_frames[i-_left_frames-_right_frames];
				_sil2audio_cache_sum = sil2audio_win_sum - _pre_judge_frames[i-_sil_left_frames-_right_frames];
			}
			// 移除已经算完的帧
			int ok_frames = judge_frames.size();
			_pre_judge_frames.erase(_pre_judge_frames.begin(),_pre_judge_frames.begin()+ok_frames);
			_pre_frames_energy.erase(_pre_frames_energy.begin(),_pre_frames_energy.begin()+ok_frames);
		}
		return judge_frames;
	}

	std::vector<int> JudgeFrames(std::vector<T> &data, int end_flag = 0)
	{
		std::vector<float> frames_energy = FramesEnergy(data);
		return JudgeFramesFromEnergy(frames_energy, end_flag);
	}

	//                   audio_flag    start, end frame
	typedef std::vector<std::pair<int, std::pair<int ,int> > > VadSeg;

	VadSeg &CompressVadRes(std::vector<int> judge_frames)
	{
		int start_frame = 0;
		int end_frame = 0;
		for(int n=0;n<judge_frames.size();++n)
		{
			if(_prev_audio_flag == -1)
			{
				_prev_audio_flag = judge_frames[n];
				start_frame = n;
			}
			if(_prev_audio_flag != judge_frames[n])
			{
				if(n == 0)
				{ //说明之前最后一个包和当前第一个包标记不一样
					start_frame = n;
					_prev_audio_flag = judge_frames[n];
				}
				else
				{
					end_frame = n ;
					// 静音非静音之间转换
					_vad_res.push_back(std::make_pair(_prev_audio_flag, 
								std::make_pair(start_frame, end_frame)));
					start_frame = n ;
					_prev_audio_flag = judge_frames[n];
				}
			}
			if(n == judge_frames.size()-1)
			{
				end_frame = n + 1;
				_vad_res.push_back(std::make_pair(_prev_audio_flag,
							std::make_pair(start_frame, end_frame)));
				_prev_audio_flag = judge_frames[n];
			}
		}
		return _vad_res;
	}

	VadSeg &JudgeFramesRes(std::vector<T> &data, int end_flag = 0)
	{
		std::vector<float> frames_energy = FramesEnergy(data);
		std::vector<int> judge_frames = JudgeFramesFromEnergy(frames_energy, end_flag);
		return CompressVadRes(judge_frames);
	}

	// SIL,AUDIO,SIL,AUDIO...
	// 获取语音数据
	// return:
	//    n : 还有n段数据
	//    0 : 没有数据
	int GetData(std::vector<T> &out_data, int &getdata_audio_flag)
	{
		out_data.clear();
		Print(_vad_res);
		int del_end = 0;
		while(true)
		{
			if(_vad_res.size() > 0)
			{
				// 如果之前是上一次调用CompressVadRes的最后是语音帧
				if(_getdata_audio_flag == AUDIO)
				{ 
					int start = _vad_res[0].second.first * _frame_shift_samp;
					int end = 0;
					// 当前是静音帧，将最后一帧缺少的提供出去
				   	if(_vad_res[0].first == SIL && start == 0)
					{
						end = _frame_length_samp - _frame_shift_samp;
						_getdata_audio_flag = SIL;
					}
					else if(_vad_res[0].first == AUDIO)
					{
						// 当前是语音，
						// 如果接下来的是静音，那么补全语音帧
						// 如果是最后一段，并且已经结束，也补全语音帧
						if((_vad_res.size() > 1 && _vad_res[1].first == SIL) 
								|| (_vad_res.size() == 1 && _data_flag == 2))
						{
							end = _vad_res[0].second.second*_frame_shift_samp +
								_frame_length_samp - _frame_shift_samp;
						}
						else
						{
							end = _vad_res[0].second.second*_frame_shift_samp;
						}
						_getdata_audio_flag = AUDIO;
					}
					out_data.insert(out_data.end(),
							_tot_data_cache.begin()+ start,_tot_data_cache.begin()+end);
					del_end = (_vad_res[0].second.second)*_frame_shift_samp;
					// 删除当前段数据
					_vad_res.erase(_vad_res.begin());
					if(_vad_res.size() > 0)
					{
						// 如果接下来是SIL，删掉
						if(_vad_res[0].first == SIL)
						{
							del_end = (_vad_res[0].second.second)*_frame_shift_samp;
							_vad_res.erase(_vad_res.begin());
							_getdata_audio_flag = SIL;
						}
					}
				} //if(_getdata_audio_flag == AUDIO)
				// 如果上一次调用是非语音帧
				else if(_getdata_audio_flag == SIL)
				{
					int start = _vad_res[0].second.first * _frame_shift_samp;
					int end = 0;
					// 之前是静音，现在也是静音，跳过静音段
					if(_vad_res[0].first == SIL)
					{
						_vad_res.erase(_vad_res.begin());
						del_end = (_vad_res[0].second.second)*_frame_shift_samp;
						_getdata_audio_flag = SIL;
						continue;
					}
					else
					{ // 之前是静音，现在是语音
						// 当前是语音，
						// 如果接下来的是静音，那么补全语音帧
						// 如果是最后一段，并且已经结束，也补全语音帧
						if((_vad_res.size() > 1 && _vad_res[1].first == SIL) 
								|| (_vad_res.size() == 1 && _data_flag == 2))
						{
							end = _vad_res[0].second.second*_frame_shift_samp +
								_frame_length_samp - _frame_shift_samp;
						}
						else
						{
							end = _vad_res[0].second.second*_frame_shift_samp;
						}
						_getdata_audio_flag = AUDIO;
						out_data.insert(out_data.end(), 
								_tot_data_cache.begin()+start,
								_tot_data_cache.begin()+end);
						del_end = (_vad_res[0].second.second)*_frame_shift_samp;
						// 删除已经拷贝的当前段记录
						_vad_res.erase(_vad_res.begin()); 
						if(_vad_res.size() > 0)
						{
							// 如果接下来是SIL，删掉
							if(_vad_res[0].first == SIL)
							{
								del_end = (_vad_res[0].second.second)*_frame_shift_samp;
								_vad_res.erase(_vad_res.begin());
								_getdata_audio_flag = SIL;
							}
						}
					}
				} //else if(_getdata_audio_flag == SIL)
			}
			// 如果是最后一段标记音频，删除掉缓存音频，因为已经全部返回完毕
			if(_vad_res.size() == 0 && del_end > 0)
			{
				_tot_data_cache.erase(_tot_data_cache.begin(), _tot_data_cache.begin()+del_end);
				VLOG_COM(2) << "_data_cache.erase del_end : " << del_end ;
				_frames_offset += del_end / _frame_shift_samp;
			}
			getdata_audio_flag = _getdata_audio_flag;
			return _vad_res.size();
		}// while
	}

	void Print(VadSeg &vad_res)
	{
		for(auto it = vad_res.begin(); it != vad_res.end(); ++it)
		{
			std::string type;
			if(it->first == SIL)
				type = "sil";
			else
				type = "nosil";
			VLOG_COM(2) << type << " " << it->second.first+_frames_offset << " " 
				<< it->second.second+_frames_offset << " " 
				<< it->second.second-it->second.first;
		}
	}

	void GetSilAndNosil(int &sil_frames, int &nosil_frames)
	{
		sil_frames = _sil_frames;
		nosil_frames = _nosil_frames;
	}

	void Reset()
	{
		_pre_frames_energy.clear();
		_pre_judge_frames.clear();
		_data_cache.clear();
		_tot_data_cache.clear();
		_vad_res.clear();
		_pre_frames_energy = std::vector<float>(_left_frames, 0.0);
		_pre_judge_frames = std::vector<int>(_left_frames, 0);
		_data_flag = SIL;
		_prev_audio_flag = -1;
		_getdata_audio_flag = SIL; // 设置每次初始为静音
		_audio2sil_cache_sum = -1;
		_sil2audio_cache_sum = -1;
		_audio_flag = SIL;
		_sil_frames = 0;
		_nosil_frames = 0;
		_frames_offset = 0;
	}
	~EnergyVad() { }
private:
	//std::string _data_type;
	std::string _cal_method;
	// need reset
	std::vector<T> _data_cache;
	std::vector<T> _tot_data_cache;

	std::vector<float> _pre_frames_energy;
	std::vector<int> _pre_judge_frames;
	VadSeg _vad_res;

	int _data_flag; // 0 is first, 1 is middle , 2 is end;
	int _audio2sil_cache_sum; // cache _pre_judge_frames[_left_frames + 1 + _right_frames - 1]  sum
	int _sil2audio_cache_sum; // cache _pre_judge_frames[_sil_left_frames + 1 + _right_frames - 1]  sum
	int _audio_flag; // 0 sil, 1 audio 记录当前帧是否为语音，为了判断
	int _prev_audio_flag; // 0 sil, 1 audio
	int _getdata_audio_flag; // init SIL

	int _sil_frames;
	int _nosil_frames;
	int _frames_offset;

	float _sil2audio_ratio;  // audio:win > 0.5
	float _audio2sil_ratio;  // sil:win > 0.8
	

	int _left_frames;
	int _sil_left_frames; // 0.4 * _left_frames
	int _right_frames;
	float _energy_threshold1;
	float _energy_threshold2;

};
#include "src/util/namespace-end.h"

#endif

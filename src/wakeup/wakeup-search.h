/*
 * Copyright 2017-2018
 *
 * File Type : C++ Source File(*.cpp)
 * File Name : wakeup-search.h
 * Module    :
 * Create on : 2018/1/24
 * Author    : hubo
 *
 * This class is used wake up search .
 * */


#ifndef __WAKEUP_SEARCH_H__
#define __WAKEUP_SEARCH_H__

#include <vector>
#include <cassert>
#include "dtw.h"

using namespace std;

class WakeupSearch
{
public:
	WakeupSearch(bool adjust=false):adjust_(adjust),cur_frame_(0),max_frame_len_(0), dtw_(NULL) {}
	~WakeupSearch()
	{
		delete dtw_;
	}

	void Init(vector<int> &states, vector<float> &weights, int max_frame_len)
	{
		assert(states.size() == weights.size());
		int state_len = states.size();
		states_.resize(state_len);
		weight_.resize(state_len);
		align_states_.resize(state_len);
		for(int i=0;i<states.size();++i)
		{
			states_[i] = states[i];
			weight_[i] = weights[i];
		}
		if (dtw_ != NULL)
			delete dtw_;
		dtw_ = new DtwAlign(max_frame_len, state_len);
		dtw_->Init();
		max_frame_len_ = max_frame_len;
	}
	// add data
	void InputDataOneFrame(float *data, int cols);

	float ProcessData(float *data, int rows, int cols);

	float JudgeWakeup(int &start, int &end);
	// get state align res
	Align_res* GetAliState();

	void PrintAliState()
	{
		for(int i = 0 ; i < states_.size(); ++i)
		{
			align_states_[i].Print();
		}
	}

	void Reset()
	{
		dtw_->Reset();
		cur_frame_ = 0;
	}
private:

	bool adjust_;
	int cur_frame_;
	int max_frame_len_;

	vector<int> states_;   // state list
	vector<float> weight_; // state weight
	//DP(dtw) need space and source
	DtwAlign *dtw_;
	vector<Align_State> align_states_;
};


#endif

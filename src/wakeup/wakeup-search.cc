/*
 * Copyright 2017-2018
 *
 * File Type : C++ Source File(*.cpp)
 * File Name : wakeup-search.h
 * Module    :
 * Create on : 2018/1/24
 * Author    : hubo
 *
 * This file is used wake up search realize.
 * */

#include "wakeup-search.h"

Align_res* WakeupSearch::GetAliState()
{
	int dtw_len=0;
	Align_res *dtw_ali = dtw_->startAlign(&dtw_len);
	if(dtw_ali == NULL)
		return NULL;

	float cur_score=0;
	for(int i=0,j=0; i<states_.size(); ++i)
	{
		align_states_[i].state = states_[i];
		if(i==0)
			align_states_[i].start = dtw_ali[0].frame;
		else
			align_states_[i].start = align_states_[i-1].end+1;
		for(; j<dtw_len-1; ++j)//the first and the end frame isn't consider
		{
			if(dtw_ali[j].state != dtw_ali[j+1].state)
				break;
		}
		align_states_[i].end = dtw_ali[j].frame;
		align_states_[i].state_score = dtw_ali[j].total_score - cur_score;
		align_states_[i].state_average_score = 
			align_states_[i].state_score /(align_states_[i].end-align_states_[i].start+1);
		cur_score += align_states_[i].state_score;
		++j;
	}
	return dtw_ali;
}

void WakeupSearch::InputDataOneFrame(float *data, int cols)
{
	for(int j=0;j<states_.size();++j)
	{
		int col = states_[j];
		assert(col < cols);
		float wei = weight_[j];
		dtw_->setData2Dtw(data[col] * wei);
	}
	++cur_frame_;
}

float WakeupSearch::JudgeWakeup(int &start, int &end)
{
	start = -1, end = 0;
	int total_nonsil_frame = 0;
	float total_score = 0;
	int word_len = 0;
	for(int i=0; i < align_states_.size(); ++i)
	{
		if(align_states_[i].state != 0)// nonsilence
		{
			if(start < 0)
				start = align_states_[i].start;
			end = align_states_[i].end;
			word_len = align_states_[i].end - align_states_[i].start + 1;
			total_nonsil_frame += word_len;
			total_score += align_states_[i].state_score;
			if (adjust_==true)
			{
				if(word_len < 3) // word it's to short
					return 0;
			}
		}
	}
	{
		if (adjust_==true)
		{
			return total_score/total_nonsil_frame;
		}
		if(total_score/total_nonsil_frame > 0.1
				&& total_nonsil_frame > 40 )
		{
			return total_score/total_nonsil_frame;
		}
	}
	return 0;
}
float WakeupSearch::ProcessData(float *data, int rows, int cols)
{
	int best_start =0,best_end = 0;
	float best_score = 0.0;
	int all_start, all_end=0;
	float all_score =0.0;
	for(int i=0;i<rows;++i)
	{
		float *data_r = data+i*cols;
		InputDataOneFrame(data_r, cols);
		// judge wakeup
		if(cur_frame_ > 50 && cur_frame_ % 10 == 0)
		{
			float average_posterior = 0;
			Align_res *ali_res = GetAliState();
			int start=0,end=0;
			float score = JudgeWakeup(start,end);
			//printf("cur:%d %d %d %f\n",cur_frame_,start,end,score);
			if (score > best_score)
			{
				best_start = start;
				best_end = end;
				best_score = score;
			}
			all_start = start;
			all_end = end;
			all_score = score;
		}
	}
	printf("best:%d %d %d %f ",cur_frame_,best_start,best_end,best_score);
	printf("all:%d %d %f\n",all_start,all_end,all_score);
	PrintAliState();
	return 0.0;
}

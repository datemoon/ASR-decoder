/*
 * Copyright 2015-2016
 *
 * File Type : C++ Source File(*.cpp)
 * File Name : dtw.cpp
 * Module    :
 * Create on :2015/12/21
 * Author    :hubo
 *
 * This file realization class DtwAlign.
 * */

#include <string.h>
#include "dtw.h"

/*
 * judge alignment result
 * @align_word:word alignment result.
 * @ali       :frame alignment result.
 * @ali_len   :ali length.
 * @states    :state sequence.
 * @num       :state sequence length.
 * */
void judge_align_res(Align_State *align_word,Align_res *ali,int ali_len,int *states,int num)
{
    int i=0,j=0;
    float cur_score=0;
    for(i=0;i<num;++i){
        align_word[i].state = states[i];
        if(i==0)
            align_word[i].start = ali[0].frame;
        else
            align_word[i].start = align_word[i-1].end+1;
        for(;j<ali_len-1;++j){//the first and the end frame isn't consider
            if(ali[j].state != ali[j+1].state){
                break;
            }
        }
        align_word[i].end = ali[j].frame;
        align_word[i].state_score = ali[j].total_score - cur_score;
        align_word[i].state_average_score = 
            align_word[i].state_score/(align_word[i].end-align_word[i].start+1);
        cur_score += align_word[i].state_score;

        ++j;
    }
}

// new space
int DtwAlign::Init()
{
    dtw_ = new float[dtw_rows_*dtw_cols_];
    memset(dtw_,0x00,sizeof(float)*dtw_rows_*dtw_cols_);
    dtw_total_ = new float[dtw_rows_*dtw_cols_];
    memset(dtw_total_,0x00,sizeof(float)*dtw_rows_*dtw_cols_);
    align_ = new Align_res[dtw_rows_+dtw_cols_];
    return 0;
}

void DtwAlign::Reset()
{
    start_location_ = 0;
    end_location_ = 0;
    total_load_rows_ = 0;
    cur_cols_ = 0;
}

int DtwAlign::loadData2Dtw(float *data,int rows,int cols)
{
    int i=0,j=0;
    for(i=0;i<rows;++i){
        for(j=0;j<cols;++j){
            setData2Dtw(data[i*cols+j]);
        }
    }
    return 0;
}
/* 
 * data:
 * */
void DtwAlign::setData2Dtw(float data)
{
    dtw_[(total_load_rows_%dtw_rows_)*dtw_cols_+cur_cols_] = data;
    if(cur_cols_ == (dtw_cols_-1)){
        cur_cols_ = 0;
        ++total_load_rows_;
        end_location_= (total_load_rows_)%dtw_rows_;
        if(total_load_rows_ > dtw_rows_)
            start_location_ = total_load_rows_%dtw_rows_;
    }
    else{
        ++cur_cols_;
        if(total_load_rows_ > dtw_rows_)
            start_location_ = (total_load_rows_+1)%dtw_rows_;
    }
}

Align_res* DtwAlign::startAlign(int *res_len)
{
    memset(dtw_total_,0x00,sizeof(float)*dtw_rows_*dtw_cols_);
    int i=0,j=0;
    int start=0,end=0;
    if(cur_cols_ != 0){
        return NULL;
    }
    else{
        start = start_location_;
        if(start != 0 || total_load_rows_ >= dtw_rows_)
            end = end_location_+dtw_rows_;
        else
            end = end_location_;
    }
    /* dtw_total
     * |* 0 0 0 0 0 0 0 0|
     * |* * 0 0 0 0 0 0 0|
     * |* * * 0 0 0 0 0 0|
     * |* * * * 0 0 0 0 0|
     * |* * * * * 0 0 0 0|
     * |* * * * * * 0 0 0|
     * |* * * * * * * 0 0|
     * |* * * * * * * * 0|
     * |* * * * * * * * *|
     * |* * * * * * * * *|
     * |0 * * * * * * * *|
     * |0 0 * * * * * * *|
     * |0 0 0 * * * * * *|
     * |0 0 0 0 * * * * *|
     * |0 0 0 0 0 * * * *|
     * |0 0 0 0 0 0 * * *|
     * |0 0 0 0 0 0 0 * *|
     * |0 0 0 0 0 0 0 0 *|
     * */
    for(i=start;i<end;++i){
        for(j=0;j<dtw_cols_;++j){
            if(j > (i-start))
                dtw_total_[(i-start)*dtw_cols_+j]=0;
            else if(i>=(end-dtw_cols_) && (j < i-end+dtw_cols_))
                dtw_total_[(i-start)*dtw_cols_+j]=0;
            else if((i-start) == 0){
                dtw_total_[(i-start)*dtw_cols_+j]=dtw_[start*dtw_cols_+j];
            }
            else if(j==0){
                dtw_total_[(i-start)*dtw_cols_+j] = 
                    dtw_total_[(i-start-1)*dtw_cols_+j] +dtw_[(i%dtw_rows_)*dtw_cols_+j];
            }
            else{
                float max =  
                    dtw_total_[(i-start-1)*dtw_cols_+j]>
                    dtw_total_[(i-start-1)*dtw_cols_+j-1] 
                    ? dtw_total_[(i-start-1)*dtw_cols_+j] 
                    : dtw_total_[(i-start-1)*dtw_cols_+j-1];
                dtw_total_[(i-start)*dtw_cols_+j] = 
                    max + dtw_[(i%dtw_rows_)*dtw_cols_+j];
            }
        }
    }//end dtw_total_ block

    //look back upon
    {
        int stop_row = end-start-1;
        *res_len = end-start;
        j = dtw_cols_-1;
        align_[stop_row].frame = total_load_rows_-1;
        //printf("start %d end %d %d %d\n",start,end,stop_row,total_load_rows_);
        align_[stop_row].state = j;
        align_[stop_row].total_score = dtw_total_[stop_row*dtw_cols_+j];
        --stop_row;
        for(i=stop_row;i>=0;--i){
            align_[i].frame = align_[i+1].frame-1;
       /*     if(j == 0){
                align_[i].state = state_[j];
                align_[i].total_score = dtw_total_[i*dtw_cols_+j];
            }
            else if(dtw_total_[i*dtw_cols_+j] > dtw_total_[i*dtw_cols_+j-1]){
                align_[i].state = state_[j];
                align_[i].total_score = dtw_total_[i*dtw_cols_+j];
            }
            else{
                j--;
                align_[i].state = state_[j];
                align_[i].total_score = dtw_total_[i*dtw_cols_+j];
            }*/
			if(j != 0 && dtw_total_[i*dtw_cols_+j] < dtw_total_[i*dtw_cols_+j-1])
				--j;
            align_[i].state = j;
            align_[i].total_score = dtw_total_[i*dtw_cols_+j];
        }
    }//end look upon
    return align_;
}

Align_res* DtwAlign::StartAlign(int *res_len)
{
    memset(dtw_total_,0x00,sizeof(float)*dtw_rows_*dtw_cols_);
    int i=0,j=0;
    int start=0,end=0;
    if(cur_cols_ != 0){
        return NULL;
    }
    else{
        start = start_location_;
        if(start != 0 || total_load_rows_ >= dtw_rows_)
            end = end_location_+dtw_rows_;
        else
            end = end_location_;
    }
    /* dtw_total
     * |* * 0 0 0 0 0 0 0|
     * |* * * 0 0 0 0 0 0|
     * |* * * * 0 0 0 0 0|
     * |* * * * * 0 0 0 0|
     * |* * * * * * 0 0 0|
     * |* * * * * * * 0 0|
     * |* * * * * * * * 0|
     * |* * * * * * * * *|
     * |* * * * * * * * *|
     * |* * * * * * * * *|
     * |* * * * * * * * *|
     * |0 * * * * * * * *|
     * |0 0 * * * * * * *|
     * |0 0 0 * * * * * *|
     * |0 0 0 0 * * * * *|
     * |0 0 0 0 0 * * * *|
     * |0 0 0 0 0 0 * * *|
     * |0 0 0 0 0 0 0 * *|
     * */
	int min = dtw_cols_/5;
	int width = (end-start)/5 ; //restrict dtw width
    if(min > width)
		width = end-start;
	if(width < 0)
		width = dtw_cols_;
	
	for(i=start;i<end;++i){
        for(j=0;j<dtw_cols_;++j){
			if((i-start) == 0 && j==0)
				dtw_total_[(i-start)*dtw_cols_+j]=dtw_[start*dtw_cols_+j];
			else if(j > (i-start) + width)
                dtw_total_[(i-start)*dtw_cols_+j] = DTWINF;
            else if(i>=(end-dtw_cols_)+width && (j < i-end+dtw_cols_ - width))
                dtw_total_[(i-start)*dtw_cols_+j] = DTWINF;
            else if((i-start) == 0){
                dtw_total_[(i-start)*dtw_cols_+j]=
					dtw_total_[(i-start)*dtw_cols_+j-1] + dtw_[start*dtw_cols_+j];
            }
            else if(j==0){
                dtw_total_[(i-start)*dtw_cols_+j] = 
                    dtw_total_[(i-start-1)*dtw_cols_+j] +dtw_[(i%dtw_rows_)*dtw_cols_+j];
            }
            else{
                float min =  
                    dtw_total_[(i-start-1)*dtw_cols_+j] <
                    dtw_total_[(i-start-1)*dtw_cols_+j-1] 
                    ? dtw_total_[(i-start-1)*dtw_cols_+j] 
                    : dtw_total_[(i-start-1)*dtw_cols_+j-1];
				min = min < dtw_total_[(i-start)*dtw_cols_+j-1] 
					? min : dtw_total_[(i-start)*dtw_cols_+j-1];
                dtw_total_[(i-start)*dtw_cols_+j] = 
                    min + dtw_[(i%dtw_rows_)*dtw_cols_+j];
            }
        }
    }//end dtw_total_ block

    //look back upon
    {
        int stop_row = end-start-1;
        *res_len = end-start;
        j = dtw_cols_-1;

		// here is align
		int k = dtw_rows_+dtw_cols_-1;
        align_[k].frame = total_load_rows_-1;
        //printf("start %d end %d %d %d\n",start,end,stop_row,total_load_rows_);
        align_[k].state = j;
        align_[k].total_score = dtw_total_[stop_row*dtw_cols_+j];
        //--stop_row;
        for(i=stop_row;i>=0;){
			k--;
            //align_[i].frame = align_[i+1].frame-1;
			if(i == 0 && j == 0)
			{
				break;
			}
			else if(j == 0 || 
					(dtw_total_[(i-1)*dtw_cols_+j] < dtw_total_[(i-1)*dtw_cols_+j-1] &&
					 dtw_total_[(i-1)*dtw_cols_+j] < dtw_total_[(i)*dtw_cols_+j-1]))
			{
				i--;
				align_[k].frame = align_[k+1].frame-1;
            }
			else if(i == 0 ||
					(dtw_total_[(i)*dtw_cols_+j-1] < dtw_total_[(i-1)*dtw_cols_+j-1] &&
					 dtw_total_[(i)*dtw_cols_+j-1] < dtw_total_[(i-1)*dtw_cols_+j]))
			{
				j--;
				align_[k].frame = align_[k+1].frame;
			}
            else{
				i--;
                j--;
				align_[k].frame = align_[k+1].frame-1;
            }
            align_[k].state = j;
            align_[k].total_score = dtw_total_[i*dtw_cols_+j];
        }
		*res_len = dtw_rows_+dtw_cols_-1 - k;
    }//end look upon
    return align_;
}


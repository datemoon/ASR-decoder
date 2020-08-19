/*
 * Copyright 2015-2016
 *
 * File Type : C++ Header File(*.h)
 * File Name : dtw.h
 * Module    :
 * Create on :2015/12/21
 * Author    :hubo
 *
 * DTW(Dynamic Time Warping) is a DP algorithm. At here it's used alignment.
 * */


#ifndef __DTW_H__
#define __DTW_H__

#include <stdio.h>
#include <iostream>

#define DTWINF (1.0e30)

struct Align_res
{
    int frame;
    int state; // it's col
    float total_score;
    Align_res():frame(0),state(0),total_score(0.0){}
};

class DtwAlign
{
public:
    
    DtwAlign(int rows,int cols): dtw_(NULL),dtw_total_(NULL),align_(NULL),
		dtw_rows_(rows),dtw_cols_(cols),start_location_(0),end_location_(0),total_load_rows_(0),cur_cols_(0){ }
    
    ~DtwAlign(){
        if(dtw_ != NULL){delete[]dtw_;dtw_=NULL;}
        if(dtw_total_ != NULL){delete[]dtw_total_;dtw_total_=NULL;}
        if(align_ != NULL){delete[]align_;align_=NULL;}
        dtw_rows_=0,dtw_cols_=0;
        start_location_=0,end_location_=0;
        total_load_rows_=0,cur_cols_=0;
    }
    /*
     * initialize dtw space
     * state:state sequence
     * cols: state sequence length
     * */
    int Init();
    /*
     * reset dtw
     * */
    void Reset();
    /*
     * load data to dtw space
     * */
    int loadData2Dtw(float *data,int rows,int cols);

    /*
     * set dtw data ,once set one data.
     * */
    void setData2Dtw(float data);

    /*
     * start computer align
     * */
    Align_res* startAlign(int *res_len);

	/*
	 * the other way for align
	 * */
	Align_res* StartAlign(int *res_len);

    int getStateDim(){return dtw_cols_;}

	int GetCols() {return dtw_cols_;}

	int GetRows() {return dtw_rows_;}

	int GetAlignLength() { return (dtw_rows_+dtw_cols_-1);}

private:
    float *dtw_;         //dtw_ be recycled
    float *dtw_total_;
    Align_res *align_;
    int dtw_rows_;       //dtw row ,it's time
    int dtw_cols_;       //dtw col ,it's state
    int start_location_; //dtw_ start location
    int end_location_;   //dtw_ end location
    int total_load_rows_;//record total send in data rows ,increment
    int cur_cols_;       //record current location
};

struct Align_State
{
    int state;
    int start;
    int end;
    float state_score;
    float state_average_score;
    Align_State():start(0),end(0),state_score(0.0),state_average_score(0){}
	void Print()
	{
		std::cout << state << " " << start << " " << end << " " << state_average_score << std::endl;
	}
};

void judge_align_res(Align_State *align_word,Align_res *ali,int ali_len,int *states,int num);
#endif

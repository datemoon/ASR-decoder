#ifndef __NNET_FEATURE_API_H__
#define __NNET_FEATURE_API_H__

#include <stdio.h>
#include "src/nnet/FeatureExtractor.h"
#include "src/pitch/pitch-functions.h"
using namespace tasr;

#include "src/util/namespace-start.h"
class DnnFeat
{
private:
	FeatureExtractor *_feat_ext;
	float *_feats; // features storage memery
	int _dims;     // feature dims
	int _rows;     // feature rows
	int _start;    // features is start or not. 1 is start, 0 is not start.
	int _end;      // features is end or not. 1 is end, 0 is not end.
	int _capacity; // _feats capacity ,init 1 second _feats space
	int _win_shift;

	int _cur_frame;
	int _leftcontext; // is negative or 0.
	int _rightcontext;// is positive or 0.
public:
	DnnFeat():
		_feat_ext(NULL),
		_feats(NULL),_dims(0),_rows(0),_start(1),_end(0),
		_capacity(0),_leftcontext(0),_rightcontext(0) { }

	
	~DnnFeat()
	{
		if(_feat_ext != NULL)
			delete _feat_ext; 
		_feat_ext = NULL;
		if(_feats != NULL)
			delete []_feats;
		_feats = NULL;
		_dims = 0;_rows = 0;_start = 0;_end = 0;
		_capacity = 0;_leftcontext = 0;_rightcontext = 0;
	}
	
	int Init(const char*cfgPath,const char *sysDir);
	
	// malloc storage and relation parmeter.
	void InitParameters( int leftcontext, int rightcontext);
private:
	// get features point for feature extract.
	// if storage isn't enough , realloc.
	float *GetFeatsPoint(int frames);

	// ExtractFeat after,reset _rows , must be call after GetFeatsPoint().
	// beacuse GetFeatsPoint rows not always use up.
	inline void SetRows(int rows);
public:
	// get features point for dnn forward
	// if first copy first frame.
	// if end copy the last frame.
	// nnet_in_frame : can send in dnn score frame number.
	// endflag       : 1 is end,0 is not end.
	int GetFeats(char *data, int len, 
			float **feats, int *nnet_in_frame, int *dim, int endflag = 0);

	// for support kaldi format feature add this function
	int GetFeats(const char *file,float **feats,int *nnet_in_frame, int *dim);

	int GetFeats(FILE *fp,float **feats,int *nnet_in_frame, int *dim);
	// call GetFeats and use feature after,you must call this function
	// for delete have been used features.
	void RemoveFeats(int nnet_in_frame);

	void Reset();
};


struct DnnOut
{
	float *_dnn_outs; // dnn out put storage memery
	int _dims;        // dnn out dims
	int _rows;        // dnn out rows
	int _cur_frame;   // have used dnn out didn't save , _dnn_outs start is _cur_frame
	int _capacity;    //_dnn_outscapacity ,init 1 second _dnn_outs space.
};

class DnnPitchFeat
{
private:
	FeatureExtractor *_feat_ext;
	float *_feats; // features storage memery
	int _dims;     // feature dims
	int _rows;     // feature rows
	int _start;    // features is start or not. 1 is start, 0 is not start.
	int _end;      // features is end or not. 1 is end, 0 is not end.
	int _capacity; // _feats capacity ,init 1 second _feats space
	int _win_shift;

	int _cur_frame;
	int _leftcontext; // is negative or 0.
	int _rightcontext;// is positive or 0.

	// fbanks features
	float *_am_feats;
	int _am_dims;
	int _am_rows;
	int _am_capacity;
	// pitch 
	float *_pitch;
	int _pitch_dims;
	int _pitch_rows;
	int _pitch_capacity;

	PitchExtractionOptions _pitch_opts;
	ProcessPitchOptions _process_opts;

	StreamPitch *_stream_pitch;

public:
	DnnPitchFeat(ConfigParseOptions *conf):
		_feat_ext(NULL),
		_feats(NULL),_dims(0),_rows(0),_start(1),_end(0),
		_capacity(0),_leftcontext(0),_rightcontext(0),
		_am_feats(NULL), _am_dims(0), _am_rows(0), _am_capacity(0),
		_pitch(NULL), _pitch_dims(0), _pitch_rows(0), _pitch_capacity(0)
	{
	   	_pitch_opts.Register(conf);
		_process_opts.Register(conf);
	}

	
	~DnnPitchFeat()
	{
		if(_feat_ext != NULL)
			delete _feat_ext; 
		_feat_ext = NULL;
		if(_feats != NULL)
			delete []_feats;
		_feats = NULL;
		_dims = 0;_rows = 0;_start = 0;_end = 0;
		_capacity = 0;_leftcontext = 0;_rightcontext = 0;
		if(_am_feats != NULL)
			delete []_am_feats;
		_am_feats=NULL;_am_dims = 0;_am_rows=0;
		if(_pitch != NULL)
			delete []_pitch;
		_pitch= NULL;_pitch_dims=0;_pitch_rows=0;
		if(_stream_pitch != NULL)
			delete _stream_pitch;
	}
	
	int Init(const char*cfgPath,const char *sysDir);
	
	// malloc storage and relation parmeter.
	void InitParameters( int leftcontext, int rightcontext);
private:
	// get features point for feature extract.
	// if storage isn't enough , realloc.
	float *GetFeatsPoint(int frames);

	float *GetAmFeatsPoint(int rows);

	float *GetPitchFeatsPoint(int rows);
	// ExtractFeat after,reset _rows , must be call after GetFeatsPoint().
	// beacuse GetFeatsPoint rows not always use up.
	inline void SetRows(int rows);

	int MergeFeat();
public:
	// get features point for dnn forward
	// if first copy first frame.
	// if end copy the last frame.
	// nnet_in_frame : can send in dnn score frame number.
	// endflag       : 1 is end,0 is not end.
	int GetFeats(char *data, int len, 
			float **feats, int *nnet_in_frame, int *dim, int endflag = 0);

	// for support kaldi format feature add this function
	int GetFeats(char *file,float **feats,int *nnet_in_frame, int *dim);

	// call GetFeats and use feature after,you must call this function
	// for delete have been used features.
	void RemoveFeats(int nnet_in_frame);

	void Reset();
};
#include "src/util/namespace-end.h"
#endif

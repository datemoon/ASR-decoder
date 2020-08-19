#ifndef __NNET_FORWARD_H__
#define __NNET_FORWARD_H__

#include <assert.h>

#include "src/util/namespace-start.h"
class Lstm
{
};

class DnnItf
{
public:
	// return frames >= 0.
	int CalDnnScore(float *feat,int frames,int dim);

	float LogLikelihood(int frame, int ilabel);

	int NumFrame() { return _num_frame;}

	void DecodeOneFrame() { _have_process_frames++ ;}

	bool ExamineFrame() 
	{
		assert(_num_frames >=_have_process_frames);
		if(_num_frames == _have_process_frames)
			return false;
		return true;
	}
private:
	float *_feats;
	int _feat_capacity; // default 1 minute.
	int _feat_size;
	int _feat_dim;

	int _num_frames;
	int _have_process_frames;
	int _shiks;

	float *_post;
};


#include "src/util/namespace-end.h"

#endif

#include <assert.h>
#include <string.h>
#include "src/nnet/nnet-feature-api.h"
#include "src/nnet/nnet-util.h"

#include "src/util/namespace-start.h"

int DnnFeat::Init(const char*cfgPath,const char *sysDir)
{
	_feat_ext = new FeatureExtractor;
	return _feat_ext->Init(cfgPath,sysDir);
}

void DnnFeat::InitParameters(int leftcontext, int rightcontext)
{
	Reset();
	_dims = _feat_ext->GetFeatureDim();
	if(_capacity < _dims * (leftcontext + 1 +rightcontext + 10))
	{
		_capacity = _dims * (leftcontext + 1 +rightcontext + 10);
	}
	_leftcontext = leftcontext;
	_rightcontext = rightcontext;
	_rows = leftcontext;
	_start = 1;
	_end = 0;

	_feats = new float[_capacity];

	_win_shift = _feat_ext->GetSrcSampleRate()/_feat_ext->GetTgtFrmRate();
}

float *DnnFeat::GetFeatsPoint(int rows)
{
	float *feats = NULL;
	// need total storage
	int tot_storage = _dims * (rows + _rows + _rightcontext); // if it's end, I need additional _rightcontext frames.
	if(tot_storage > _capacity)
	{
		float *new_feat = new float[tot_storage];
		memcpy(new_feat, _feats, _dims*_rows*sizeof(float));
		delete []_feats;
		_feats = new_feat;
		_capacity = tot_storage;
	}
	feats = _feats + _dims * _rows;
	return feats;
}

void DnnFeat::SetRows(int rows)
{
	_rows += rows;
	_cur_frame += rows;
}

int DnnFeat::GetFeats(FILE *fp,float **feats,int *nnet_in_frame, int *dim)
{
	// anasy file
	long f_offset = ftell(fp);
#define GETLINE (128*64)
	char line[GETLINE]={0};
	int num_line =0;
	while(fgets(line,GETLINE,fp) != NULL)
	{
		if (num_line == 0)
			assert(strstr(line,"["));
		num_line ++;
		if (strstr(line,"]") != NULL)
			break;
	}
	if (num_line == 0)
		return 1;
	fseek(fp, f_offset, SEEK_SET);
	int frames = num_line - 1;
	float *feat = GetFeatsPoint(frames);

	ReadMatrix(fp, feat, frames, _dims);
	SetRows(frames);
	// no features.
	if(frames == 0 && _start == 1)
		return 0;

	// copy the first frame 
	for(int i=0; i < _leftcontext; ++i)
	{
		memcpy(_feats+i*_dims,
				_feats+_leftcontext*_dims,_dims*sizeof(float));
	}
	// GetFeatsPoint have consider memery, so there no realloc process.
	// but I do assert() check.
	assert(_capacity >= (_rows+_rightcontext)*_dims);
	for(int i=0; i < _rightcontext; ++i)
	{
		memcpy(_feats+(_rows+i)*_dims,
				_feats+ (_rows-1)*_dims, _dims*sizeof(float));
	}
	_end = 1;
	_rows += _rightcontext;
	
	*feats = _feats;
	*nnet_in_frame = _rows - _leftcontext - _rightcontext;
	*nnet_in_frame = *nnet_in_frame > 0 ? *nnet_in_frame : 0;
	*dim = _dims;
	fclose(fp);
	return 0;
}

int DnnFeat::GetFeats(const char *file,float **feats,int *nnet_in_frame, int *dim)
{
	// anasy file
	FILE *fp = fopen(file,"r");
	if(NULL == fp)
	{
		return -1;
	}
#define GETLINE (128*64)
	char line[GETLINE]={0};
	int num_line =0;
	while(fgets(line,GETLINE,fp) != NULL)
		num_line ++;
	fseek(fp,0,SEEK_SET);
	int frames = num_line - 1;
	float *feat = GetFeatsPoint(frames);

	ReadMatrix(fp, feat, frames, _dims);
	SetRows(frames);
	// no features.
	if(frames == 0 && _start == 1)
		return 0;

	// copy the first frame 
	for(int i=0; i < _leftcontext; ++i)
	{
		memcpy(_feats+i*_dims,
				_feats+_leftcontext*_dims,_dims*sizeof(float));
	}
	// GetFeatsPoint have consider memery, so there no realloc process.
	// but I do assert() check.
	assert(_capacity >= (_rows+_rightcontext)*_dims);
	for(int i=0; i < _rightcontext; ++i)
	{
		memcpy(_feats+(_rows+i)*_dims,
				_feats+ (_rows-1)*_dims, _dims*sizeof(float));
	}
	_end = 1;
	_rows += _rightcontext;
	
	*feats = _feats;
	*nnet_in_frame = _rows - _leftcontext - _rightcontext;
	*nnet_in_frame = *nnet_in_frame > 0 ? *nnet_in_frame : 0;
	*dim = _dims;
	fclose(fp);
	return 0;
}

int DnnFeat::GetFeats(char *data, int len,
		float **feats, int *nnet_in_frame, int *dim, int endflag)
{
	*feats = NULL;
	*nnet_in_frame = 0;
	*dim = _dims;
	// according to send data length , calculate feature frames.
	// only 16 bit.
	int bit = sizeof(short);
	int frames = (len/bit/_win_shift + 3);
	// malloc features space.
	float *feat = GetFeatsPoint(frames);
	int feat_len = frames * _dims * sizeof(float);

	// set features space.
	if(0 != _feat_ext->SetBuff(feat,feat_len))
		return -1;
	int frame_num = 0;
	// feature extract.
	int ret = 0;
	if(endflag == 0)
	{
		ret = _feat_ext->ExtractFeat(data,len,frame_num);
	}
	else
	{
		ret = _feat_ext->ExtractFeat_Last(data,len,frame_num);
	}
	if(ret != 0)
	{
		// feature extract error.
		return -2;
	}
	// set right frame number.
	SetRows(frame_num);
	// no features.
	if(frame_num == 0 && _start == 1)
		return 0;
	
	assert(!(endflag == 1 && _end == 1));
	if(_start == 1)
	{
		// copy the first frame 
		for(int i=0; i < _leftcontext; ++i)
		{
			memcpy(_feats+i*_dims,
					_feats+_leftcontext*_dims,_dims*sizeof(float));
		}
		_start = 0;
	}
	if(endflag == 1)
	{
		// GetFeatsPoint have consider memery, so there no realloc process.
		// but I do assert() check.
		assert(_capacity >= (_rows+_rightcontext)*_dims);
		for(int i=0; i < _rightcontext; ++i)
		{
			memcpy(_feats+(_rows+i)*_dims,
					_feats+ (_rows-1)*_dims, _dims*sizeof(float));
		}
		_end = 1;
		_rows += _rightcontext;
	}
	// there can be set for add speed , beacuse matrix is faster then vector,
	// so _rows less then some value , don't send it in nnet,
	// but now I don't do this. can modefy 1 to n. 
	if(_rows < (_leftcontext + 1 + _rightcontext))
	{
		// no feature
		*nnet_in_frame = 0;
		return 0;
	}

	*feats = _feats;
	*nnet_in_frame = _rows - _leftcontext - _rightcontext;
	*nnet_in_frame = *nnet_in_frame > 0 ? *nnet_in_frame : 0;
	*dim = _dims;
	return 0;
}

void DnnFeat::RemoveFeats(int nnet_in_frame)
{
	if(nnet_in_frame > 0)
	{
		memmove(_feats,
				_feats+nnet_in_frame*_dims,
				(_rows - nnet_in_frame) * _dims * sizeof(float));
		_rows -= nnet_in_frame;
	}
}

void DnnFeat::Reset()
{
	_feat_ext->Reset();
	_rows = _leftcontext;
	_start = 1;
	_end = 0;
	_cur_frame = 0;
	memset(_feats,0x00,sizeof(float) * _capacity);
}

#include "src/util/namespace-end.h"


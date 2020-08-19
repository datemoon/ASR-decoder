#include <assert.h>
#include <string.h>
#include "src/nnet/nnet-feature-api.h"
#include "src/nnet/nnet-util.h"

int DnnPitchFeat::Init(const char*cfgPath,const char *sysDir)
{
	_stream_pitch = new StreamPitch(_pitch_opts, _process_opts);
	_feat_ext = new FeatureExtractor;
	return _feat_ext->Init(cfgPath,sysDir);
}

void DnnPitchFeat::InitParameters(int leftcontext, int rightcontext)
{
	Reset();
	_am_dims = _feat_ext->GetFeatureDim();
	_pitch_dims = _stream_pitch->GetDim();
	_dims = _am_dims + _pitch_dims;
	_am_rows = 0;
	_pitch_rows = 0;
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

float *DnnPitchFeat::GetFeatsPoint(int rows)
{
	float *feats = NULL;
	// need total storage
	int tot_storage = _dims * (rows + _rows + _rightcontext); // additional _rightcontext frames.
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

float * DnnPitchFeat::GetAmFeatsPoint(int rows)
{
	int tot_storage = _am_dims * (rows + _am_rows);
	if(tot_storage > _am_capacity)
	{
		float *new_feat = new float[tot_storage];
		memcpy(new_feat, _am_feats, _am_dims * _am_rows * sizeof(float));
		delete []_am_feats;
		_am_feats = new_feat;
		_am_capacity = tot_storage;
	}
	return _am_feats + _am_dims * _am_rows;
}

float *DnnPitchFeat::GetPitchFeatsPoint(int rows)
{
	int tot_storage = _pitch_dims * (rows + _pitch_rows);
	if(tot_storage > _pitch_capacity)
	{
		float *new_feat = new float[tot_storage];
		memcpy(new_feat, _pitch, _pitch_dims * _pitch_rows * sizeof(float));
		delete []_pitch;
		_pitch = new_feat;
		_pitch_capacity = tot_storage;
	}
	return _pitch + _pitch_dims * _pitch_rows;
}

void DnnPitchFeat::SetRows(int rows)
{
	_rows += rows;
	_cur_frame += rows;
}

int DnnPitchFeat::GetFeats(char *file,float **feats,int *nnet_in_frame, int *dim)
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

int DnnPitchFeat::MergeFeat()
{
	int true_frame = _am_rows < _pitch_rows ? _am_rows : _pitch_rows;
	float *feat_merge = GetFeatsPoint(true_frame);
	for(int i = 0 ; i < true_frame; ++i)
	{
		memcpy(feat_merge+i*_dims, _am_feats+i*_am_dims, sizeof(float)*_am_dims);
		memcpy(feat_merge+i*_dims+_am_dims, _pitch+i*_pitch_dims, sizeof(float)*_pitch_dims);
	}
#ifdef DEBUG
	for(int r = 0 ; r < true_frame ; ++r)
	{
		for(int c = 0; c < _dims ; ++c)
			std::cout << feat_merge[r*_dims+c] << " ";
		std::cout << std::endl;
	}
#endif
	// move feat
	memmove(_am_feats,_am_feats+true_frame*_am_dims,sizeof(float)*(_am_rows-true_frame)*_am_dims);
	_am_rows -= true_frame;
	memmove(_pitch, _pitch + true_frame * _pitch_dims, sizeof(float) * (_pitch_rows - true_frame) * _pitch_dims);
	_pitch_rows -= true_frame;
	return true_frame;
}

int DnnPitchFeat::GetFeats(char *data, int len,
		float **feats, int *nnet_in_frame, int *dim, int endflag)
{
	assert(_am_rows == 0 || _pitch_rows == 0);
	*feats = NULL;
	*nnet_in_frame = 0;
	*dim = _dims;
	// according to send data length , calculate feature frames.
	// only 16 bit.
	int bit = sizeof(short);
	// calcate pitch
	bool is_end_send = false;
	if(endflag == 1)
		is_end_send = true;

	int frames = (len/bit/_win_shift + 3);
	// malloc features space.
	float *feat = GetAmFeatsPoint(frames);
	int feat_len = frames * _am_dims * sizeof(float);

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
	// calcate feat rows
	_am_rows += frame_num;

	float *pitch_feat = NULL;
	int pitch_dim = 0;
	int pitch_rows = 0;
	_stream_pitch->ProcessWave(_pitch_opts, (short*)data, len/sizeof(short),
			&pitch_feat, pitch_dim, pitch_rows, is_end_send);
	float *feat_pitch = GetPitchFeatsPoint(pitch_rows);
	memcpy(feat_pitch,pitch_feat,sizeof(float) * pitch_dim * pitch_rows);
	_pitch_rows += pitch_rows;
	if(pitch_feat != NULL)
		delete []pitch_feat;
	// print pitch

#ifdef DEBUGPITCH
	for(int r =0 ; r < pitch_rows; ++r)
	{
		for(int c = 0 ; c<pitch_dim;++c)
		{
			std::cout << pitch_feat[r*pitch_dim+c] << " ";
		}
		std::cout << std::endl;
	}
#endif

	// merge feat and pitch
	int true_frame = MergeFeat();
	// set right frame number.
	SetRows(true_frame);
	// no features.
	if(true_frame == 0 && _start == 1)
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

void DnnPitchFeat::RemoveFeats(int nnet_in_frame)
{
	if(nnet_in_frame > 0)
	{
		memmove(_feats,
				_feats+nnet_in_frame*_dims,
				(_rows - nnet_in_frame) * _dims * sizeof(float));
		_rows -= nnet_in_frame;
	}
}

void DnnPitchFeat::Reset()
{
	_feat_ext->Reset();
	if(_stream_pitch != NULL)
		delete _stream_pitch;
	_stream_pitch = new StreamPitch(_pitch_opts, _process_opts);
	_rows = _leftcontext;
	_start = 1;
	_end = 0;
	_cur_frame = 0;
	_am_rows = 0;
	_pitch_rows = 0;
	memset(_feats,0x00,sizeof(float) * _capacity);
	memset(_am_feats, 0x00, sizeof(float) * _am_capacity);
	memset(_pitch, 0x00, sizeof(float) * _pitch_capacity);
}



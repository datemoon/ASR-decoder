#include <stdio.h>
#include <string.h>
#include "src/nnet/nnet-nnet.h"
#include "src/nnet/lstm-layer.h"
#include "src/nnet/nnet-layer.h"
#include "src/nnet/matrix.h"
#include "src/nnet/cudnn-lstm-layer.h"
#include "src/nnet/nnet-util.h"
#include "src/nnet/lstm-projected-layer.h"
#include "src/nnet/nnet-simple-recurrent.h"
#include "src/nnet/tf-lstm-layer.h"

#include "src/util/namespace-start.h"

bool Nnet::ReadNnet(const char* file)
{
	FILE *fp = fopen(file,"rb");
	if(fp ==NULL)
		return false;
	Component* layer = NULL;

	unsigned nlayer = 0;
	if(1 != fread(&nlayer,sizeof(int),1,fp))
		return false;
	while(layer = Component::Read(fp), layer != NULL)
	{
		unsigned size = _nnet.size();
		_nnet.resize(size+1);
		_nnet[size] = layer;
	}
	if(nlayer != _nnet.size())
		return false;
	fclose(fp);
	return true;
}

bool Nnet::ReadNnet(const char *file, bool binary)
{
	FILE *fp = fopen(file,"rb");
	if(fp ==NULL)
		return false;
	Component* layer = NULL;

	//unsigned nlayer = 0;
	while(layer = Component::Read(fp,binary), layer != NULL)
	{
		unsigned size = _nnet.size();
		_nnet.resize(size+1);
		_nnet[size] = layer;
	}
	fclose(fp);
	return true;
}

bool Nnet::WriteNnet(const char *file)
{
	FILE *fp = fopen(file,"w");
	if(fp ==NULL)
		return false;

	unsigned nlayer = _nnet.size();
	if(1 != fwrite(&nlayer,sizeof(unsigned),1,fp))
		return false;
	for(unsigned i=0;i<nlayer;++i)
	{
		Component * comp = _nnet[i];
		comp->Write(fp);
	}
	fclose(fp);
	return true;

}

void NnetForward::GetLRoffset(int &left,int &right)
{
	left = 0,right = 0;
	for(int i=0;i<NumComponents();++i)
	{
		Component * comp = (*_nnet)[i];
		if(kSplice == comp->GetType())
		{

			left = comp->GetLeft();
			right = comp->GetRight();
			return ;
		}
	}
}
void NnetForward::FeedForward(float *in,int frames,int dim)
{
	if(frames <= 0)
		return ;
	_cur_frameid += _frames;
	_frames = frames;
	// because skip frames , so calculate send frame
	_true_cur_frameid += _true_frames;
	_true_frames = (_cur_frameid + _frames + _skip_frames)/(1 + _skip_frames)
	   	- _true_cur_frameid;
	if(_true_frames <= 0)
		return;
	// for speed , all frames together process.
	// ensure memery is enough.
	if(_capacity < _max_dim * _true_frames)
	{
		if(_input_data != NULL)
			delete []_input_data;
		if(_output_data != NULL)
			delete []_output_data;
		_capacity = _max_dim * (_true_frames +5);
		_input_data = new float[_capacity];
		_output_data = new float[_capacity];
	}
	// because skip frames , so input can not be "in" start.
	int offset_frame = _true_cur_frameid * (1 + _skip_frames) - _cur_frameid;
	int offset = offset_frame * dim;
	float *input = in + offset;

	int outdim = 0;
	int indim = dim;
	// default the last layer is Prior
	//for(int i=0;i<NumComponents()-1;++i)
	// because net can have no prior layer, so traverse net and judge prior,if true ,skip
	for(int i=0;i<NumComponents();++i)
	{
		memset(_output_data,0x00,sizeof(float)*_capacity);
		Component * comp = (*_nnet)[i];
		if(comp->GetType() == kPrior)
			break;
		if (false == _do_softmax)
		{
			if(comp->GetType() == kSoftmax)
				break;
		}
		comp->Propagate(input,_true_frames,indim,_output_data,outdim);
		indim = outdim;
		SwapInputOutput();
		input = _input_data;
		// print every layer information
		//PrintPoint(_output_data, _true_frames , outdim);
	}
	if(_do_softmax == true && _do_log == true)
	{
		for(int i = 0; i < _true_frames;++i)
		{
			if(_block_pdf_pdfid < 0)
				break;
			input[i*outdim+_block_pdf_pdfid] *= _bk_scale;
			if(input[i*outdim+_block_pdf_pdfid]/(_bk_scale+1e-8) > _skip_bk)
			{
				input[i*outdim+_block_pdf_pdfid] = 2.71828e30;
			}
		}
		DoLog(input, _true_frames, outdim, _output_data);

		// the last layer must be prior
		if(_sub_prior == true)
		{
			SwapInputOutput();
			input = _input_data;
			memset(_output_data,0x00,sizeof(float)*_capacity);
			Component * comp = (*_nnet)[NumComponents()-1];
			assert(comp->GetType() == kPrior && "the last layer isn't prior layer, shouldn't sub-prior");
			comp->Propagate(input,_true_frames,outdim,_output_data,outdim);
		}
		SwapInputOutput();
	}
	SwapInputOutput();
	_outdim = outdim;
}

void NnetForward::ResetRnnBuffer()
{
	for(int i=0;i<NumComponents();++i)
	{
		Component * comp = (*_nnet)[i];
		ComponentType type = comp->GetType();
		if(type == kLstm)
		{
			Lstm * lstm = dynamic_cast<Lstm *>(comp);
			lstm->ResetBuf();
		}
		else if(type == kLstmCudnn)
		{
			LstmCudnn *lstmcudnn = dynamic_cast<LstmCudnn *>(comp);
			lstmcudnn->ResetBuf();
		}
		else if(type == kLstmProjectedStreams)
		{
			LstmProjectedStreams *lstmproject = dynamic_cast<LstmProjectedStreams*>(comp);
			lstmproject->ResetBuf();
		}
		else if(type == kSRU)
		{
			SRUcell *sru = dynamic_cast<SRUcell*>(comp);
			sru->ResetBuf();
		}
		else if(type == kTfLstm)
		{
			TfLstm *tflstm = dynamic_cast<TfLstm*>(comp);
			tflstm->ResetBuf();
		}
	}
}

void NnetForward::SetSkipFrames(int skip)
{
	for(int i=0;i<NumComponents();++i)
	{
		Component * comp = (*_nnet)[i];
		ComponentType type = comp->GetType();
		if(type == kSplice)
		{
			Splice * splice = dynamic_cast<Splice *>(comp);
			splice->SetSkip(skip);
		}
	}

}

int NnetForward::MaxPdf(int frame,float *max_pdf)
{
	float maxpdf = _output_data[(frame-_true_cur_frameid)*_outdim+0];
	int maxpdfid = 0;
	for(int i=1;i<_outdim;++i)
	{
		if(maxpdf < _output_data[(frame-_true_cur_frameid)*_outdim+i])
		{
			maxpdf = _output_data[(frame-_true_cur_frameid)*_outdim+i];
			maxpdfid = i;
		}
	}
	if(max_pdf != NULL)
		*max_pdf = maxpdf;
	return maxpdfid;
}
#include "src/util/namespace-end.h"

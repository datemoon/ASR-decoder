#ifndef __NNET_NNET_H__
#define __NNET_NNET_H__

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <assert.h>
#include <string>
#include <vector>
#include "src/nnet/nnet-component.h"
#include "src/hmm/transition-model.h"
#include "src/util/config-parse-options.h"
#include "src/nnet/decodable-itf.h"
using namespace std;
#include "src/util/namespace-start.h"

class Nnet
{
public:
	Nnet():_have_trans_model(false) { }
	~Nnet()
	{
		for(unsigned i=0; i<_nnet.size() ; ++i)
		{
				delete _nnet[i];
		}
	}
	bool ReadNnet(const char * file);
	
	bool ReadNnet(const char *file, bool binary);

	bool WriteNnet(const char *file);

	bool ReadTransModel(const char *file)
	{
		std::ifstream ifs;
		bool binary = false;
		ifs.open(file,
				binary ? std::ios_base::in | std::ios_base::binary
				: std::ios_base::in);
		if(ifs.is_open())
		{
			_have_trans_model = true;
			_trans_model.Read(ifs, binary);
			return true;
		}
		else
		{
			std::cerr << "trainsmodel file open fail. " << file << std::endl;
			return false;
		}
	}
	vector<Component *> *GetNet() { return &_nnet;}

	TransitionModel * GetTrans() { return _have_trans_model ? &_trans_model: NULL;;}

private:
	bool _have_trans_model;
	TransitionModel _trans_model;
	vector<Component *> _nnet;
};

struct NnetForwardOptions
{
	int _skip;
	bool _do_log;
	bool _sub_prior;
	bool _do_softmax;
	float _block_scale;
	float _skip_block;
	float _acoustic_scale;
	int _block_pdf_pdfid;

	NnetForwardOptions():
		_skip(0), _do_log(true), _sub_prior(true), _do_softmax(true),  _block_scale(1.0), _skip_block(1.0), _acoustic_scale(1.0), _block_pdf_pdfid(-1) { }

	void Register(ConfigParseOptions *opts)
	{
		opts->Register("skip", &_skip, "Skip frame");// (int, default = 0)");
		opts->Register("do-log", &_do_log , "Transform NN output by log()");//(bool, default = true)");
		opts->Register("sub-prior", &_sub_prior , "Whether subtract prior");// (bool, default = true)");
		opts->Register("do-softmax", &_do_softmax , "Transform NN output by softmax");//(bool, default = true)");
		opts->Register("block-scale", &_block_scale, "In ctc have block, you can scale");// (float, default = 1.0)");
		opts->Register("skip-block", &_skip_block, "Whether process some large probability block. If probability greather then skip-block, so skip it ");//(float, default = 1.0)");
		opts->Register("acoustic-scale", &_acoustic_scale, "Scaling factor for acoustic likelihoods");// (float, default = 1.0)");
		opts->Register("block-pdf-pdfid", &_block_pdf_pdfid, "If use ctc model block, here it's NumTransitionIds+1 ");
	}
};
class NnetForward : public DecodableInterface
{
public:
	NnetForward(Nnet *nnet, const NnetForwardOptions * config):
		_trans_model(nnet->GetTrans()), _block_pdf_pdfid(config->_block_pdf_pdfid), _block_pdf_index(-1),
		_nnet(nnet->GetNet()),
		_skip_frames(config->_skip), _do_log(config->_do_log), 
		_sub_prior(config->_sub_prior), _do_softmax(config->_do_softmax),
		_bk_scale(config->_block_scale),
		_skip_bk(config->_skip_block), _acoustic_scale(config->_acoustic_scale)
	{
		// first get _block_pdf_id for skip block
		if(NULL != _trans_model)
		{
			if(_block_pdf_pdfid >= 0)
				_block_pdf_index = _trans_model->NumTransitionIds() + 1;
		}
		_max_dim = 0;
		for(int i = 0 ; i < NumComponents();++i)
		{
			Component *comp = (*_nnet)[i];
			if(_max_dim < comp->InputDim())
				_max_dim = comp->InputDim();
			if(_max_dim < comp->OutputDim())
				_max_dim = comp->OutputDim();
		}
		SetSkipFrames(_skip_frames);
		_input_data = NULL;
		_output_data = NULL;
		_capacity = 0;
		_frames = 0;
		_cur_frameid = 0;
		_outdim = 0;
		_true_cur_frameid = 0;
		_true_frames = 0;
	}
	NnetForward(Nnet *nnet, int skip=0, bool do_log = true, 
			bool sub_prior = true,bool do_softmax = true,  float bk_scale=1.0,
		   	float skip_bk = 1.0, float acoustic_scale=1.0, int block_pdf_pdfid=-1):
		_trans_model(nnet->GetTrans()), _block_pdf_pdfid(block_pdf_pdfid), _block_pdf_index(-1),
		_nnet(nnet->GetNet()), _skip_frames(skip),
		_do_log(do_log), _sub_prior(sub_prior), _do_softmax(do_softmax),
		_bk_scale(bk_scale), _skip_bk(skip_bk),
		_acoustic_scale(acoustic_scale)
	{
		// first get _block_pdf_id for skip block
		if(NULL != _trans_model)
		{
			if(_block_pdf_pdfid >= 0)
				_block_pdf_index = _trans_model->NumTransitionIds() + 1;
		}
		_max_dim = 0;
		for(int i = 0 ; i < NumComponents();++i)
		{
			Component *comp = (*_nnet)[i];
			if(_max_dim < comp->InputDim())
				_max_dim = comp->InputDim();
			if(_max_dim < comp->OutputDim())
				_max_dim = comp->OutputDim();
		}
		SetSkipFrames(_skip_frames);
		_input_data = NULL;
		_output_data = NULL;
		_capacity = 0;
		_frames = 0;
		_cur_frameid = 0;
		_outdim = 0;
		_true_cur_frameid = 0;
		_true_frames = 0;
   	}

	~NnetForward()
	{
		if(_input_data != NULL)
			delete []_input_data;
		if(_output_data != NULL)
			delete []_output_data;
		_nnet = NULL;
		_input_data = NULL;
		_output_data = NULL;
		_capacity = 0;_frames = 0;_cur_frameid = 0;_max_dim=0 ;_outdim =0;
		_true_cur_frameid = 0;_true_frames = 0;
	}
	int NumComponents() { return _nnet->size();}
	void GetLRoffset(int &left,int &right);
	void FeedForward(float *in,int frames,int dim);

	// because have rnn model ,so every sentence I must be reset rnn buffer.
	// add function ResetRnnBuffer, every start must be call.
	void ResetRnnBuffer();
	
	int GetOutdim() const { return _outdim;}
	void Reset()
	{
		_frames = 0;
		_cur_frameid = 0;
		_true_cur_frameid = 0;
		_true_frames = 0;
		ResetRnnBuffer();
	}

	bool ExamineFrame(int cur_frame)
	{
		if(cur_frame >= (_true_cur_frameid + _true_frames))
			return false;
		return true;
	}

	int NumFramesReady() const
	{
		return _true_frames+_true_cur_frameid;
	}

	bool IsLastFrame(int frame=0) const
	{
		return true;
	}
	
	int NumIndices() const
	{
		return GetOutdim()+1;
	}

	float LogLikelihood(int frame, int ilabel)
	{
		if(NULL != _trans_model)
		{
			if(ilabel == _block_pdf_index)
				ilabel = _block_pdf_pdfid;
			else
				ilabel = _trans_model->TransitionIdToPdf(ilabel);
		}
		else
		{
			if(ilabel == _block_pdf_index)
				ilabel = _block_pdf_pdfid;
			else
				ilabel -= 1;
		}
		assert((_true_cur_frameid + _true_frames) > frame);
		if(ilabel >= _outdim)
			printf("ilabel %d _outdim %d\n",ilabel,_outdim);
		assert( ilabel < _outdim);
		return _acoustic_scale * _output_data[(frame-_true_cur_frameid)*_outdim+ilabel];
	}

	inline float DnnOut(int frame, int dim)
	{
		return _output_data[(frame-_true_cur_frameid)*_outdim+dim];
	}

	int MaxPdf(int frame,float *max_pdf=NULL);

	inline int GetPhoneId(int ilabel)
	{
		if(NULL != _trans_model)
		{
			if(ilabel == _block_pdf_index)
				return -1 * ilabel;
			else
				return _trans_model->TransitionIdToPhone(ilabel);
		}
		else
		{
			return ilabel;
		}

	}

	void SwapInputOutput() 
	{
		float *tmp = _input_data;
		_input_data = _output_data;
		_output_data = tmp;
   	}

	bool SkipBlockFrame(int frame)
	{
		if(NULL != _trans_model)
		{
			return (LogLikelihood(frame, _block_pdf_index) > 20);
		}
		else
		{
			return (LogLikelihood(frame, 1) > 20);
		}
	}

	inline int GetBlockTransitionId()
	{
		return _block_pdf_index;
	}
	inline int GetSkipFrame() { return _skip_frames;}
private:
	void SetSkipFrames(int skip);
private:
	TransitionModel *_trans_model;
	int _block_pdf_pdfid;
	int _block_pdf_index;
	vector<Component *> *_nnet;
	float *_input_data;
	float *_output_data;
	int _capacity;    // _input_data and _output_data capacity
	int _frames;      // how many frames can be process.
	int _cur_frameid; // current frame index
	int _max_dim; // for _input_data and _output_data malloc storage.
	int _outdim;  // nnet out put dim.
	// for skip frame.
	int _skip_frames;
	int _true_cur_frameid;
	int _true_frames;
	// for nnet posterior
	bool _do_log;
	bool _sub_prior;
	bool _do_softmax;

	float _bk_scale;
	float _skip_bk;
	float _acoustic_scale;
};
typedef NnetForward AmInterface;
template <typename Real>
void PrintPoint(Real mat,int rows,int cols);
#include "src/util/namespace-end.h"
#endif

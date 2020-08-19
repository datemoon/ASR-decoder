#ifndef __NNET_LAYER_H__
#define __NNET_LAYER_H__

#include <string>
#include <stdio.h>
#include "src/nnet/matrix.h"
#include "src/nnet/nnet-component.h"
#include "src/nnet/nnet-util.h"

#include "src/util/namespace-start.h"

class Splice :public Component
{
public:
	Splice(int input_dim,int output_dim):
		Component( input_dim, output_dim),_skip_frames(0),
		_dim(0),_offsets(NULL) { }
	
	~Splice() { delete[] _offsets;_dim=0;}
	
	bool ReadData(FILE *fp)
	{
		_dim = OutputDim() / InputDim();
		_offsets = new int[_dim];
		if(_dim != (int)fread(_offsets,sizeof(int),_dim,fp))
			return false;
		return true;
	}
	
	bool ReadData(FILE *fp, bool binary)
	{
		_dim = OutputDim() / InputDim();
		_offsets = new int[_dim];
		ReadMatrix(fp, _offsets, 1, _dim);
		return true;
	}
	bool WriteData(FILE *fp)
	{
		if((unsigned)_dim != fwrite(_offsets,sizeof(int),_dim,fp))
			return false;
		return true;
	}
	
	void PropagateFnc(float *in, int frames, int cols, float *out);

	int GetLeft() { return (-1*_offsets[0]);}
	int GetRight() { return _offsets[_dim-1];}
	void SetSkip(int n) { _skip_frames = n; }

	ComponentType GetType() const { return kSplice; }
	void GetType(string &type) const { type = "Splice" ;}
private:
	int _skip_frames;
	int _dim;
	int *_offsets;
};

class AddShift :public Component
{
public:
	AddShift(int input_dim,int output_dim):
		Component( input_dim, output_dim),_dim(0),_shift_data(NULL) { }
	~AddShift()
	{
		delete []_shift_data; _dim = 0;
	}
	bool ReadData(FILE *fp)
	{
		_dim = InputDim();
		_shift_data = new float[_dim];
		if((unsigned)_dim != fread(_shift_data,sizeof(float),_dim,fp))
			return false;
		return true;
	}
	bool WriteData(FILE *fp)
	{
		if((unsigned)_dim != fwrite(_shift_data,sizeof(float),_dim,fp))
			return false;
		return true;
	}
	bool ReadData(FILE *fp, bool binary)
	{
		_dim = InputDim();
		_shift_data = new float[_dim];
		ReadMatrix(fp, _shift_data, 1, _dim);
		return true;
	}
	ComponentType GetType() const { return kAddShift; }
	void GetType(string &type) const { type = "AddShift" ;}
	void PropagateFnc(float *in, int frames, int cols, float *out);
private:
	int _dim;
	float *_shift_data;
};

class Prior:public Component
{
public:
	Prior(int input_dim,int output_dim):
		Component( input_dim, output_dim),_dim(0),_log_priors(NULL) { }
	~Prior()
	{
		delete []_log_priors; _dim = 0;
	}
	bool ReadData(FILE *fp)
	{
		_dim = InputDim();
		_log_priors = new float[_dim];
		if((unsigned)_dim != fread(_log_priors,sizeof(float),_dim,fp))
			return false;
		return true;
	}
	bool WriteData(FILE *fp)
	{
		if((unsigned)_dim != fwrite(_log_priors,sizeof(float),_dim,fp))
			return false;
		return true;
	}
	bool ReadData(FILE *fp, bool binary)
	{
		_dim = InputDim();
		_log_priors = new float[_dim];
		ReadMatrix(fp, _log_priors, 1, _dim);
		// calculate
		int total_counts = 0;
		for(int i=0;i<_dim;++i)
			total_counts += _log_priors[i];
		for(int i=0;i<_dim;++i)
			_log_priors[i] = _log_priors[i]/total_counts + 1e-20;
		DoLog(_log_priors,1,_dim,_log_priors);
		return true;
	}
	ComponentType GetType() const { return kPrior; }
	void GetType(string &type) const { type = "Prior" ;}
	void PropagateFnc(float *in, int frames, int cols, float *out);
private:
	int _dim;
	float *_log_priors;
};

class Rescale:public Component
{
public:
	Rescale(int input_dim,int output_dim):
		Component( input_dim, output_dim),_dim(0),_scale_data(NULL) { }
	~Rescale()
	{
		delete []_scale_data; _dim = 0;
	}
	bool ReadData(FILE *fp)
	{
		_dim = InputDim();
		_scale_data = new float[_dim];
		if((unsigned)_dim != fread(_scale_data,sizeof(float),_dim,fp))
			return false;
		return true;
	}
	bool WriteData(FILE *fp)
	{
		if((unsigned)_dim != fwrite(_scale_data,sizeof(float),_dim,fp))
			return false;
		return true;
	}
	bool ReadData(FILE *fp, bool binary)
	{
		 _dim = InputDim();
		 _scale_data = new float[_dim];
		 ReadMatrix(fp, _scale_data , 1, _dim);
		 return true;
	}
	ComponentType GetType() const { return kRescale; }
	void GetType(string &type) const { type = "Rescale" ;}
	void PropagateFnc(float *in, int frames, int cols, float *out);
private:
	int _dim;
	float *_scale_data;
};

class Sigmoid:public Component
{
public:
	Sigmoid(int input_dim,int output_dim):
		Component( input_dim, output_dim){ }

	~Sigmoid() { }
	ComponentType GetType() const { return kSigmoid; }
	void GetType(string &type) const { type = "Sigmoid" ;}
	void PropagateFnc(float *in, int frames, int cols, float *out);
private:
};

class Softmax:public Component
{
public:
	Softmax(int input_dim,int output_dim):
		Component( input_dim, output_dim){ }
	~Softmax() { }
	ComponentType GetType() const { return kSoftmax; }
	void GetType(string &type) const { type = "Softmax" ;}
	void PropagateFnc(float *in, int frames, int cols, float *out );
private:
};


class AffineTransform:public Component
{
public:
	AffineTransform(int input_dim,int output_dim):
		Component( input_dim, output_dim),
		_linearity(NULL),_bias(NULL),_rows(0),_cols(0){ }
	~AffineTransform() 
	{
	   	delete []_linearity; _linearity = NULL;
		delete [] _bias; _bias = NULL;
		_rows = 0, _cols = 0;
	}
	bool ReadData(FILE *fp)
	{
		_rows = OutputDim();
		_cols = InputDim();
		_linearity = new float[_rows*_cols];
		_bias = new float[_rows];
		if((unsigned)_rows != fread(_bias,sizeof(float),_rows,fp))
			return false;
		if((unsigned)(_rows*_cols) != fread(_linearity,sizeof(float),_rows*_cols,fp))
			return false;
		return true;
	}
	bool WriteData(FILE *fp)
	{
		if((unsigned)_rows != fwrite(_bias,sizeof(float),_rows,fp))
			return false;
		if((unsigned)(_rows*_cols) != fwrite(_linearity,sizeof(float),_rows*_cols,fp))
			return false;
		return true;
	}
	bool ReadData(FILE *fp, bool binary)
	{
		_rows = OutputDim();
		_cols = InputDim();
		_linearity = new float[_rows*_cols];
		_bias = new float[_rows];

		long fsize = ftell(fp);
		const unsigned len = 1024;
		char line[len];
		ReadLine(fp,line,len);
		char *token = NULL;
		char *ptr = NULL;
		token = strtok_r(line," ",&ptr);
		if(strncmp(token, "[", 1) == 0)
			fseek(fp,fsize,SEEK_SET);

		ReadMatrix(fp, _linearity, _rows , _cols);
		ReadMatrix(fp, _bias, 1, _rows);
		return true;
	}
	ComponentType GetType() const { return kAffineTransform; }
	void GetType(string &type) const { type = "AffineTransform" ;}
	void PropagateFnc(float *in, int frames, int cols, float *out );
private:
	float *_linearity;
	float *_bias;
	int _rows;
	int _cols;
};

class LinearTransform:public Component
{
public:
	LinearTransform(int input_dim,int output_dim):
		Component( input_dim, output_dim),
		_linearity(NULL), _rows(0), _cols(0) { }

	~LinearTransform() 
	{
		if(_linearity != NULL)
			delete [] _linearity;
		_linearity = NULL;
		_rows = 0, _cols = 0;
	}
	bool ReadData(FILE *fp)
	{
		_rows = OutputDim();
		_cols = InputDim();
		_linearity = new float[_rows*_cols];
		if((unsigned)(_rows*_cols) != fread(_linearity,sizeof(float),_rows*_cols,fp))
			return false;
		return true;
	}
	bool WriteData(FILE *fp)
	{
		if((unsigned)(_rows*_cols) != fwrite(_linearity,sizeof(float),_rows*_cols,fp))
			return false;
		return true;
	}
	bool ReadData(FILE *fp, bool binary)
	{
		_rows = OutputDim();
		_cols = InputDim();
		_linearity = new float[_rows*_cols];
		ReadMatrix(fp, _linearity, _rows , _cols);
		return true;
	}
	ComponentType GetType() const { return kLinearTransform; }
	void GetType(string &type) const { type = "LinearTransform" ;}
	void PropagateFnc(float *in, int frames, int cols, float *out);
private:
	float *_linearity;
	int _rows;
	int _cols;
};
#include "src/util/namespace-end.h"

#endif

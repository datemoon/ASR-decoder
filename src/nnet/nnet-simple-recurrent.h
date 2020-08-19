#ifndef __NNET_SIMPLE_RECURRENT_H__
#define __NNET_SIMPLE_RECURRENT_H__
#include <string>
#include "src/nnet/matrix.h"
#include "src/nnet/nnet-component.h"


#include "src/util/namespace-start.h"

class SRUcell :public Component
{
public:
	SRUcell(int input_dim,int output_dim):
		Component( input_dim, output_dim), _xfrh_buffer(NULL), _capacity(0), _buffer(NULL),_gh_buffer(NULL),
		_hidden_rows(0),_input_dims(0),_hidden_dim(0),_out_dim(0), _wei_xfrh(NULL), _bias_f(NULL), _bias_r(NULL){ }

	~SRUcell() 
	{
		if(_xfrh_buffer != NULL) delete[] _xfrh_buffer;
		if(_buffer != NULL) delete[] _buffer;
		if(_gh_buffer != NULL) delete[] _gh_buffer;
		if(_wei_xfrh != NULL) delete[] _wei_xfrh;
		if(_bias_f != NULL) delete[] _bias_f;
		if(_bias_r != NULL) delete[] _bias_r;
   	}

	ComponentType GetType() const { return kSRU;}

	void GetType(string &type) const { type = "SRU";}

	void ResetBuf();
	
	void InitBuf();

	bool ReadData(FILE *fp,bool binary);

	bool ReadData(FILE *fp)
	{
		int input_dim = InputDim();
		int output_dim =  OutputDim();
		_input_dims = input_dim;
		_out_dim = output_dim;
		_hidden_rows = _out_dim*4;

		// malloc xfrh weight space.
		_wei_xfrh = new float[_hidden_rows * _input_dims];
		_bias_f = new float[_out_dim];
		_bias_r = new float[_out_dim];

		// read _wei_xfrh
		if (_hidden_rows * _input_dims != (int)fread(_wei_xfrh,sizeof(float),_hidden_rows * _input_dims,fp))
			return false;
		// read _bias_f
		if(_out_dim != (int)fread(_bias_f, sizeof(float),_out_dim,fp))
			return false;
		// read _bias_r
		if(_out_dim != (int)fread(_bias_r, sizeof(float),_out_dim,fp))
			return false;
		return true;
	}

	bool WriteData(FILE *fp)
	{
		if (_hidden_rows * _input_dims != (int)fwrite(_wei_xfrh,sizeof(float),_hidden_rows * _input_dims,fp))
			return false;
		if(_out_dim != (int)fwrite(_bias_f, sizeof(float),_out_dim,fp))
			return false;
		if(_out_dim != (int)fwrite(_bias_r, sizeof(float),_out_dim,fp))
			return false;
		return true;
	}

	void PropagateFnc(float *in, int frames, int cols, float *out );
private:
	void MallocXfrhMemory(int frames);

private:
	float *_xfrh_buffer;
	int _capacity;
	float *_buffer;   // 2 * _out_dim
	float *_gh_buffer; //_out_dim

private:
	int _hidden_rows; //  output dim * 4(xfrh)
	int _input_dims; // input dim
	int _hidden_dim; // hidden cell dim (it's should equal outdim)
	int _out_dim; // output dim

	float *_wei_xfrh;
	float *_bias_f;
	float *_bias_r;
};

#include "src/util/namespace-end.h"

#endif

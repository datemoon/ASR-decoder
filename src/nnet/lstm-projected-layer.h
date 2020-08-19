#ifndef __LSTMPROJECTEDSTREAMS_LAYER_H__
#define __LSTMPROJECTEDSTREAMS_LAYER_H__
#include <string>
#include "src/nnet/matrix.h"
#include "src/nnet/nnet-component.h"

#include "src/util/namespace-start.h"

class LstmProjectedStreams :public Component
{	
public:
	LstmProjectedStreams(int input_dim,int output_dim):
		Component( input_dim, output_dim),
		_gifo_buffer(NULL),_capacity(0),
		_buffer(NULL),
		_wei_gifo_x(NULL),_hidden_rows(0),_input_cols(0),_wei_r_m(NULL),
		_wei_gifo_r(NULL),_bias(NULL),_phole_i_c(NULL),
		_phole_f_c(NULL),_phole_o_c(NULL),_hidden_dim(0), _out_dim(0) { }

	~LstmProjectedStreams()
	{
		if(_gifo_buffer != NULL) delete []_gifo_buffer;
		if(_buffer != NULL) delete []_buffer;
		if(_wei_gifo_x != NULL) delete []_wei_gifo_x;
		if(_wei_r_m != NULL) delete[]_wei_r_m;
		if(_wei_gifo_r != NULL) delete []_wei_gifo_r;
		if(_bias != NULL) delete [] _bias;
		if(_phole_i_c != NULL) delete [] _phole_i_c;
		if(_phole_f_c != NULL) delete [] _phole_f_c;
		if(_phole_o_c != NULL) delete [] _phole_o_c;
	}
	
	ComponentType GetType() const { return kLstmProjectedStreams;}

	void GetType(string &type) const { type = "LstmProjectedStreams";}

	bool ReadData(FILE *fp,bool binary);

	bool ReadData(FILE *fp)
	{
		int input_dim = InputDim();
		int output_dim =  OutputDim();
		_out_dim = output_dim;
		if(1 != (int)fread(&_hidden_dim,sizeof(int),1,fp))
			return false;

		_hidden_rows = _hidden_dim * 4;
		_input_cols = input_dim;
		// malloc gifo weight space.
		_wei_gifo_x = new float[_hidden_rows * _input_cols];
		_wei_gifo_r = new float[_hidden_rows * _out_dim];
		_bias = new float[_hidden_rows];
		_phole_i_c = new float[_hidden_rows];
		_phole_f_c = new float[_hidden_rows];
		_phole_o_c = new float[_hidden_rows];
		_wei_r_m = new float[_out_dim *_hidden_dim];
		if(_hidden_rows * _input_cols != (int)fread(_wei_gifo_x,sizeof(float),_hidden_rows * _input_cols,fp))
		{
			return false;
		}
		if(_hidden_rows * _out_dim != (int)fread(_wei_gifo_r,sizeof(float),_hidden_rows * _out_dim,fp))
		{
			return false;
		}
		if(_hidden_rows != (int)fread(_bias,sizeof(float),_hidden_rows,fp))
			return false;

		if(_hidden_dim != (int)fread(_phole_i_c,sizeof(float),_hidden_dim,fp))
			return false;
		if(_hidden_dim != (int)fread(_phole_f_c,sizeof(float),_hidden_dim,fp))
			return false;
		if(_hidden_dim != (int)fread(_phole_o_c,sizeof(float),_hidden_dim,fp))
			return false;
		if(_out_dim * _hidden_dim != (int)fread(_wei_r_m,sizeof(float), _out_dim * _hidden_dim,fp))
			return false;
		return true;
	}

	bool WriteData(FILE *fp)
	{
		if(1 != (int)fwrite(&_hidden_dim , sizeof(int), 1, fp))
			return false;
		if(_hidden_rows * _input_cols != (int)fwrite(_wei_gifo_x,sizeof(float),_hidden_rows * _input_cols,fp))
		{
			return false;
		}
		if(_hidden_rows * _out_dim != (int)fwrite(_wei_gifo_r,sizeof(float),_hidden_rows * _out_dim,fp))
		{
			return false;
		}
		if(_hidden_rows != (int)fwrite(_bias,sizeof(float),_hidden_rows,fp))
			return false;

		if(_hidden_dim != (int)fwrite(_phole_i_c,sizeof(float),_hidden_dim,fp))
			return false;
		if(_hidden_dim != (int)fwrite(_phole_f_c,sizeof(float),_hidden_dim,fp))
			return false;
		if(_hidden_dim != (int)fwrite(_phole_o_c,sizeof(float),_hidden_dim,fp))
			return false;
		if(_out_dim * _hidden_dim != (int)fwrite(_wei_r_m,sizeof(float), _out_dim * _hidden_dim,fp))
			return false;
		return true;
	}
	void ResetBuf();
	void InitBuf();
	void PropagateFnc(float *in, int frames, int cols, float *out );
private:
	void MallocGifoMemory(int frames);
private:
	float *_gifo_buffer;
	int _capacity;
	float *_buffer;
private:
	float *_wei_gifo_x;
	int _hidden_rows; // hidden cell dim * 4(gifo)
	int _input_cols; // input dim
	float *_wei_r_m;
	float *_wei_gifo_r;
	float *_bias;

	float *_phole_i_c;
	float *_phole_f_c;
	float *_phole_o_c;
	int _hidden_dim; // hidden cell dim
	int _out_dim; // output dim
};
#include "src/util/namespace-end.h"
#endif

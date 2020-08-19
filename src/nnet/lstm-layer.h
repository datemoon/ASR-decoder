#ifndef __LSTM_LAYER_H__
#define __LSTM_LAYER_H__
#include <string>
#include "src/nnet/matrix.h"
#include "src/nnet/nnet-component.h"

#include "src/util/namespace-start.h"

class Lstm :public Component
{	
public:
	Lstm(int input_dim,int output_dim):
		Component( input_dim, output_dim),
		_gifo_buffer(NULL),_capacity(0),
		_buffer(NULL),
		_wei_gifo_x(NULL),_rows(0),_cols(0),
		_wei_gifo_m(NULL),_bias(NULL),_phole_i_c(NULL),
		_phole_f_c(NULL),_phole_o_c(NULL),_dim(0) { }

	~Lstm()
	{
		if(_gifo_buffer != NULL) delete []_gifo_buffer;
		if(_buffer != NULL) delete []_buffer;
		if(_wei_gifo_x != NULL) delete []_wei_gifo_x;
		if(_wei_gifo_m != NULL) delete []_wei_gifo_m;
		if(_bias != NULL) delete [] _bias;
		if(_phole_i_c != NULL) delete [] _phole_i_c;
		if(_phole_f_c != NULL) delete [] _phole_f_c;
		if(_phole_o_c != NULL) delete [] _phole_o_c;
	}
	
	ComponentType GetType() const { return kLstm;}

	void GetType(string &type) const { type = "Lstm";}

	bool ReadData(FILE *fp,bool binary);

	bool ReadData(FILE *fp)
	{
		int input_dim = InputDim();
		int output_dim =  OutputDim();
		_dim = output_dim;
		_rows = output_dim * 4;
		_cols = input_dim;
		// malloc gifo weight space.
		_wei_gifo_x = new float[_rows * _cols];
		_wei_gifo_m = new float[_rows * _dim];
		_bias = new float[_rows];
		_phole_i_c = new float[_rows];
		_phole_f_c = new float[_rows];
		_phole_o_c = new float[_rows];
		if(_rows * _cols != (int)fread(_wei_gifo_x,sizeof(float),_rows * _cols,fp))
		{
			return false;
		}
		if(_rows * _dim != (int)fread(_wei_gifo_m,sizeof(float),_rows * _dim,fp))
		{
			return false;
		}
		if(_rows != (int)fread(_bias,sizeof(float),_rows,fp))
			return false;

		if(_dim != (int)fread(_phole_i_c,sizeof(float),_dim,fp))
			return false;
		if(_dim != (int)fread(_phole_f_c,sizeof(float),_dim,fp))
			return false;
		if(_dim != (int)fread(_phole_o_c,sizeof(float),_dim,fp))
			return false;
		return true;
	}

	bool WriteData(FILE *fp)
	{
		if(_rows * _cols != (int)fwrite(_wei_gifo_x,sizeof(float),_rows * _cols,fp))
		{
			return false;
		}
		if(_rows * _dim != (int)fwrite(_wei_gifo_m,sizeof(float),_rows * _dim,fp))
		{
			return false;
		}
		if(_rows != (int)fwrite(_bias,sizeof(float),_rows,fp))
			return false;

		if(_dim != (int)fwrite(_phole_i_c,sizeof(float),_dim,fp))
			return false;
		if(_dim != (int)fwrite(_phole_f_c,sizeof(float),_dim,fp))
			return false;
		if(_dim != (int)fwrite(_phole_o_c,sizeof(float),_dim,fp))
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
	int _rows; // hidden dim * 4(gifo)
	int _cols; // input dim
	float *_wei_gifo_m;
	float *_bias;

	float *_phole_i_c;
	float *_phole_f_c;
	float *_phole_o_c;
	int _dim;
};
#include "src/util/namespace-end.h"
#endif

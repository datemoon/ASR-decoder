#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "src/nnet/lstm-layer.h"
#include "src/nnet/nnet-util.h"

#include "src/util/namespace-start.h"

void Lstm::ResetBuf()
{
	if(_buffer == NULL)
		InitBuf();
	memset(_buffer,0x00,sizeof(float)* 2 * _dim * 3);
}

void Lstm::InitBuf()
{
	_buffer = new float[2 * _dim * 3]; // c h m
	memset(_buffer,0x00,sizeof(float)*2 * _dim * 3);
}

void Lstm::MallocGifoMemory(int frames)
{
	if(_capacity >= frames * 4 * _dim)
		return;
	_capacity = frames * 4 * _dim;
	float *tmp = new float[_capacity];
	if(_gifo_buffer != NULL)
		delete _gifo_buffer;
	_gifo_buffer = tmp;
}

void Lstm::PropagateFnc(float *in, int frames, int cols, float *out )
{
	// first malloc space , because first calculate MatrixMulMatrix 
	// so I must malloc enough space.
	MallocGifoMemory(frames);

	float *prev_buf = _buffer;
	float *cur_buf = _buffer+_dim * 3;
	float *y_gifo = _gifo_buffer ;

	MatrixMulMatrix(in, _wei_gifo_x, y_gifo, frames, _rows,cols,1.0,0.0);
	AddVecToRows(y_gifo, frames, _rows, _bias, 1.0, 1.0);
//	int prev = 0;
//	int cur = 0;
	for(int t = 0 ; t < frames ; ++t)
	{
		float *prev_y_c = prev_buf;
		float *prev_y_m = prev_buf + 2 * _dim;
		float *y_g = y_gifo + t * 4 * _dim;
		float *y_i = y_g + _dim;
		float *y_f = y_i + _dim;
		float *y_o = y_f + _dim;
		float *y_c = cur_buf ;
		float *y_h = y_c + _dim;
		float *y_m = y_h + _dim;

		//                                     y_g = y_gifo + t * 4 * _dim
		MatrixMulVector(_wei_gifo_m, prev_y_m, y_g, _rows, _dim, 1.0, 1.0);
		// input gate
		VectorMulVector(_phole_i_c, prev_y_c, y_i, _dim, 1.0, 1.0);
		// forget gate
		VectorMulVector(_phole_f_c, prev_y_c, y_f, _dim, 1.0, 1.0);
		// apply sigmoid/tanh functionis to squash the outputs
		DoSigmoid(y_i, 1, _dim, y_i);
		DoSigmoid(y_f, 1, _dim, y_f);
		DoTanH(y_g, 1, _dim, y_g);
		// memory cell
		VectorMulVector(y_i, y_g, y_c, _dim, 1.0, 0.0);
		VectorMulVector(y_f, prev_y_c, y_c, _dim, 1.0, 1.0);
		// h - the tanh-squashed version of c
		DoTanH(y_c, 1, _dim, y_h);

		// output gate
		VectorMulVector(_phole_o_c, y_c, y_o, _dim, 1.0, 1.0);
		DoSigmoid(y_o, 1.0, _dim, y_o);

		// finally the outputs
		VectorMulVector(y_o, y_h, y_m, _dim, 1.0, 0.0);

		// swap _buffer
		memcpy(prev_buf,cur_buf,_dim*3*sizeof(float));
		//copy to out
		memcpy(out+t*_dim,y_m,_dim*sizeof(float));
	}// end of loop
}

bool Lstm::ReadData(FILE *fp,bool binary)
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
	
	// read _wei_gifo_x
	ReadMatrix(fp, _wei_gifo_x , _rows, _cols);
	// read _wei_gifo_x ok.
	
	// read _wei_gifo_m
	ReadMatrix(fp, _wei_gifo_m , _rows, _dim);	
	
	// read _bias
	ReadMatrix(fp, _bias , 1, _rows);

	// read _phole_i_c
	ReadMatrix(fp, _phole_i_c, 1, _dim);
	
	// read _phole_f_c
	ReadMatrix(fp, _phole_f_c, 1, _dim);
	
	// read _phole_o_c
	ReadMatrix(fp, _phole_o_c, 1, _dim);
	return true;
}
#include "src/util/namespace-end.h"

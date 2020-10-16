#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "src/nnet/lstm-projected-layer.h"
#include "src/nnet/nnet-util.h"

#include "src/util/namespace-start.h"

void LstmProjectedStreams::ResetBuf()
{
	if(_buffer == NULL)
		InitBuf();
	memset(_buffer,0x00,sizeof(float)* 2 * _hidden_dim * 4);
}

void LstmProjectedStreams::InitBuf()
{
	_buffer = new float[2 * _hidden_dim * 4]; // c h m r , here r is _out_dim,because _out_dim < _hidden_dim , so there I can't malloc the real space.
	memset(_buffer,0x00,sizeof(float)*2 * _hidden_dim * 4);
}

void LstmProjectedStreams::MallocGifoMemory(int frames)
{
	if(_capacity >= frames * 4 * _hidden_dim)
		return;
	_capacity = frames * 4 * _hidden_dim;
	float *tmp = new float[_capacity];
	if(_gifo_buffer != NULL)
		delete[] _gifo_buffer;
	_gifo_buffer = tmp;
}

void LstmProjectedStreams::PropagateFnc(float *in, int frames, int cols, float *out )
{
	// first malloc space , because first calculate MatrixMulMatrix 
	// so I must malloc enough space.
	MallocGifoMemory(frames);

	float *prev_buf = _buffer;
	float *cur_buf = _buffer+_hidden_dim * 4;
	float *y_gifo = _gifo_buffer ;

	MatrixMulMatrix(in, _wei_gifo_x, y_gifo, frames, _hidden_rows,cols,1.0,0.0);
	AddVecToRows(y_gifo, frames, _hidden_rows, _bias, 1.0, 1.0);
//	int prev = 0;
//	int cur = 0;
	for(int t = 0 ; t < frames ; ++t)
	{
		float *prev_y_c = prev_buf;
		float *prev_y_r = prev_buf + 3 * _hidden_dim;
		float *y_g = y_gifo + t * 4 * _hidden_dim;
		float *y_i = y_g + _hidden_dim;
		float *y_f = y_i + _hidden_dim;
		float *y_o = y_f + _hidden_dim;
		float *y_c = cur_buf ;
		float *y_h = y_c + _hidden_dim;
		float *y_m = y_h + _hidden_dim;
		float *y_r = y_m + _hidden_dim;

		//                                     y_g = y_gifo + t * 4 * _dim
		MatrixMulVector(_wei_gifo_r, prev_y_r, y_g, _hidden_rows, _out_dim, 1.0, 1.0);
		// input gate
		VectorMulVector(_phole_i_c, prev_y_c, y_i, _hidden_dim, 1.0, 1.0);
		// forget gate
		VectorMulVector(_phole_f_c, prev_y_c, y_f, _hidden_dim, 1.0, 1.0);
		// apply sigmoid/tanh functionis to squash the outputs
		DoSigmoid(y_i, 1, _hidden_dim, y_i);
		DoSigmoid(y_f, 1, _hidden_dim, y_f);
		DoTanH(y_g, 1, _hidden_dim, y_g);
		// memory cell
		VectorMulVector(y_i, y_g, y_c, _hidden_dim, 1.0, 0.0);
		VectorMulVector(y_f, prev_y_c, y_c, _hidden_dim, 1.0, 1.0);
		// h - the tanh-squashed version of c
		DoTanH(y_c, 1, _hidden_dim, y_h);

		// output gate
		VectorMulVector(_phole_o_c, y_c, y_o, _hidden_dim, 1.0, 1.0);
		DoSigmoid(y_o, 1.0, _hidden_dim, y_o);

		// finally the outputs
		VectorMulVector(y_o, y_h, y_m, _hidden_dim, 1.0, 0.0);

		MatrixMulMatrix(y_m, _wei_r_m, y_r, 1.0, _out_dim, _hidden_dim,1.0,0.0);
		// swap _buffer
		memcpy(prev_buf,cur_buf,_hidden_dim*4*sizeof(float));
		//copy to out
		memcpy(out+t*_out_dim, y_r, _out_dim*sizeof(float));
	}// end of loop
}

bool LstmProjectedStreams::ReadData(FILE *fp,bool binary)
{
	const unsigned len = 1024;
	char line[len];
	memset(line,0x00,sizeof(char)*len);
	ReadLine(fp,line,len);
	char *token = NULL;
	char *ptr = NULL;
	token = strtok_r(line," ",&ptr);
	if(strncmp(token, "<CellDim>", 9) == 0)
	{
		token = strtok_r(NULL," ",&ptr);
		_hidden_dim = atoi(token); // hidden dim
		token = strtok_r(NULL," ",&ptr);
	}
	if(strncmp(token, "<ClipGradient>", 14) == 0)
	{
		token = strtok_r(NULL," ",&ptr);
		token = strtok_r(NULL," ",&ptr);
	}
	int input_dim = InputDim();
	int output_dim =  OutputDim();
	_out_dim = output_dim;
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

	// read _wei_gifo_x
	ReadMatrixData(fp, _wei_gifo_x , _hidden_rows, _input_cols);
	// read _wei_gifo_x ok.
	
	// read _wei_gifo_r
	ReadMatrixData(fp, _wei_gifo_r , _hidden_rows, _out_dim);	
	
	// read _bias
	ReadMatrixData(fp, _bias , 1, _hidden_rows);

	// read _phole_i_c
	ReadMatrixData(fp, _phole_i_c, 1, _hidden_dim);
	
	// read _phole_f_c
	ReadMatrixData(fp, _phole_f_c, 1, _hidden_dim);
	
	// read _phole_o_c
	ReadMatrixData(fp, _phole_o_c, 1, _hidden_dim);

	// read _wei_r_m
	ReadMatrixData(fp, _wei_r_m , _out_dim ,_hidden_dim);
	return true;
}
#include "src/util/namespace-end.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "src/nnet/nnet-simple-recurrent.h"
#include "src/nnet/nnet-util.h"

#include "src/util/namespace-start.h"

bool SRUcell::ReadData(FILE *fp,bool binary)
{
	int input_dim = InputDim();
	int output_dim =  OutputDim();
	_out_dim = output_dim;
	_hidden_rows = _out_dim * 4;
	_input_dims = input_dim;
/*	const unsigned len = 1024;
	char line[len];
	memset(line,0x00,sizeof(char)*len);
	ReadLine(fp,line,len);
	char *token = NULL;
	char *ptr = NULL;
	token = strtok_r(line," ",&ptr);
	while(token != NULL)
	{
		if(strncmp(token, "<CellDim>", 9) == 0)
		{
			token = strtok_r(NULL," ",&ptr);
			_hidden_dim = atoi(token); // hidden dim
			assert(_hidden_dim == _out_dim && "hidden_dim should be equal out_dim");
		}
		else if(strncmp(token, "<LearnRateCoef>", 15) == 0)
		{
			token = strtok_r(NULL," ",&ptr);
			//float lrc = (float)atof(token);
		}
		else if(strncmp(token, "<BiasLearnRateCoef>", 19) == 0)
		{
			token = strtok_r(NULL," ",&ptr);
			//float blrc = (float)atof(token);
		}
		else if(strncmp(token, "<GradClip>", 10) == 0)
		{
			token = strtok_r(NULL," ",&ptr);
			//float gc = (float)atof(token);
		}
		token = strtok_r(NULL," ",&ptr);
	}
*/
	// malloc xfrh weight space.
	_wei_xfrh = new float[_hidden_rows * _input_dims];
	_bias_f = new float[_out_dim];
	_bias_r = new float[_out_dim];
	
	// read _wei_xfrh
	ReadMatrixData(fp, _wei_xfrh , _hidden_rows, _input_dims);

	// read _bias_f
	ReadMatrixData(fp, _bias_f, 1, _out_dim);
	// read _bias_r
	ReadMatrixData(fp, _bias_r, 1, _out_dim);
	return true;
}


void SRUcell::MallocXfrhMemory(int frames)
{
	if(_capacity >= frames * 4 * _out_dim)
		return;
	_capacity = frames * 4 * _out_dim;
	float *tmp = new float[_capacity];
	if(_xfrh_buffer != NULL)
		delete[] _xfrh_buffer;
	_xfrh_buffer = tmp;
}

void SRUcell::ResetBuf()
{
	if(_buffer == NULL)
		InitBuf();
	memset(_buffer,0x00,sizeof(float)* 2 * _out_dim);
}
void SRUcell::InitBuf()
{
	_buffer = new float[2 * _out_dim]; // c neet t-1 save
	memset(_buffer,0x00,sizeof(float)*2 * _out_dim);
	_gh_buffer = new float[_out_dim * 2];
	memset(_gh_buffer,0x00,sizeof(float)*2 * _out_dim);
}

void SRUcell::PropagateFnc(float *in, int frames, int cols, float *out )
{
	// first malloc space , because first calculate MatrixMulMatrix
	// so I must malloc enough space.
	MallocXfrhMemory(frames);

	float *prev_buf = _buffer;
	float *cur_buf = _buffer+_out_dim;
	float *y_xfrh = _xfrh_buffer;

	MatrixMulMatrix(in, _wei_xfrh, y_xfrh, frames, _hidden_rows,cols,1.0,0.0);
	AddVecToRowsRange(y_xfrh, frames, _hidden_rows, _bias_f, _out_dim, _out_dim*1, 1.0, 1.0);
	AddVecToRowsRange(y_xfrh, frames, _hidden_rows, _bias_r, _out_dim, _out_dim*2, 1.0, 1.0);
	for(int t = 0 ; t < frames ; ++t)
	{
		float *prev_y_c = prev_buf;
		float *y_x = y_xfrh + t * 4 * _out_dim;
		float *y_f = y_x + _out_dim;
		float *y_r = y_f + _out_dim;
		float *y_ah = y_r + _out_dim;

		float *y_c = cur_buf;
		memset(y_c,0x00,sizeof(float) * _out_dim);
		float *y_g = _gh_buffer;
		float *y_h = y_g + _out_dim;
		memset(_gh_buffer,0x00,sizeof(float)* 2 * _out_dim);
		DoSigmoid(y_f,1,_out_dim,y_f);
		DoSigmoid(y_r,1,_out_dim,y_r);
		//c_t = f_t * c_t-1 + (1 - f_t) * ax_t = ax_t - f_t * ax_t + f_t * c_t-1
		MatAddMat(y_c, 1 , _out_dim, y_x, 1.0);
		VectorMulVector(y_f, y_x, y_c, _out_dim, -1.0, 1.0);
		VectorMulVector(y_f, prev_y_c, y_c, _out_dim, 1.0, 1.0);
		// gc_t = c_t
		MatAddMat(y_g, 1 , _out_dim, y_c, 1.0);
		// h_t = r_t * gc_t + (1 - r_t) * ah_t = ah_t - r_t * ah_t + r_t * gc_t
		MatAddMat(y_h, 1, _out_dim, y_ah, 1.0);
		VectorMulVector(y_r, y_ah, y_h, _out_dim, -1.0, 1.0);
		VectorMulVector(y_r, y_c, y_h, _out_dim, 1.0, 1.0);

		// swap _buffer
		memcpy(prev_buf, cur_buf, _out_dim * sizeof(float));
		// copy to out
		memcpy(out+t*_out_dim, y_h, _out_dim * sizeof(float));
	}// end of loop
}
#include "src/util/namespace-end.h"

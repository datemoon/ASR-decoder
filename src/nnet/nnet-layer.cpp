#include <string.h>
#include <assert.h>
#include "src/nnet/nnet-layer.h"
#include "src/nnet/matrix.h"

#include "src/util/namespace-start.h"
void Splice::PropagateFnc(float *in, int frames, int cols, float *out)
{
	assert(InputDim() == cols);
	// here _offsets must be continues.
	for(int i=0;i<frames; ++i)
	{
		int offset = i * (1 + _skip_frames);
		memcpy(out+i*OutputDim(),in+offset*InputDim(),sizeof(float)*OutputDim());
	}
}

void AddShift::PropagateFnc(float *in, int frames, int cols, float *out)
{
	assert(cols == _dim);
	AddVecToRows(in, frames, cols, _shift_data, 1.0, 1.0);
	memcpy(out,in,sizeof(float)*frames*cols);
}

void Prior::PropagateFnc(float *in, int frames, int cols, float *out)
{
	assert(cols == _dim);
	//DoLog(in,frames,cols,out);
	memcpy(out,in,sizeof(float)*frames*cols);
	AddVecToRows(out, frames, cols, _log_priors, -1.0, 1.0);
}

void Rescale::PropagateFnc(float *in, int frames, int cols, float *out)
{
	MulColsVec(in, frames, cols, _scale_data);
	memcpy(out,in,sizeof(float)*frames*cols);
}

void Sigmoid::PropagateFnc(float *in, int frames, int cols, float *out)
{
	DoSigmoid(in,frames,cols,out);
}

void Softmax::PropagateFnc(float *in, int frames, int cols, float *out)
{
	DoSoftmax(in,frames,cols,out);
}

void AffineTransform::PropagateFnc(float *in, int frames, int cols, float *out)
{
	memset(out,0x00,sizeof(float)*frames*OutputDim());
	AddVecToRows(out, frames, OutputDim(), _bias, 1.0, 1.0);
	MatrixMulMatrix(in,_linearity,out,frames,_rows,cols,1.0,1.0);
}

void LinearTransform::PropagateFnc(float *in, int frames, int cols, float *out )
{
	memset(out,0x00,sizeof(float)*frames*OutputDim());
	MatrixMulMatrix(in,_linearity,out,frames,_rows,cols,1.0,1.0);
}

#include "src/util/namespace-end.h"

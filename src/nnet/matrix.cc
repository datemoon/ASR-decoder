#include "src/nnet/matrix.h"
#ifdef CBLAS
extern "C"
{
#include "src/lib/cblas.h"
}
#endif
#include "src/util/namespace-start.h"

/*  alpha*A*B + beta * C **/
// m : A matrix row , C matrix row
// n : B matrix col , C matrix col
// k : A matrix col , B matrix row
void MatrixMulMatrix(float* A,float* B,float* C,int m,int n,int k,float alpha,float beta)
{
	cblas_sgemm(CblasRowMajor,CblasNoTrans,CblasTrans,m,n,k,alpha,A,k,B,k,beta,C,n);
}

void MatrixMulVector(float *A,float *B,float *C,int arow,int acol,float alpha,float beta)
{
	cblas_sgemv(CblasRowMajor,CblasNoTrans,arow,acol,alpha,A,acol,B,1,beta,C,1);
}

// (alpha * A[i] + B[i]) + beta * C[i] = C[i]
void VectorMulVector(float *A,float *B,float *C,int dim,float alpha,float beta)
{
	cblas_sgbmv(CblasRowMajor,CblasNoTrans,dim,dim,0,0,alpha,A,1,B,1,beta,C,1);
}

void AddVecToRows(float *A,int row,int col,float *B,float alpha,float beta)
{
	int num = row * col;
	for(int i= 0 ; i < num ; ++i)
	{
		A[i] += alpha * B[i%col];
	}
}
//B = alpha * A  +  B
void MatAddMat(float *A,int row,int col, float *B, float alpha)
{
	for(int i=0; i<row; ++i,A+=col,B+=col)
	{
		cblas_saxpy(col, alpha, B, 1, A,1);
	}
}

void AddVecToRowsRange(float *A,int row,int col,float *B, int B_col, int offset,float alpha,float beta)
{
	for(int i=0; i<row; ++i)
	{
		for(int j=0; j< B_col;++j)
		{
			A[i*col + offset + j] += B[j];
		}
	}
}

void MulColsVec(float *A,int row,int col,float *B)
{
	int offset = 0;
	for(int i =0;i<row;++i)
	{
		for(int k=0;k<col;++k)
			A[offset+k] *= B[k];
		offset += col;
	}
}

void DoSigmoid(float *A,int row,int col,float *B)
{
	int num = row * col ;
	for(int i=0;i<num;i++)
	{
		float x = A[i];
		if (x > 0.0) 
		{
			x = 1.0 / (1.0 + Exp(-x));
		} 
		else 
		{
			float ex = Exp(x);
			x = ex / (ex + 1.0);
		}
		B[i] = x;
	}
	return ;
}

float DoMax(float *data,int dim)
{
	float max = data[0];
	for(int i=1;i<dim;++i)
	{
		if(max < data[i])
			max = data[i];
	}
	return max;
}

void DoSoftmax(float *A,int row,int col,float *B)
{
	int offset = 0;
	for(int i=0;i<row;i++)
	{
		float *data = A + offset ;
		float *bdata = B + offset;
		float max = DoMax(data,col);
		float sum = 0.0;
		for(int j=0;j<col;j++)
		{
			sum += (bdata[j] = Exp(data[j] - max)) ;
		}
		cblas_sscal(col,1.0/sum,bdata,1);
		offset += col;
	}
	return ;
}

void DoLog(float *A,int row,int col,float *B)
{
	int i=0;
	for(i=0;i<row*col;++i)
	{
		B[i] = Log(A[i]+(1e-20)); // ln()
	}
}

void DoTanH(float *A,int row,int col,float *B)
{
	for(int i=0;i<row*col;++i)
	{
		float x = A[i];
		if(x > 0.0)
		{
			float e = Exp(-x);
			x = -1.0 + 2.0 / (1.0 + e * e);
		}
		else
		{
			float e = Exp(x);
			x = 1.0 - 2.0 / (1.0 + e * e);
		}
		B[i] = x;
	}
}
#include "src/util/namespace-end.h"

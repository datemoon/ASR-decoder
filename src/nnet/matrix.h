#ifndef __MATRIX_H__
#define __MATRIX_H__
#include <math.h>
#define Exp(x) expf(x)
#define Log(x) logf(x)

#include "src/util/namespace-start.h"
/*  alpha*A*B + beta * C **/
// m : A matrix row , C matrix col
// n : B matrix col , C matrix row
// k : A matrix col , B matrix row
void MatrixMulMatrix(float* A,float* B,float* C,int m,int n,int k,float alpha,float beta);

void MatrixMulVector(float *A,float *B,float *C,int arow,int acol,float alpha,float beta);

void VectorMulVector(float *A,float *B,float *C,int dim,float alpha,float beta);

void AddVecToRows(float *A,int row,int col,float *B,float alpha,float beta);

void MulColsVec(float *A,int row,int col,float *B);

void MatAddMat(float *A,int row,int col, float *B, float alpha);

void AddVecToRowsRange(float *A,int row,int col,float *B, int B_col, int offset,float alpha,float beta);

void DoSigmoid(float *A,int row,int col,float *B);

float DoMax(float *data,int dim);

void DoSoftmax(float *A,int row,int col,float *B);

void DoLog(float *A,int row,int col,float *B);

void DoTanH(float *A,int row,int col,float *B);

#include "src/util/namespace-end.h"
#endif

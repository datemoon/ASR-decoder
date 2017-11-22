#ifndef __NNET_UTIL_H__
#define __NNET_UTIL_H__

void ReadMatrixData(FILE *fp,float *mat,int rows,int cols);

void ReadMatrix(FILE *fp,float *mat,int row,int col);

void ReadMatrix(FILE *fp,int *mat,int row,int col);

void ReadLine(FILE *fp,char *str,int len);

template <typename Real>
void PrintPoint(Real mat,int rows,int cols);

template <typename Real>
int MaxVec(Real vec,int n);
#endif

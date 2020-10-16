#include <iostream>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "src/nnet/nnet-util.h"

#include "src/util/namespace-start.h"

bool ReadScp(FILE *fp, std::string &key, std::string &file, long &offset)
{
	const unsigned len = 2048;
	char line[len];
	memset(line,0x00,sizeof(char)*len);
	char *token = NULL;
	if(true != ReadLine(fp,line,len))
		return false;
	char *ptr = NULL;
	token = strtok_r(line," ",&ptr);
	key = token;
	token = strtok_r(NULL,":",&ptr);
	file = token;
//	token = strtok_r(NULL," ",&ptr);
//	offset = atol(token);
	return true;
}

void ReadMatrixData(FILE *fp,float *mat,int rows,int cols)
{
	const unsigned len = 20480*20;
	char line[len];
	memset(line,0x00,sizeof(char)*len);
	char *token = NULL;
	int row = 0,col = 0;
	while(1)
	{
		ReadLine(fp,line,len);
		char *ptr = NULL;
		token = strtok_r(line," ",&ptr);
		if( strncmp(token,"[",1) == 0)
		{
			token = strtok_r(NULL," ",&ptr);
		}
		if(token == NULL || strlen(token) == 0)
			continue;

		if(strncmp(token,"]",1) == 0)
			break;
		float weight = (float)atof(token);
		col = 0;
		mat[row*cols+col] = weight;
		col ++;
		while((token = strtok_r(NULL," ",&ptr)) != NULL)
		{
			if(strncmp(token,"]",1) == 0)
				break;
			mat[row*cols+col] = (float)atof(token);
			col ++;
		}
		if(col == cols)
			row++;
		assert(col == cols);
		if(token == NULL)
			continue;
		if(strncmp(token,"]",1) == 0)
			break;
	}
	assert(row == rows);
}
void ReadMatrix(FILE *fp,float *mat,int rows,int cols)
{
	const unsigned len = 20480*20;
	char line[len];
	memset(line,0x00,sizeof(char)*len);
	char *token = NULL;
	int row = 0,col = 0;
	int flag = 0;
	while(1)
	{
		ReadLine(fp,line,len);
		char *ptr = NULL;
		token = strtok_r(line," ",&ptr);
		if(strncmp(token, "<LearnRateCoef>", 15) == 0)
		{
			token = strtok_r(NULL," ",&ptr);
			token = strtok_r(NULL," ",&ptr);
		}
		if(strncmp(token, "<MaxGrad>", 15) == 0)
		{
			token = strtok_r(NULL," ",&ptr);
			token = strtok_r(NULL," ",&ptr);
		}
		while(flag == 0)
		{
			if(strncmp(token,"[",1) == 0)
			{
				flag = 1;
				token = strtok_r(NULL," ",&ptr);
				break;
			}
			token = strtok_r(NULL," ",&ptr);
			if(token == NULL)
				break;
		}
		if(token == NULL || flag != 1)
			continue;

		float weight = (float)atof(token);
		col = 0;
		mat[row*cols+col] = weight;
		col ++;
		while((token = strtok_r(NULL," ",&ptr)) != NULL)
		{
			if(strncmp(token,"]",1) == 0)
				break;
			mat[row*cols+col] = (float)atof(token);
			col ++;
		}
		if(col == cols)
			row++;
		assert(col == cols);
		if(token == NULL)
			continue;
		if(strncmp(token,"]",1) == 0)
			break;
	}
	assert(row == rows);
}

void ReadMatrix(FILE *fp,int *mat,int rows,int cols)
{
	const unsigned len = 2048*20;
	char line[len];
	memset(line,0x00,sizeof(char)*len);
	char *token = NULL;
	int row = 0,col = 0;
	int flag = 0;
	while(1)
	{
		char *ptr = NULL;
		ReadLine(fp,line,len);
		token = strtok_r(line," ",&ptr);
		if(strncmp(token, "<LearnRateCoef>", 15) == 0)
		{
			token = strtok_r(NULL," ",&ptr);
			token = strtok_r(NULL," ",&ptr);
		}
		if(strncmp(token, "<MaxGrad>", 15) == 0)
		{
			token = strtok_r(NULL," ",&ptr);
			token = strtok_r(NULL," ",&ptr);
		}
		while(flag == 0)
		{
			if(strncmp(token,"[",1) == 0)
			{
				flag = 1;
				token = strtok_r(NULL," ",&ptr);
				break;
			}
			token = strtok_r(NULL," ",&ptr);
			if(token == NULL)
				break;
		}

		if(token == NULL || flag == 0)
			continue;

		int weight = atoi(token);
		col = 0;
		mat[row*cols+col] = weight;
		col ++;
		while((token = strtok_r(NULL," ",&ptr)) != NULL)
		{
			if(strncmp(token,"]",1) == 0)
				break;
			mat[row*cols+col] = atoi(token);
			col ++;
		}
		if(col == cols)
			row++;
		assert(col == cols);
		if(token == NULL)
			continue;
		if(strncmp(token,"]",1) == 0)
			break;
	}
	assert(row == rows);
}

bool ReadLine(FILE *fp,char *str,int len)
{
	memset(str,0x00,sizeof(char)*len);
	char *line = fgets(str, len, fp);
	if(line == NULL)
		return false;
	str[strlen(str)-1] = '\0';
	return true;
}

template <typename Real>
void PrintPoint(Real mat,int rows,int cols)
{
	for(int r=0;r<rows;++r)
	{
		for(int c=0;c<cols;++c)
			std::cout << mat[r*cols+c] << " ";
		std::cout << std::endl;
	}
}
template
void PrintPoint<float*>(float* mat,int rows, int cols);

template <typename Real>
int MaxVec(Real *vec,int n)
{
	if(n<=0)
		return -1;
	Real max = vec[0];
	int maxid=0;
	for(int i=1;i<n;++i)
	{
		if(max < vec[i])
		{
			max = vec[i];
			maxid=i;
		}
	}
	return maxid;
}

template
int MaxVec<float>(float *vec,int n);
#include "src/util/namespace-end.h"

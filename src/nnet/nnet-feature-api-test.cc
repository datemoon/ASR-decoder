#include <stdio.h>
#include "src/nnet/nnet-feature-api.h"

#ifdef NAMESPACE
using namespace datemoon;
#endif

int main(int argc,char *argv[])
{
	DnnFeat *dnnfeat = new DnnFeat;
	dnnfeat->Init(argv[1],"./");
	int left = 35;
	int right = 10;
	dnnfeat->InitParameters(left,right);
#define LEN (1024)
	char data[LEN];
	FILE *fp = fopen(argv[2],"rb");
	if(fp == NULL)
	{
		fprintf(stderr,"fopen %s error.\n",argv[2]);
		return -1;
	}
	fread(data,sizeof(char),44,fp);
	int frame = 0;
	int len = LEN;
	int nnet_in_frame = 0;
	int dim = 0;
	float *feats = NULL;
	while(0 != (len = fread(data,sizeof(char),len,fp)))
	{
		dnnfeat->GetFeats(data,len,&feats,&nnet_in_frame,&dim,0);
		// use features
		for(int k=0;k<nnet_in_frame;k++)
		{
			printf("%d@ ",frame+k);
			for(int m =0; m <dim;++m)
				printf("%6.3f ",feats[(k+left)*dim+m]);
			printf("\n");
		}
		frame += nnet_in_frame;
		// delete features
		dnnfeat->RemoveFeats(nnet_in_frame);
	}
	dnnfeat->GetFeats(data,len,&feats,&nnet_in_frame,&dim,1);
	for(int k=0;k<nnet_in_frame;k++)
	{
		printf("%d@ ",frame+k);
		for(int m =0; m <dim;++m)
			printf("%6.3f ",feats[(k+left)*dim+m]);
		printf("\n");
	}
	frame += nnet_in_frame;
	dnnfeat->RemoveFeats(nnet_in_frame);
	fclose(fp);
	delete dnnfeat;
	return 0;
}

#include <stdio.h>
#include "src/nnet/nnet-nnet.h"
#include "src/nnet/nnet-feature-api.h"

#ifdef NAMESPACE
using namespace datemoon;
#endif

int main(int argc,char *argv[])
{
	if(argc != 2)
	{
		fprintf(stderr,"%s nnetmodel\n",argv[0]);
		return -1;
	}
	Nnet *nnet = new Nnet;
	nnet->ReadNnet(argv[1],false);
	return 0;
	// new source.
	NnetForward *forward = new NnetForward(nnet,2,false,false);
	int left = 0, right =0 ;
	
	forward->GetLRoffset(left,right);
	DnnFeat *dnnfeat = new DnnFeat;
	dnnfeat->Init(argv[1],"./");
	dnnfeat->InitParameters(left,right);
#define LEN (1024)
	/*
	char data[LEN];
	FILE *fp = fopen(argv[3],"rb");
	if(NULL == fp)
	{
		fprintf(stderr,"fopen %s error.\n",argv[3]);
		return -1;
	}
	fread(data,sizeof(char),44,fp);
	int len = LEN;
*/	int frame = 0;
	int nnet_in_frame = 0;
	int dim = 0;
	float *feats = NULL;
	while(1)
	{
		dnnfeat->GetFeats(argv[3],&feats,&nnet_in_frame,&dim);
		// use features.

		forward->ResetRnnBuffer();
		forward->FeedForward(feats,nnet_in_frame,dim);
#define DEBUG
#ifdef DEBUG
		int outdim = forward->GetOutdim();
		for(int i = 0;i<nnet_in_frame;++i)
		{
			printf("%d@ ",frame+i);
			if(false == forward->ExamineFrame(frame+i))
			{
				fprintf(stderr,"it's nnet forward error.\n");
				return -1;
			}
			for(int j=0;j<outdim;++j)
				printf("%7.6f ",forward->LogLikelihood(frame+i,j));
			printf("\n");
		}
#endif
		frame +=nnet_in_frame;
		dnnfeat->RemoveFeats(nnet_in_frame);
		break;
	}

//	fclose(fp);
	delete dnnfeat;
	delete forward;
	delete nnet;
	return 0;
}

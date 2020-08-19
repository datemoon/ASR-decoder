#include <stdio.h>
#include <sys/time.h>
#include "src/nnet/nnet-nnet.h"
#include "src/nnet/nnet-feature-api.h"

#ifdef NAMESPACE
using namespace datemoon;
#endif

int main(int argc,char *argv[])
{
	if(argc != 4)
	{
		fprintf(stderr,"%s conf nnetmodel feat\n",argv[0]);
		return -1;
	}
	struct timeval start,end;
	float all_nnettime = 0;

	Nnet *nnet = new Nnet;
	//nnet->ReadNnet(argv[2]);
	nnet->ReadNnet(argv[2],true);
	// new source.
	//NnetForward *forward = new NnetForward(nnet,2,false,false);
	NnetForward *forward = new NnetForward(nnet,0,false,false);
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
	gettimeofday(&start,NULL);
	while(1)
	{
		dnnfeat->GetFeats(argv[3],&feats,&nnet_in_frame,&dim);
		all_nnettime += nnet_in_frame;
		// use features.

		forward->ResetRnnBuffer();
		int send_frame = 1;
		frame=0;
		for(int i=0;i<nnet_in_frame;i+=send_frame)
		{
			forward->FeedForward(feats,send_frame,dim);
#define DEBUG
#ifdef DEBUG
			int outdim = forward->GetOutdim();
			int k=0;
			for(k = frame;k<nnet_in_frame;++k)
			{
				if(false == forward->ExamineFrame(frame))
				{
					break;
					fprintf(stderr,"it's nnet forward error.\n");
					return -1;
				}
				printf("%d@ ",frame);
				for(int j=0;j<outdim;++j)
					printf("%7.6f ",forward->LogLikelihood(frame,j+1));
				printf("\n");
				++frame;
			}
			frame=k;
#endif
			dnnfeat->RemoveFeats(send_frame);
		}
		break;
	}
	gettimeofday(&end,NULL);
	float usetime = end.tv_sec - start.tv_sec + (end.tv_usec - start.tv_usec)/1000000.0;
	printf("nnet rt is %f\n",usetime*100/all_nnettime);
//	fclose(fp);
	delete dnnfeat;
	delete forward;
	delete nnet;
	return 0;
}

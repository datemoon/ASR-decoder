#include <stdio.h>
#include "src/decoder/wordid-to-wordstr.h"
#include "src/decoder/optimize-fst.h"
#include "src/decoder/optimize-faster-decoder.h"
#include "src/nnet/nnet-nnet.h"
#include "src/nnet/nnet-feature-api.h"

#ifdef NAMESPACE
using namespace datemoon;
#endif

int main(int argc,char *argv[])
{
	if(argc != 5)
	{
		fprintf(stderr,"%s conf nnetmodel fst wav\n",argv[0]);
		return -1;
	}
	Nnet *nnet = new Nnet;
	nnet->ReadNnet(argv[2]);
	// new source.
	NnetForward *forward = new NnetForward(nnet);
	int left = 0, right =0 ;
	
	forward->GetLRoffset(left,right);
	DnnFeat *dnnfeat = new DnnFeat;
	dnnfeat->Init(argv[1],"./");
	dnnfeat->InitParameters(left,right);

	Fst fst;
	fst.ReadFst(argv[3]);

	FasterDecoderOptions decodeopt(16.0,7000,20,0.5,2.0);

	FasterDecoder decode(&fst, &decodeopt);
	decode.InitDecoding();

#define LEN (1024)
	char data[LEN];
	FILE *fp = fopen(argv[4],"rb");
	if(NULL == fp)
	{
		fprintf(stderr,"fopen %s error.\n",argv[4]);
		return -1;
	}
	fread(data,sizeof(char),44,fp);
	int frame = 0;
	int len = LEN;
	int nnet_in_frame = 0;
	int dim = 0;
	float *feats = NULL;
	forward->ResetRnnBuffer();
	while(0 != (len = fread(data,sizeof(char),len,fp)))
	{
		int end = 0;
		if(feof(fp))
			end = 1;
		dnnfeat->GetFeats(data,len,&feats,&nnet_in_frame,&dim,end);
		// use features.
		
		forward->FeedForward(feats,nnet_in_frame,dim);

		int max_frames = forward->NumFramesReady();
		decode.AdvanceDecoding(forward,max_frames);

		if(max_frames != frame+nnet_in_frame)
			fprintf(stderr,"it's a serious bug. %d != %d\n",max_frames,frame+nnet_in_frame);
//#define DEBUGPRINT
#ifdef DEBUGPRINT
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
	}
	vector<int> best_words_arr;
	vector<int> best_phones_arr;
//	printf("%s	",linefeat);
	if(decode.GetBestPath(best_words_arr, best_phones_arr))
	{
/*		for(unsigned i = 0; i < best_words_arr.size(); ++i)
		{
			if(NULL != wordsymbol)
				printf("%s ",wordsymbol->FindWordStr(best_words_arr[i]));
		}
		printf("\n");
		if(NULL == wordsymbol)
*/		{
			printf("words number %d: ",(int)best_words_arr.size());
			for(unsigned i = 0; i < best_words_arr.size(); ++i)
				//printf("(%d) %d \n",i,best_words_arr[i]);
				printf(" %d",best_words_arr[i]);
			printf("\n");
			for(unsigned i = 0; i < best_phones_arr.size(); ++i)
				//printf("(%d) %d \n",i,best_phones_arr[i]);
				printf(" %d",best_phones_arr[i]);
			printf("\n");
		}
	}
	else
		printf("no result.\n");

	fclose(fp);
	delete dnnfeat;
	delete forward;
	delete nnet;
	return 0;
}

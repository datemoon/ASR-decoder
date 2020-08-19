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
	bool do_log=false;
	NnetForward *forward = new NnetForward(nnet,0,do_log);
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
	std::string feat_list=argv[4];
	int nnet_in_frame = 0;
	int dim = 0;
	float *feats = NULL;
	FILE *featfp = fopen(feat_list.c_str(),"r");
	if(NULL == featfp)
	{
		fprintf(stderr,"fopen %s failed.\n", feat_list.c_str());
		return -1;
	}
//	float all_wavtime = 0;
//	float all_decodetime  = 0;
//	float all_nnettime = 0;
	char linefeat[LEN];

	while(NULL != fgets(linefeat,1024,featfp))
	{
		linefeat[strlen(linefeat)-1] = '\0';
		if(linefeat[0] == '#' || isspace((int)linefeat[0]))
			continue;
#ifdef DEBUGPRINT
		int frame = 0;
#endif
		while(1)
		{
			forward->Reset();
			decode.InitDecoding();
			dnnfeat->Reset();
			dnnfeat->GetFeats(linefeat,&feats,&nnet_in_frame,&dim);
			// use features.

			if(feats == NULL)
			{
				fprintf(stderr,"GetFeats failed.\n");
				return -1;
			}
			forward->FeedForward(feats,nnet_in_frame,dim);
			int max_frames = forward->NumFramesReady();
			decode.AdvanceDecoding(forward,max_frames);
//			decode.FinalizeDecoding();

			if(max_frames - nnet_in_frame > 1)
				fprintf(stderr,"it's a serious bug.%d != %d",max_frames,nnet_in_frame);
			dnnfeat->RemoveFeats(nnet_in_frame);
			break;
		}
		vector<int> best_words_arr;
		vector<int> best_phones_arr;
//		printf("%s	",linefeat);
		if(decode.GetBestPath(best_words_arr, best_phones_arr))
		{
/*			for(unsigned i = 0; i < best_words_arr.size(); ++i)
			{
				if(NULL != wordsymbol)
					printf("%s ",wordsymbol->FindWordStr(best_words_arr[i]));
			}
			printf("\n");
			if(NULL == wordsymbol)
*/			{
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

	}

	fclose(featfp);
	delete dnnfeat;
	delete forward;
	delete nnet;
	return 0;
}

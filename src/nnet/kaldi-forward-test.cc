#include <stdio.h>
#include <sys/time.h>
#include "src/nnet/nnet-nnet.h"
#include "src/nnet/nnet-feature-api.h"
#include "src/nnet/nnet-util.h"

#ifdef NAMESPACE
using namespace datemoon;
#endif

int main(int argc,char *argv[])
{
	const char *usage = "This is a nnet forward.\n\n"
		"Usage:kaldi-forward-test featconf nnetmodel feat.list\n";;
	ConfigParseOptions conf(usage);

	NnetForwardOptions nnetopt;
	nnetopt.Register(&conf);
	conf.Read(argc, argv);

	if(conf.NumArgs() != 4)
	{
		conf.PrintUsage();
		//fprintf(stderr,"%s conf nnetmodel feat\n",argv[0]);
		return -1;
	}
	std::string featconf = conf.GetArg(1);
	std::string am_filename = conf.GetArg(2),
		feat_list = conf.GetArg(3);

	Nnet *nnet = new Nnet;
	nnet->ReadNnet(am_filename.c_str());
	// new source.
	NnetForward *forward = new NnetForward(nnet, &nnetopt);
	int left = 0, right =0 ;
	
	forward->GetLRoffset(left,right);
	DnnFeat *dnnfeat = new DnnFeat;
	dnnfeat->Init(featconf.c_str(),"./");
	dnnfeat->InitParameters(left,right);
#define LEN (1024)
	int nnet_in_frame = 0;
	int dim = 0;
	float *feats = NULL;

	FILE *featfp = fopen(feat_list.c_str(),"r");
	if(NULL == featfp)
	{
		fprintf(stderr,"fopen %s failed.\n", feat_list.c_str());
		return -1;
	}
	float all_wavtime = 0;
	float all_nnettime = 0;
	std::string key,file;
	long offset = 0;
	while(ReadScp(featfp, key, file, offset) == true)
	{
#define DEBUG
#ifdef DEBUG
		int frame = 0;
#endif
		while(1)
		{
			forward->Reset();
			dnnfeat->Reset();
			dnnfeat->GetFeats(file.c_str() ,&feats,&nnet_in_frame,&dim);
			if(feats == NULL)
			{
				fprintf(stderr,"GetFeats failed.\n");
				return -1;
			}
#define TESTTIME
#ifdef TESTTIME
			struct timeval start,end;
			gettimeofday(&start,NULL);
#endif
			forward->FeedForward(feats,nnet_in_frame,dim);
#ifdef TESTTIME
			gettimeofday(&end,NULL);
			float usetime = end.tv_sec - start.tv_sec + (end.tv_usec - start.tv_usec)/1000000.0;
			all_nnettime += usetime;
			all_wavtime += nnet_in_frame/100.0;
			gettimeofday(&start,NULL);
#endif
#ifdef DEBUG
			int outdim = forward->GetOutdim();
			printf("%s [\n",key.c_str());
			for(int i = 0;i<nnet_in_frame;++i)
			{
				if(false == forward->ExamineFrame(frame+i))
				{
					break;
				}
				for(int j=0;j<outdim;++j)
					printf("%7.6f ",forward->LogLikelihood(frame+i,j+1));
				if(false == forward->ExamineFrame(frame+i+1))
					printf("]");
				printf("\n");
			}
			frame +=nnet_in_frame;
#endif
			dnnfeat->RemoveFeats(nnet_in_frame);
			break;
		}
	}
//	printf("nnet rt is %f\n",all_nnettime/all_wavtime);
//	fclose(fp);
	delete dnnfeat;
	delete forward;
	delete nnet;
	return 0;
}

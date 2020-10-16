#include <stdio.h>
#include <iostream>
#include <sys/time.h>
#include "src/nnet/nnet-nnet.h"
#include "src/nnet/nnet-feature-api.h"
#include "src/util/config-parse-options.h"
#include "src/util/io-funcs.h"

using namespace std;
#ifdef NAMESPACE
using namespace datemoon;
#endif

int main(int argc,char *argv[])
{
	struct timeval start_time,end_time;
	float all_nnettime = 0;
	float all_feattime = 0;
	float all_wavtime = 0;
	const char *usage = "This is a nnet forward calculate with pitch \n"
		"pitch-forward-test --feature-conf=fbanks.cfg --conf=conf nnetmodel wav\\\n";
	ConfigParseOptions conf(usage);

	NnetForwardOptions nnet_option;

	string feat_conf("");
	string word_list("");
	conf.Register("feature-conf",&feat_conf,"default is empty");
	conf.Register("word-list",&word_list,"default is empty");
	nnet_option.Register(&conf);
	DnnPitchFeat *dnnfeat = new DnnPitchFeat(&conf);
	conf.Read(argc, argv);
	if(conf.NumArgs() != 3 )
	{
		conf.PrintUsage();
		return -1;
	}
	// read word list
	vector<string> wordlist;
	if(word_list.size() != 0)
	{
		if(!ReadWordList(word_list,wordlist,false))
		{
			std::cerr << "open " << word_list << "failed!" << std::endl;
			return -1;
		}
	}
	Nnet *nnet = new Nnet;
	if(true != nnet->ReadNnet(conf.GetArg(1).c_str()))
	{
		std::cerr << "read " << conf.GetArg(1) << " failed." << std::endl;
		return -1;
	}
	// new source.
	NnetForward *forward = new NnetForward(nnet, &nnet_option);
	int left = 0, right =0 ;
	
	forward->GetLRoffset(left,right);
	dnnfeat->Init(feat_conf.c_str(), "./");
	dnnfeat->InitParameters(left,right);
#define LEN (1600)
	FILE *filep = fopen(conf.GetArg(2).c_str(),"rb");
	if(NULL == filep)
	{
		std::cerr << "fopen " << conf.GetArg(2) << "error." << std::endl;
		return -1;
	}
#define WAVHEADLEN 44
	assert(LEN >= WAVHEADLEN);
	char filename[128];
	memset(filename ,0x00,sizeof(filename));
	while(NULL != fgets(filename,sizeof(filename),filep))
	{
		filename[strlen(filename)-1] = '\0';
		if(filename[0] == '#')
		{
			memset(filename ,0x00,sizeof(filename));
			continue;
		}
		// reset 
		forward->Reset();
		dnnfeat->Reset();
		// get feature
		float *feats = NULL;
		int nnet_in_frame = 0;
		int dim = 0;
		gettimeofday(&start_time,NULL);
		dnnfeat->GetFeats(filename,&feats,&nnet_in_frame,&dim);
		gettimeofday(&end_time,NULL);
		all_feattime += end_time.tv_sec - start_time.tv_sec + (end_time.tv_usec - start_time.tv_usec)/1000000.0;
#ifdef DEBUGFEAT
		for(int r = 0 ; r < nnet_in_frame; ++r)
		{
			for(int c = 0 ; c < dim ; ++c)
				std::cout << feats[(r+left)*dim+c] << " " ;
			std::cout << std::endl;
		}
#endif
		
		int frame = 0;
		int send_frame = 1;
		vector<int> res ;
		bool is_black = false;
		for(int i=0;i<nnet_in_frame;i+=send_frame)
		{
//			int end = 0;
//			if(i == nnet_in_frame-1)
//				end = 1;
			gettimeofday(&start_time,NULL);
			forward->FeedForward(feats, send_frame, dim);
			gettimeofday(&end_time,NULL);
			all_nnettime += end_time.tv_sec - start_time.tv_sec + (end_time.tv_usec - start_time.tv_usec)/1000000.0;
			for(int i = 0;i<send_frame;++i)
			{
				if(false == forward->ExamineFrame(frame))
				{
					break;
					fprintf(stderr,"it's nnet forward error.\n");
					return -1;
				}
#ifdef DEBUGDNNOUT
				int outdim = forward->GetOutdim();
				printf("%d@ ",frame);
				for(int j=0;j<outdim;++j)
				{
					// because defalue j is ilabel so I need add offset 1.
					printf("%7.6f ",forward->LogLikelihood(frame,j+1));
				}
				printf("\n");
#endif
				float maxpdf = 0;
				int maxpdfid = forward->MaxPdf(frame,&maxpdf);
//				std::cout << frame << " " << maxpdfid << " " << maxpdf <<std::endl;

				if(maxpdfid != 0)
				{
					if(res.size() == 0)
						res.push_back(maxpdfid);
					else if(is_black==true || maxpdfid != res[res.size()-1])
						res.push_back(maxpdfid);
				}
				if(maxpdfid == 0)
					is_black = true;
				else
					is_black = false;
				frame++;
			}
//		frame +=nnet_in_frame;
			dnnfeat->RemoveFeats(send_frame);
		}

		all_wavtime += frame;
		if(word_list.size() != 0)
		{
			std::cout << string(filename) << " " ;
			for(unsigned i=0;i<res.size();++i)
			{
				if(res[i] >= (int)wordlist.size())
				{
					std::cerr << "it's a seriousness error!This word beyond word list range." << std::endl;
					return -1;
				}
				std::cout << wordlist[res[i]] << " ";
			}
			std::cout << std::endl;
		}
		else
		{
			for(unsigned i=0;i<res.size();++i)
				std::cout << res[i] << " ";
			std::cout << std::endl;
		}
		memset(filename ,0x00,sizeof(filename));
	}
	std::cout << "feat rt is " << all_feattime * 100 /all_wavtime/3 << std::endl;
	std::cout << "nnet rt is " << all_nnettime * 100 /all_wavtime/3 << std::endl;
	fclose(filep);
	delete dnnfeat;
	delete forward;
	delete nnet;
	return 0;
}

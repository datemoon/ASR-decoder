#include <stdio.h>
#include <vector>
#include <string.h>
#include <sys/time.h>
#include <utility>
#include <assert.h>
#include <string>
#include "src/decoder/optimize-fst.h"
#include "src/decoder/optimize-ctc-faster-decoder.h"
#include "src/nnet/nnet-nnet.h"
#include "src/nnet/nnet-feature-api.h"
#include "src/decoder/wordid-to-wordstr.h"
#include "src/align/phone-to-word.h"

using namespace std;

#ifdef NAMESPACE
using namespace datemoon;
#endif

int main(int argc,char *argv[])
{
	const char *usage = "This is a ctc decode.\n"
		"wfst graph no block flag.\n\n"
		"Usage:optimize-ctc-faster-decoder-kaldifeature-test featconf fst nnetmodel feat.list\n";
	ConfigParseOptions conf(usage);

	CtcFasterDecoderOptions decodeopt(16.0,5000,20,0.5,2.0);
	NnetForwardOptions nnetopt;
	decodeopt.Register(&conf, true);
	nnetopt.Register(&conf);



	std::string wordlist, phonedict, prondict;
	conf.Register("wordlist", &wordlist, "default is no wordlist");
	conf.Register("phonedict", &phonedict , "default is no phonedict");
	conf.Register("prondict", &prondict, "default is no prondict");

	std::string hmmfile ;
	conf.Register("hmmfile", &hmmfile, "default is nohmmfile");
	
	conf.Read(argc, argv);
	if(conf.NumArgs() != 5)
	{
		conf.PrintUsage();
//		fprintf(stderr,"%s featconf fst nnetmodel feat.list\n",argv[0]);
		return -1;
	}

	std::string featconf = conf.GetArg(1);
	std::string fst_in_filename = conf.GetArg(2),
		am_filename = conf.GetArg(3),
		feat_list = conf.GetArg(4);

	PhoneToWord pronounce;
	if(wordlist.size() != 0)
	{
		pronounce.ReadText(wordlist.c_str(), phonedict.c_str());
		pronounce.ProcessPronDict(prondict.c_str());
	}
	Nnet *nnet = new Nnet;
	nnet->ReadNnet(am_filename.c_str());
	if(hmmfile.size() != 0)
	{
		nnet->ReadTransModel(hmmfile.c_str());
	}
	// new source.
	//NnetForward *forward = new NnetForward(nnet, 2, true, true, 20.0, 1.0);
	NnetForward *forward = new NnetForward(nnet, &nnetopt);
	
	int left = 0, right =0 ;
	
	forward->GetLRoffset(left,right);
	DnnFeat *dnnfeat = new DnnFeat;
	dnnfeat->Init(featconf.c_str(), "./");
	dnnfeat->InitParameters(left,right);

	Fst fst;
	fst.ReadFst(fst_in_filename.c_str());


	CtcFasterDecoder decode(&fst, &decodeopt);

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
	float all_decodetime  = 0;
	float all_nnettime = 0;
	char linefeat[LEN];
	WordSymbol * wordsymbol = new WordSymbol;
	if((wordlist.size() != 0) && 0 != wordsymbol->ReadText(wordlist.c_str()))
	{
		fprintf(stderr,"read word list %s failed.\n",wordlist.c_str());
		return -1;;
	}
	if(wordlist.size() == 0)
	{// init fail.
		delete wordsymbol;
		wordsymbol = NULL;
	}
	while(NULL != fgets(linefeat,1024,featfp))
	{
		linefeat[strlen(linefeat)-1] = '\0';
		if(linefeat[0] == '#')
			continue;
//#define DEBUGPRINT
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
			gettimeofday(&start,NULL);
#endif
			int max_frames = forward->NumFramesReady();
			decode.AdvanceDecoding(forward,max_frames);

#ifdef TESTTIME
			gettimeofday(&end,NULL);
			usetime = end.tv_sec - start.tv_sec + (end.tv_usec - start.tv_usec)/1000000.0;
//			printf("time is %f,rt is %f\n",usetime,usetime*100/nnet_in_frame);
		   	all_wavtime += nnet_in_frame/100.0;
			all_decodetime += usetime;
#endif
			if(max_frames - nnet_in_frame/3 > 1)
				fprintf(stderr,"it's a serious bug.%d != %d",max_frames,nnet_in_frame);
#ifdef DEBUGPRINT
			int outdim = forward->GetOutdim();
			for(int i = 0;i<nnet_in_frame/3;++i)
			{
				printf("%d@ ",frame+i);
				if(false == forward->ExamineFrame(frame+i))
				{
					fprintf(stderr,"it's nnet forward error.\n");
					return -1;
				}
				for(int j=0;j<outdim;++j)
					printf("%7.6f ",forward->DnnOut(frame+i,j));
				printf("\n");
			}
			frame +=nnet_in_frame;
#endif
			dnnfeat->RemoveFeats(nnet_in_frame);
			break;
		}
		vector<int> best_words_arr;
		vector<int> best_phones_arr;
		printf("%s	",linefeat);
		float best_tot_score=0 ,best_lm_score=0;
		if(decode.GetBestPath(best_words_arr, best_phones_arr,
				   	best_tot_score, best_lm_score))
		{
			if(NULL == wordsymbol)
			{
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
			else
			{
				for(unsigned i = 0; i < best_words_arr.size(); ++i)
				{
					printf("%s ",wordsymbol->FindWordStr(best_words_arr[i]));
				}
				//printf("%f %f",best_tot_score,best_lm_score);
				printf("\n");
				//for(unsigned i = 0; i < best_phones_arr.size(); ++i)
					//printf("(%d) %d \n",i,best_phones_arr[i]);
				//	printf(" %d",best_phones_arr[i]);
				//printf("\n");
			}
		}
		else
			printf("no result.\n");

		//decode.PrintBestPath();
		// the last do align
		if(prondict.size() != 0)
		{
			// there if it's context phone, I need exchange ilableid to phoneid.
			for(size_t i=0;i<best_phones_arr.size();++i)
			{
				best_phones_arr[i] = forward->GetPhoneId(best_phones_arr[i]);
			}
			vector<int> phones;
			vector<int> phones_location;
			bool amongbk = true;
			for(size_t i=0;i<best_phones_arr.size();++i)
			{
				// there I need think about some phone continue appear 
				// and among no have blank
				if(best_phones_arr[i] != 1)
				{
					if(amongbk == true ||
							(amongbk == false && best_phones_arr[i] != phones[phones.size()-1]))
					{
						// here,I process silence
						if(168 == best_phones_arr[i])
							continue;
						phones.push_back(best_phones_arr[i]);
						phones_location.push_back(i*3);
					}
					else if(amongbk == false && best_phones_arr[i] == phones[phones.size()-1])
					{
						phones_location[phones.size()-1] = i*3;
					}
					amongbk= false;
				}
				else
					amongbk = true;
			}
			int phones_len = phones.size();
			int offset = 0;
			int wordpair = 0;
			vector<int> word_align;
			for(size_t i=0;i<best_words_arr.size();++i)
			{
				wordpair = pronounce.SearchWord(phones.data()+offset,
						phones_len-offset,best_words_arr[i]);
				assert(wordpair != -1);
				word_align.push_back(wordpair);
				offset+=wordpair;
			}
			assert(word_align.size() == best_words_arr.size());
			int phone_offset = 0;
			for(unsigned i = 0; i < best_words_arr.size(); ++i)
			{
				int start = 0;
				if(phone_offset != 0)
					start = phones_location[phone_offset-1]+1;
				else
					start = phones_location[phone_offset];
				int end = phones_location[phone_offset+word_align[i]-1];
				if(phone_offset == 1)
				{
					start +=1;
					phone_offset--;
				}
				phone_offset+=word_align[i];
				if(NULL != wordsymbol)
					printf("%s\t%5d\t%5d\n",wordsymbol->FindWordStr(best_words_arr[i]),
							start, end);
			//				st_ed[i]._start,st_ed[i]._end);
			}
			printf("\n");
		}
	}// end feat.list
	printf("nnet rt is %f,decode rt is %f\n",
			all_nnettime/all_wavtime, all_decodetime/all_wavtime);
	fclose(featfp);
	if(NULL != wordsymbol)
		delete wordsymbol;
	delete dnnfeat;
	delete forward;
	delete nnet;
	return 0;
}

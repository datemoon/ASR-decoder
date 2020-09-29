
#include <string>
//kaldi
#include "online2/online-nnet3-decoding.h"
#include "online2/online-nnet2-feature-pipeline.h"
#include "online2/onlinebin-util.h"
#include "online2/online-timing.h"
#include "online2/online-endpoint.h"
#include "feat/wave-reader.h"
#include "nnet3/nnet-utils.h"
//datemoon
#include "src/kaldi-nnet3/kaldi-online-nnet3-my-decoder.h"
#include "src/newfst/lattice-functions.h"
#include "src/newfst/lattice-to-nbest.h"
#include "src/newfst/lattice-determinize.h"
#include "src/util/log-message.h"

int main(int argc, char *argv[])
{
#ifdef NAMESPACE
	using namespace datemoon;
#endif
//	using namespace kaldi;
//	using namespace fst;
	
	const char *usage = "This is a test kaldi-online-nnet3-my-decoder code.\n"
		"Usage: kaldi-online-nnet3-my-decoder-test [options] <nnet3-in> "
		"<fst-in> <word-symbol-table> <wavfile>\n";
	kaldi::ParseOptions po(usage);

	// init online featuer and nnet config
	OnlineDecoderConf online_conf;
	online_conf.Register(&po);

	po.Read(argc, argv);
	if (po.NumArgs() != 4)
	{
		po.PrintUsage();
		return 1;
	}

	std::string nnet3_rxfilename = po.GetArg(1),
		fst_in_filename = po.GetArg(2),
		word_syms_filename = po.GetArg(3),
		wavfilelist = po.GetArg(4);

	// online decoder info
	OnlineDecoderInfo online_info(online_conf, nnet3_rxfilename,
			fst_in_filename, word_syms_filename);

	// online decoder
	OnlineClgLatticeFastDecoder online_clg_decoder(online_info);

#define LEN 4096
#define FILE_LEN 128
	FILE *flfp = fopen(wavfilelist.c_str(), "r");

	if(flfp == NULL)
	{
		LOG_ERR << "Open wav: " << wavfilelist <<" failed!!!";
		return -1;
	}
	char wavfile[FILE_LEN];
	memset(wavfile,0x00,sizeof(wavfile));
	while(fgets(wavfile, FILE_LEN, flfp) != NULL)
	{
		// delete \n
		if(wavfile[0] == '#')
			continue;
		wavfile[strlen(wavfile)-1] = '\0';
		FILE *fp = fopen(wavfile , "r");
		if(fp == NULL)
		{
			LOG_WARN << "Open wav: " <<wavfile <<" failed!!!";
			continue;
		}
		memset(wavfile,0x00,sizeof(wavfile));
		int npackage=0;

		// init decoder
		online_clg_decoder.InitDecoding(0, true);

		while(true)
		{
			npackage++;
			char wavdata[LEN];
			memset(wavdata,0x00,sizeof(wavdata));
			size_t data_len = fread((void*)wavdata, 1, LEN, fp);
			int data_type = 2;
			VLOG_COM(2) << npackage << " package -> read wavdata length: " << data_len;
			if(data_len > 0)
			{
				online_clg_decoder.ProcessData(wavdata, data_len, 0, data_type);
			}
			else
			{
				online_clg_decoder.ProcessData(wavdata, data_len, 2, data_type);
				VLOG_COM(2) << "FinalizeDecoding. OK" ;
				if (online_clg_decoder.NumFramesDecoded() > 0)
				{
					Lattice olat;
					online_clg_decoder.GetLattice(&olat, true);

					VLOG_COM(2) << "GetLattice. OK";
					vector<Lattice> nbest_paths;
					Lattice best_path;

					LatticeShortestPath(&olat, &best_path);
					VLOG_COM(2) << "LatticeShortestPath. OK" ;
	
					{// nbest result
						Lattice detfst;
						bool debug_ptr = false;
						DeterminizeLatticeOptions opts;
						assert(LatticeCheckFormat(&olat) && "ofst format it's error");
						DeterminizeLatticeWrapper(&olat,&detfst,opts,&debug_ptr);
						assert(LatticeCheckFormat(&detfst) && "detfst format it's error");
						VLOG_COM(2) << "DeterminizeLatticeWrapper. OK" ;
						Lattice nbest_lat;
						NShortestPath(detfst, &nbest_lat, 10);
						ConvertNbestToVector(nbest_lat, &nbest_paths);
						for(unsigned i = 0; i < nbest_paths.size(); ++i)
						{
							vector<int> nbest_words_arr;
							vector<int> nbest_phones_arr;
							float nbest_tot_score=0 ,nbest_lm_score=0;
	
							if(LatticeToVector(nbest_paths[i], nbest_words_arr, nbest_phones_arr, nbest_tot_score, nbest_lm_score))
							{
								for(unsigned j = 0; j < nbest_words_arr.size(); ++j)
								{
									printf("%s ",online_info._wordsymbol.FindWordStr(nbest_words_arr[j]));
								}
								printf("%f %f\n",nbest_tot_score,nbest_lm_score);
							}
						} // nbest print
					} // nbest result
				} // if (decoder.NumFramesDecoded() > 0)
				break;
			} // end decoder
	
	
		} // decoder while(true)

		fclose(fp);
	} // while wav list

	return 0;
}


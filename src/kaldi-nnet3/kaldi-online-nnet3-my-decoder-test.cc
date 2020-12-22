
#include <string>

#include "src/decoder/mem-optimize-clg-lattice-faster-online-decoder.h"
#include "src/decoder/wordid-to-wordstr.h"
#include "src/newfst/lattice-functions.h"
#include "src/newfst/lattice-to-nbest.h"
#include "src/newfst/rmepsilon.h"
#include "src/newfst/lattice-determinize.h"

//kaldi
#include "online2/online-nnet3-decoding.h"
#include "online2/online-nnet2-feature-pipeline.h"
#include "online2/onlinebin-util.h"
#include "online2/online-timing.h"
#include "online2/online-endpoint.h"
#include "feat/wave-reader.h"
#include "nnet3/nnet-utils.h"

int main(int argc, char *argv[])
{
#ifdef NAMESPACE
	using namespace datemoon;
#endif
//	using namespace kaldi;
//	using namespace fst;
	
	const char *usage = "This is a test kaldi-online-nnet3-my-decoder code.\n"
		"Usage: kaldi-online-nnet3-my-decoder-test [options] <nnet3-in> "
		        "<fst-in> <hmm-fst-in> <word-symbol-table> <wavfile>\n";
	kaldi::ParseOptions po(usage);
	std::string wordlist,phonedict,prondict;
	//po.Register("wordlist", &wordlist, "default is no wordlist");
	po.Register("phonedict", &phonedict , "default is no phonedict");
    po.Register("prondict", &prondict, "default is no prondict");
	std::string decoder_conf;
	po.Register("config-decoder", &decoder_conf, "decoder conf file");

	kaldi::nnet3::NnetSimpleLoopedComputationOptions decodable_opts;
	kaldi::OnlineNnet2FeaturePipelineConfig feature_opts;

	feature_opts.Register(&po);
	decodable_opts.Register(&po);

	po.Read(argc, argv);
	if (po.NumArgs() != 5)
	{
		po.PrintUsage();
		return 1;
	}
	LatticeFasterDecoderConfig decoder_opts;
	ReadConfigFromFile(decoder_conf, &decoder_opts);
	//decoder_opts._max_active = 5000;
	//decoder_opts._beam = 10;
	decoder_opts.Print();

	std::string nnet3_rxfilename = po.GetArg(1),
		fst_in_filename = po.GetArg(2),
		hmm_in_filename = po.GetArg(3),
		word_syms_filename = po.GetArg(4),
		wav_file = po.GetArg(5);
	wordlist = word_syms_filename;
	WordSymbol * wordsymbol = new WordSymbol;
	if((wordlist.size() != 0) && 0 != wordsymbol->ReadText(wordlist.c_str()))
	{
		std::cerr << "read word list " << wordlist.c_str() 
			<< " failed." << std::endl;
		return -1;
	}
	if(wordlist.size() == 0)
	{// init fail.
		delete wordsymbol;
		wordsymbol = NULL;
	}
	// feature
	kaldi::OnlineNnet2FeaturePipelineInfo feature_info(feature_opts);
	kaldi::OnlineNnet2FeaturePipeline feature_pipeline(feature_info);
	BaseFloat samp_freq = feature_info.GetSamplingFrequency();

	// load clg fst
	ClgFst clgfst;
	if(clgfst.Init(fst_in_filename.c_str(), hmm_in_filename.c_str())!= true)
	{
		std::cerr << "load fst error." << std::endl;
		return -1;
	}

	MemOptimizeClgLatticeFasterOnlineDecoder online_decoder(&clgfst, decoder_opts);

	//BaseFloat frame_shift = feature_info.FrameShiftInSeconds();
	//int32 frame_subsampling = decodable_opts.frame_subsampling_factor;

	// am load
	kaldi::nnet3::AmNnetSimple am_nnet;
	kaldi::TransitionModel trans_model;
	{
		bool binary;
		kaldi::Input ki(nnet3_rxfilename, &binary);
		trans_model.Read(ki.Stream(), binary);
		am_nnet.Read(ki.Stream(), binary);
		kaldi::nnet3::SetBatchnormTestMode(true, &(am_nnet.GetNnet()));
		kaldi::nnet3::SetDropoutTestMode(true, &(am_nnet.GetNnet()));
		kaldi::nnet3::CollapseModel(kaldi::nnet3::CollapseModelConfig(),
				&(am_nnet.GetNnet()));
	}

	kaldi::nnet3::DecodableNnetSimpleLoopedInfo decodable_info(decodable_opts,
			&am_nnet);

	kaldi::nnet3::DecodableAmNnetLoopedOnline decodable(trans_model,
			decodable_info, feature_pipeline.InputFeature(),
		   	feature_pipeline.IvectorFeature());

	int frame_offset = 0;
	online_decoder.InitDecoding();
	decodable.SetFrameOffset(frame_offset);
#define LEN 4096
	FILE *fp = fopen(wav_file.c_str(), "r");
	int npackage=0;
	while(true)
	{
		npackage++;
		char wavdata[LEN];
		memset(wavdata,0x00,sizeof(wavdata));
		size_t data_len = fread((void*)wavdata, 1, LEN, fp);
		int data_type = 2;
		kaldi::Vector<BaseFloat> wave_part;
		std::cout << npackage << " package -> read wavdata length: " << data_len << std::endl;
		if(data_len > 0)
		{
			wave_part.Resize(data_len/data_type);
			for(int i=0;i<data_len/data_type;++i)
			{
				wave_part(i) = ((short*)(wavdata))[i];
			}
			feature_pipeline.AcceptWaveform(samp_freq, wave_part);

			online_decoder.AdvanceDecoding(&decodable);
		}
		else
		{
			std::cout << "data ok." << std::endl;
			feature_pipeline.InputFinished();
			online_decoder.AdvanceDecoding(&decodable);
			online_decoder.FinalizeDecoding();
			std::cout << "FinalizeDecoding." << std::endl;
			if (online_decoder.NumFramesDecoded() > 0)
			{
				Lattice olat;
				online_decoder.GetRawLattice(&olat, true);

				std::cout << "GetRawLattice." << std::endl;
				vector<Lattice> nbest_paths;
				Lattice best_path;

				LatticeShortestPath(&olat, &best_path);
				std::cout << "LatticeShortestPath." << std::endl;

				{// nbest result
					Lattice detfst;
					bool debug_ptr = false;
					DeterminizeLatticeOptions opts;
					std::cout << "DeterminizeLatticeWrapper start." << std::endl;
					assert(LatticeCheckFormat(&olat) && "ofst format it's error");
					DeterminizeLatticeWrapper(&olat,&detfst,opts,&debug_ptr);
					assert(LatticeCheckFormat(&detfst) && "detfst format it's error");
					std::cout << "DeterminizeLatticeWrapper end." << std::endl;
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
								printf("%s ",wordsymbol->FindWordStr(nbest_words_arr[j]));
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
	if(NULL != wordsymbol)
		delete wordsymbol;

	return 0;
}


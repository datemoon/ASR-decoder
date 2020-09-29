
#include <string>
//kaldi
#include "base/kaldi-common.h"
#include "util/common-utils.h"
#include "tree/context-dep.h"
#include "hmm/transition-model.h"
#include "fstext/fstext-lib.h"
#include "decoder/decoder-wrappers.h"
#include "decoder/decodable-matrix.h"
#include "base/timer.h"

//datemoon
#include "src/decoder/lattice-faster-decoder.h"
#include "src/decoder/mem-optimize-hclg-lattice-faster-online-decoder.h"
#include "src/decoder/online-decoder-mempool-base.h"
#include "src/decoder/wordid-to-wordstr.h"
#include "src/newfst/lattice-functions.h"
#include "src/newfst/lattice-to-nbest.h"
#include "src/newfst/lattice-determinize.h"
#include "src/util/log-message.h"
#include "src/newlm/compose-arpalm.h"

int main(int argc, char *argv[])
{
#ifdef NAMESPACE
	using namespace datemoon;
#endif
//	using namespace kaldi;
//	using namespace fst;
	
	const char *usage = "This is a test kaldi-online-nnet3-my-decoder code.\n"
		"Usage: kaldi-online-nnet3-my-decoder-test [options] <nnet3-in> "
		"<fst-in> <word-symbol-table> <feat-in>\n";
	kaldi::ParseOptions po(usage);

	BaseFloat acoustic_scale = 0.1;
	std::string config_decoder = "";
	std::string lm_file = "";
	int32 nbest = 1;
	po.Register("acoustic-scale", &acoustic_scale, "Scaling factor for acoustic likelihoods");
	po.Register("config-decoder", &config_decoder, "decoder config file (default NULL)");
	po.Register("nbest", &nbest, "nbest result (default 1)");

//	po.Register("lm-file", &lm_file, "decoder config file (default NULL)");
	
	po.Read(argc, argv);
	if (po.NumArgs() != 4)
	{
		po.PrintUsage();
		return 1;
	}

	std::string nnet3_rxfilename = po.GetArg(1),
		fst_in_filename = po.GetArg(2),
		word_syms_filename = po.GetArg(3),
		feature_rspecifier = po.GetArg(4);

//	ArpaLm lm;
//	if(lm.Read(lm_file.c_str()) != true)
//	{
//		LOG_ERR << "load lm file!!!" << lm_file;
//	}
//	lm.Rescale(-1.0);
//	ComposeArpaLm clm(&lm);

	WordSymbol wordsymbol;
	if(wordsymbol.ReadText(word_syms_filename.c_str()) != 0)
	{
		LOG_WARN << "read wordlist " << word_syms_filename << " failed!!!";
	}
	kaldi::TransitionModel trans_model;
	kaldi::ReadKaldiObject(nnet3_rxfilename, &trans_model);

	Fst fst;
	if(fst.ReadFst(fst_in_filename.c_str())!= true)
	{
		LOG_WARN << "load fst error.";
		return -1;
	}
	LatticeFasterDecoderConfig decodeopt;
	if(config_decoder != "")
		ReadConfigFromFile(config_decoder, &decodeopt);

	//LatticeFasterDecoder decode(&fst, decodeopt);
	OnlineLatticeDecoderMempool decode(&fst, decodeopt);

	//MemOptimizeHclgLatticeFasterOnlineDecoder decode(&fst, decodeopt);
	kaldi::Timer timer;
	timer.Reset();

	kaldi::int64 frame_count = 0;
	int num_success = 0, num_fail = 0;
	kaldi::SequentialBaseFloatMatrixReader loglike_reader(feature_rspecifier);
	for(; !loglike_reader.Done(); loglike_reader.Next())
	{
		decode.InitDecoding();
		std::string utt = loglike_reader.Key();
		kaldi::Matrix<BaseFloat> loglikes (loglike_reader.Value());
		loglike_reader.FreeCurrent();
		if (loglikes.NumRows() == 0)
		{
			KALDI_WARN << "Zero-length utterance: " << utt;
			num_fail++;
			continue;
		}
		kaldi::DecodableMatrixScaledMapped decodable(trans_model, loglikes, acoustic_scale);
		decode.AdvanceDecoding(&decodable);
		decode.FinalizeDecoding();
		frame_count += loglikes.NumRows();

		LOG_COM << utt << " frames is : " << loglikes.NumRows();
		
		if(nbest==1)
		{
			std::vector<int> nbest_words_arr;
			std::vector<int> nbest_phones_arr;
			float nbest_tot_score=0 ,nbest_lm_score=0;
			Lattice best_path;
			decode.GetBestPath(&best_path);

			if(LatticeToVector(best_path, nbest_words_arr, nbest_phones_arr, nbest_tot_score, nbest_lm_score) != true)
			{
				LOG_WARN << "best paths is null!!!";
			}
			std::string onebest_string = "";
			for(unsigned i = 0; i < nbest_words_arr.size(); ++i)
				onebest_string += wordsymbol.FindWordStr(nbest_words_arr[i]) + std::string(" ");
			LOG_COM << onebest_string << " tot_score: " << nbest_tot_score << " lm_score: " << nbest_lm_score;
		}
		else
		{
			Lattice olat,lat1,detfst;
			decode.GetRawLattice(&lat1, true);
			bool debug_ptr = false;

			DeterminizeLatticeOptions opts;
			LOG_ASSERT(datemoon::LatticeCheckFormat(&lat1) && "ofst format it's error");
			DeterminizeLatticeWrapper(&lat1,&detfst,opts,&debug_ptr);
			LOG_ASSERT(datemoon::LatticeCheckFormat(&detfst) && "detfst format it's error");
//			std::string lat_file = "lat.1";
//			detfst.Write(lat_file);

//			detfst.Print();
//			ComposeLattice<FsaStateId>( &detfst,
//					static_cast<LatticeComposeItf<FsaStateId>* >(&clm),
//					&olat);
			Lattice nbest_lat;
			vector<Lattice> nbest_paths;
			NShortestPath(olat, &nbest_lat, 10);
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
						printf("%s ",wordsymbol.FindWordStr(nbest_words_arr[j]));
					}
					printf("%f %f\n",nbest_tot_score,nbest_lm_score);
				}
			}

		}

		
		/*
		Lattice best_path;
		decode.GetBestPath(&best_path);
		{
			std::string onebest_string = "";
			std::vector<int> nbest_words_arr;
			std::vector<int> nbest_phones_arr;
			float nbest_tot_score=0 ,nbest_lm_score=0;
			if(LatticeToVector(best_path, nbest_words_arr, nbest_phones_arr, nbest_tot_score, nbest_lm_score) != true)
			{
				LOG_WARN << "best paths is null!!!";
			}
			for(unsigned i = 0; i < nbest_words_arr.size(); ++i)
				onebest_string += wordsymbol.FindWordStr(nbest_words_arr[i]) + std::string(" ");
			LOG_COM << onebest_string << " tot_score: " << nbest_tot_score << " lm_score: " << nbest_lm_score;
		}*/
		num_success++;
	}
	 double elapsed = timer.Elapsed();
	 KALDI_LOG << "Time taken "<< elapsed
		 << "s: real-time factor assuming 100 frames/sec is "
		 << (elapsed*100.0/frame_count);
	 KALDI_LOG << "Done " << num_success << " utterances, failed for "
		 << num_fail;

	 if (num_success != 0) 
		 return 0;
	 else 
		 return 1;
}


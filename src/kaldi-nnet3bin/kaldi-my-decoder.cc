
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
#include "src/decoder/mem-optimize-clg-lattice-faster-online-decoder.h"
#include "src/decoder/wordid-to-wordstr.h"
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
		"<fst-in> <hmm-fst-in> <word-symbol-table> <feat-in>\n";
	kaldi::ParseOptions po(usage);

	BaseFloat acoustic_scale = 0.1;
	std::string config_decoder = "";
	po.Register("acoustic-scale", &acoustic_scale, "Scaling factor for acoustic likelihoods");
	po.Register("config-decoder", &config_decoder, "decoder config file (default NULL)");

	po.Read(argc, argv);
	if (po.NumArgs() != 5)
	{
		po.PrintUsage();
		return 1;
	}

	std::string nnet3_rxfilename = po.GetArg(1),
		fst_in_filename = po.GetArg(2),
		hmm_in_filename = po.GetArg(3),
		word_syms_filename = po.GetArg(4),
		feature_rspecifier = po.GetArg(5);

	WordSymbol wordsymbol;
	if(wordsymbol.ReadText(word_syms_filename.c_str()) != 0)
	{
		LOG_WARN << "read wordlist " << word_syms_filename << " failed!!!";
	}
	kaldi::TransitionModel trans_model;
	kaldi::ReadKaldiObject(nnet3_rxfilename, &trans_model);

	ClgFst fst;
	if(fst.Init(fst_in_filename.c_str(), hmm_in_filename.c_str())!= true)
	{
		std::cerr << "load fst error." << std::endl;
		return -1;
	}
	LatticeFasterDecoderConfig decodeopt;
	if(config_decoder != "")
		ReadConfigFromFile(config_decoder, &decodeopt);

	MemOptimizeClgLatticeFasterOnlineDecoder decode(&fst, decodeopt);

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
		}
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



#include <string>
#include <vector>
#include "src/v1-asr/kaldi-v1-asr-online.h"


int main(int argc, char *argv[])
{
	try
	{
		using namespace kaldi;
		const char *usage = "Test kaldi nnet3 decoder with vad code.\n"
			"./kaldi-v1-asr-online-test final.mdl fst words.txt scp:wav.scp";
		ParseOptions po(usage);
		std::string vad_nnet_file="";

		po.Register("vad-nnet-file", &vad_nnet_file,
				"vad model file name");
		V1AsrOpts v1_asr_opts;
		v1_asr_opts.Register(&po);

		po.Read(argc, argv);
		if(po.NumArgs() != 4)
		{
			po.PrintUsage();
			exit(1);
		}

		std::string nnet3_filename = po.GetArg(1);
		std::string fst_filename = po.GetArg(2),
			words_filename = po.GetArg(3),
			wav_rspecifier = po.GetArg(4);

		V1AsrSource v1_asr_source(v1_asr_opts, 
				nnet3_filename, fst_filename, words_filename, vad_nnet_file);

		V1AsrWorker v1_asr_worker(v1_asr_opts, 
				v1_asr_source);

		SequentialTableReader<WaveHolder> reader(wav_rspecifier);

		int32 num_utts = 0;
		for(; !reader.Done(); reader.Next())
		{
			num_utts++;
			std::string utt = reader.Key();
			const WaveData &wave_data = reader.Value();
			SubVector<BaseFloat> waveform(wave_data.Data(), 0);
			v1_asr_worker.Init(true, 0);
			int32 nbest = 0;
			bool eos = true;
			std::string result = v1_asr_worker.Process(waveform, nbest, eos);
			KALDI_LOG << utt << " : " << result ;
		}
		return 0;
	}
	catch(const std::exception &e)
	{
		std::cerr << e.what();
		return -1;
	}
}

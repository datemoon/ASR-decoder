
#include "feat/wave-reader.h"
#include "util/common-utils.h"
#include "src/v1-asr/online-vad.h"

int main(int argc, char* argv[])
{
	try
	{
		using namespace kaldi;
		const char *usage = "Test kaldi nnet3 vad code.\n";
		int32 samp_freq = 16000;
		int32 channel = -1;
		bool apply_exp = true, use_priors = false;
		ParseOptions po(usage);

		OnlineNnet2FeaturePipelineConfig feature_opts;
		VadNnetSimpleLoopedComputationOptions vad_nnet_opts;
		VadJudgeOptions vad_judge_opt;
		feature_opts.Register(&po);
		vad_nnet_opts.Register(&po);
		vad_judge_opt.Register(&po);


		po.Register("samp-freq", &samp_freq, "sample frequecy.");
		po.Register("channel", &channel, "Channel to extract (-1 -> expect mono, "
				"0 -> left, 1 -> right)");
		po.Register("use-priors", &use_priors, "If true, subtract the logs of the "
				"priors stored with the model (in this case, "
				"a .mdl file is expected as input).");
		po.Register("apply-exp", &apply_exp, "If true, apply exp function to "
				"output");

		po.Read(argc, argv);

		if(po.NumArgs() != 2)
		{
			po.PrintUsage();
			exit(1);
		}

		std::string nnet3_filename = po.GetArg(1);
		std::string wav_rspecifier = po.GetArg(2);
		nnet3::AmNnetSimple am_nnet;
		nnet3::Nnet raw_nnet;
		TransitionModel trans_model;
		if(use_priors)
		{
			bool binary;
			Input ki(nnet3_filename, &binary);
			trans_model.Read(ki.Stream(), binary);
			am_nnet.Read(ki.Stream(), binary);
			nnet3::SetBatchnormTestMode(true, &(am_nnet.GetNnet()));
			nnet3::SetDropoutTestMode(true, &(am_nnet.GetNnet()));
			nnet3::CollapseModel(nnet3::CollapseModelConfig(),
					&(am_nnet.GetNnet()));
		}
		else
		{
			ReadKaldiObject(nnet3_filename, &raw_nnet);
		}// load am ok.
		nnet3::Nnet &nnet = (use_priors ? am_nnet.GetNnet() : raw_nnet);


		VadNnetInfo vad_nnet_info(vad_nnet_opts, &nnet);
		// feature
		OnlineNnet2FeaturePipelineInfo feature_info(feature_opts);

		// int32 outdim = am_nnet.GetNnet().OutputDim("output");

		SequentialTableReader<WaveHolder> reader(wav_rspecifier);

		BaseFloat min_duration = 0.0;
		int32 num_utts = 0;// num_success = 0;
		for(; !reader.Done(); reader.Next())
		{
			num_utts++;
			std::string utt = reader.Key();
			const WaveData &wave_data = reader.Value();
			if (wave_data.Duration() < min_duration)
			{
				KALDI_WARN << "File: " << utt << " is too short ("
					<< wave_data.Duration() << " sec): producing no output.";
				continue;
			}
			int32 num_chan = wave_data.Data().NumRows(), this_chan = channel;

			{  // This block works out the channel (0=left, 1=right...)
				KALDI_ASSERT(num_chan > 0);  // This should have been caught in
				// reading code if no channels.
				if (channel == -1) 
				{
					this_chan = 0;
					if (num_chan != 1)
						KALDI_WARN << "Channel not specified but you have data with "
							<< num_chan  << " channels; defaulting to zero";
				} 
				else 
				{
					if (this_chan >= num_chan) 
					{
						KALDI_WARN << "File with id " << utt << " has "
							<< num_chan << " channels but you specified channel "
							<< channel << ", producing no output.";
						continue;
					}
				}
			}

			SubVector<BaseFloat> waveform(wave_data.Data(), this_chan);
			OnlineNnet2FeaturePipeline feature_pipeline(feature_info);
			int32 frames_offset = 0;
			int32 nnet_frame_offset = 0;
			VadNnet3 vad_nnet3(vad_nnet_info, feature_pipeline.InputFeature(),
					vad_judge_opt,
				   	frames_offset,
					nnet_frame_offset,
					apply_exp);
			feature_pipeline.AcceptWaveform(samp_freq, waveform);
			feature_pipeline.InputFinished();

			vad_nnet3.ComputerNnet(1);
		}
		return 0;
	}
	catch(const std::exception &e)
	{
		std::cerr << e.what();
		return -1;
	}
}

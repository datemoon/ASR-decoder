#include <iostream>
#include <vector>
#include <stdio.h>
#include <cstring>
#include <cassert>
#include "src/util/config-parse-options.h"
#include "src/pitch/pitch-functions.h"
#include "src/pitch/kaldi-type.h"
#include "src/pitch/kaldi-matrix.h"

using namespace std;

int main(int argc,char *argv[])
{
	const char *usage = "This is a pitch test code\n"
		"compute-and-process-kaldi-pitch-feats --simulate-first-pass-online=true\\\n"
		"--frames-per-chunk=10 --sample-frequency=8000 wav outputfeat\n";
	ConfigParseOptions conf(usage);

	PitchExtractionOptions pitch_opts;
	ProcessPitchOptions process_opts;
	
	pitch_opts.Register(&conf);
	process_opts.Register(&conf);

	conf.Read(argc, argv);

	if(conf.NumArgs() != 3)
	{
		conf.PrintUsage();
		exit(-1);
	}

	std::string wav = conf.GetArg(1),
		feat = conf.GetArg(2);

	FILE *wavfp = fopen(wav.c_str(),"rb");
	if(NULL == wavfp)
	{
		std::cerr << "fopen " << wav << "failed" << std::endl;
		return -1;
	}
	// first
	// now init online pitch
	OnlinePitchFeature pitch_extractor(pitch_opts);
	OnlineProcessPitch post_process(process_opts, &pitch_extractor);

	if (pitch_opts.simulate_first_pass_online) 
	{
		assert(pitch_opts.frames_per_chunk > 0 &&
				"--simulate-first-pass-online option does not make sense "
				"unless you specify --frames-per-chunk");
	}
	int32 cur_frame = 0;
	int32 cur_rows = 100;
	Matrix<BaseFloat> feats(cur_rows, post_process.Dim());
#define LENGTH (160)
	char wav_data[LENGTH];
	int len=0;
	fread(wav_data,sizeof(char),44,wavfp);
	bool is_end = false;
	while((len = fread(wav_data,sizeof(char),LENGTH,wavfp)))
	{
		if(len != LENGTH)
			is_end = true;
		Vector<BaseFloat> wavform;
		int wav_len = len/sizeof(short);
		wavform.Resize(wav_len, kUndefined);
		for(int i =0 ; i < wav_len ; ++i)
			wavform(i) = ((short*)wav_data)[i];
		//memcpy(wavform.Data(), wav_data, len);
//		Matrix<BaseFloat> features;
		try 
		{
//			ComputeAndProcessKaldiPitch(pitch_opts, process_opts,
//					wavform, &features);

	/*		int32 num_samp =0;
			if (samp_per_chunk > 0)
				num_samp = std::min(samp_per_chunk, wave.Dim() - cur_offset);
			else
				num_samp = wave.Dim();
	*/		pitch_extractor.AcceptWaveform(pitch_opts.samp_freq, wavform);
			if(is_end == true)
				pitch_extractor.InputFinished();
			// Get each frame as soon as it is ready.
			for (; cur_frame < post_process.NumFramesReady(); cur_frame++)
			{
				if (cur_frame >= cur_rows)
				{
					cur_rows *= 2;
					feats.Resize(cur_rows, post_process.Dim(), kCopyData);
				}
				SubVector<BaseFloat> row(feats, cur_frame);
				post_process.GetFrame(cur_frame, &row);
				row.Print();
			}
//			features = feats.RowRange(0, cur_frame);
			//feats.Print();
		}
		catch (...)
		{
			std::cerr << "Failed to compute pitch for utterance " << wav << std::endl;
			exit(-1);
		}

		// end process
//		features.Print();
	}
	fclose(wavfp);
	return 0;
}

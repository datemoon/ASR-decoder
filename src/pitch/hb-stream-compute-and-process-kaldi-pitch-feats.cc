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
	StreamPitch stream_pitch(pitch_opts, process_opts);
#define LENGTH (160*5)
	char wav_data[LENGTH];
	int len=0;
	fread(wav_data,sizeof(char),44,wavfp);
	bool is_end = false;
	Matrix<BaseFloat> features;
	Vector<BaseFloat> wavform;
	int wav_len = 0;
	while((len = fread(wav_data,sizeof(char),LENGTH,wavfp)))
	{
		wav_len = len/sizeof(short);
		wavform.Resize(wav_len, kUndefined);
		for(int i =0 ; i < wav_len ; ++i)
			wavform(i) = ((short*)wav_data)[i];
		if(len != LENGTH)
		{
			is_end = true;
			break;
		}
		try 
		{
#ifdef API
			stream_pitch.ProcessWave(pitch_opts,wavform,features, is_end);
			features.Print();
#else
			float *pitch_feat = NULL;
			int pitch_dim = 0;
			int pitch_rows = 0;
			stream_pitch.ProcessWave(pitch_opts,(short*)wav_data,len/sizeof(short), &pitch_feat, pitch_dim, pitch_rows, is_end);
			for(int r =0 ; r < pitch_rows; ++r)
			{
				for(int c = 0 ; c<pitch_dim;++c)
				{
					std::cout << pitch_feat[r*pitch_dim+c] << " ";
				}
				std::cout << std::endl;
			}
#endif
		}
		catch (...)
		{
			std::cerr << "Failed to compute pitch for utterance " << wav << std::endl;
			exit(-1);
		}

		// end process
	}
	stream_pitch.ProcessWave(pitch_opts,wavform,features, is_end);
	features.Print();
	if(len != 0)
	fclose(wavfp);
	return 0;
}


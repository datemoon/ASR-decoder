#include <iostream>
#include <vector>
#include <stdio.h>
#include <cstring>
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
#define LENGTH 102400
	char wav_data[LENGTH];
	int len=0;
	fread(wav_data,sizeof(char),44,wavfp);
	while((len = fread(wav_data,sizeof(char),LENGTH,wavfp)))
	{
		if(len == 0)
			break;
		Vector<BaseFloat> wavform;
		int wav_len = len/sizeof(short);
		wavform.Resize(wav_len, kUndefined);
		for(int i =0 ; i < wav_len ; ++i)
			wavform(i) = ((short*)wav_data)[i];
		//memcpy(wavform.Data(), wav_data, len);
		Matrix<BaseFloat> features;
		try 
		{
			ComputeAndProcessKaldiPitch(pitch_opts, process_opts,
					wavform, &features);
		}
		catch (...)
		{
			std::cerr << "Failed to compute pitch for utterance " << wav << std::endl;
			exit(-1);
		}
		features.Print();
	}
	fclose(wavfp);
	return 0;
}

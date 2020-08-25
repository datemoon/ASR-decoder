
#include "src/newfst/lattice-functions.h"
#include "src/newfst/lattice-to-nbest.h"
#include "src/newfst/rmepsilon.h"
#include "src/newfst/lattice-determinize.h"

#include "src/kaldi-nnet3/kaldi-online-nnet3-my-decoder.h"


int OnlineClgLatticeFastDecoder::ProcessData(char *data, int data_len, int eos, int data_type)
{
	kaldi::Vector<kaldi::BaseFloat> wave_part;
	assert(data_len%data_type == 0 && "data_len \% datatype is not 0");
	
	float samp_freq = _online_info._feature_info.GetSamplingFrequency();
	if(data_len > 0)
	{
		wave_part.Resize(data_len/data_type);
		for(int i=0;i<data_len/data_type;++i)
		{
			if(data_type == 4)
			{
				wave_part(i) = ((float*)(data))[i];
			}
			else if(data_type == 2)
			{
				wave_part(i) = ((short*)(data))[i];
			}
		}
	}
	_feature_pipeline->AcceptWaveform(samp_freq, wave_part);
	if(eos == 0)
	{ // send data
		_decoder.AdvanceDecoding(_decodable);
	}
	else if(eos == 1)
	{ // end point check end and continue send data.
		_decoder.AdvanceDecoding(_decodable);
		_decoder.FinalizeDecoding();
	}
	else if(eos == 2)
	{ // send data end 
		_feature_pipeline->InputFinished();
		_decoder.AdvanceDecoding(_decodable);
		_decoder.FinalizeDecoding();
	}
	return 0;
}

void OnlineClgLatticeFastDecoder::GetLattice(datemoon::Lattice *olat, 
		bool end_of_utterance)
{
	_decoder.GetRawLattice(olat, end_of_utterance);
}

void OnlineClgLatticeFastDecoder::GetBestPath(datemoon::Lattice *best_path,
		bool end_of_utterance) const
{
	_decoder.GetBestPath(best_path, end_of_utterance);
}

void OnlineClgLatticeFastDecoder::GetNbest(vector<datemoon::Lattice> &nbest_paths, int n, bool end_of_utterance)
{
	datemoon::Lattice olat;
	_decoder.GetRawLattice(&olat, end_of_utterance);

	datemoon::Lattice detfst;
	datemoon::DeterminizeLatticeOptions opts;
	bool debug_ptr = false;
	assert(datemoon::LatticeCheckFormat(&olat) && "ofst format it's error");
	datemoon::DeterminizeLatticeWrapper(&olat,&detfst,opts,&debug_ptr);
	assert(datemoon::LatticeCheckFormat(&detfst) && "detfst format it's error");
	datemoon::Lattice nbest_lat;
	datemoon::NShortestPath(detfst, &nbest_lat, n);
	datemoon::ConvertNbestToVector(nbest_lat, &nbest_paths);
}





#include "src/kaldi-nnet3/kaldi-online-nnet3-my-decoder.h"


int OnlineClgLatticeFastDecoder::ProcessData(char *data, int data_len, int eos, int data_type)
{
	kaldi::Vector<BaseFloat> wave_part;
	assert(date_len%data_type == 0 && "data_len \% datatype is not 0");
	
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
		bool end_of_utterance) const
{
	_decoder.GetRawLattice(olat, end_of_utterance);
}

void OnlineClgLatticeFastDecoder::GetBestPath(bool end_of_utterance,
		datemoon::Lattice *best_path) const
{
	vector<int> best_words_arr;
   	vector<int> best_phones_arr;
    float best_tot_score=0, best_lm_score =0;
	_decoder.GetBestPath(*best_path, best_words_arr, best_phones_arr, best_tot_score, best_lm_score);
}

void OnlineClgLatticeFastDecoder::GetNbest(vector<datemoon::Lattice> &nbest_paths, int n, bool end_of_utterance=true)
{
	Lattice olat;
	_decoder.GetRawLattice(olat, end_of_utterance);

	Lattice detfst;
	DeterminizeLatticeOptions opts;
	bool debug_ptr = false;
	assert(LatticeCheckFormat(&olat) && "ofst format it's error");
	DeterminizeLatticeWrapper(&olat,&detfst,opts,&debug_ptr);
	assert(LatticeCheckFormat(&detfst) && "detfst format it's error");
	Lattice nbest_lat;
	NShortestPath(detfst, &nbest_lat, n);
	ConvertNbestToVector(nbest_lat, &nbest_paths);

}


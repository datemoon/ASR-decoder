
#include "src/kaldi-nnet3/kaldi-online-nnet3-my-decoder.h"


void OnlineDecoder::AdvanceDecoding()
{
	_decoder.AdvanceDecoding(_decodable);
}

void OnlineDecoder::FinalizeDecoding()
{
	_decoder.FinalizeDecoding();
}

kaldi::int32 OnlineDecoder::NumFramesDecoded() const
{
	return _decoder.NumFramesDecoded();
}

void OnlineDecoder::GetLattice(bool end_of_utterance,
		datemoon::Lattice *olat) const
{
	_decoder.GetRawLattice(olat, end_of_utterance);
}

void OnlineDecoder::GetBestPath(bool end_of_utterance,
		datemoon::Lattice *best_path) const
{
	vector<int> best_words_arr;
   	vector<int> best_phones_arr;
    float best_tot_score=0, best_lm_score =0;
	_decoder.GetBestPath(*best_path, best_words_arr, best_phones_arr, best_tot_score, best_lm_score);
}



#include "src/newfst/lattice-functions.h"
#include "src/newfst/lattice-to-nbest.h"
#include "src/newfst/rmepsilon.h"
#include "src/newfst/lattice-determinize.h"

#include "src/kaldi-nnet3/kaldi-online-nnet3-my-decoder.h"


int OnlineClgLatticeFastDecoder::ProcessData(char *data, int data_len, int eos, int data_type)
{
	assert(data_len%data_type == 0 && "data_len \% datatype is not 0");
	
	if(data_len > 0)
	{
		kaldi::Vector<kaldi::BaseFloat> wave_part;
		float samp_freq = _online_info._feature_info.GetSamplingFrequency();
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
		_feature_pipeline->AcceptWaveform(samp_freq, wave_part);
	}
	if(eos == 0)
	{ // send data
		_decoder->AdvanceDecoding(_decodable);
	}
	else if(eos == 1)
	{ // end point check end and continue send data.
		_decoder->AdvanceDecoding(_decodable);
		_decoder->FinalizeDecoding();
	}
	else if(eos == 2)
	{ // send data end 
		_feature_pipeline->InputFinished();
		_decoder->AdvanceDecoding(_decodable);
		_decoder->FinalizeDecoding();
	}
	return 0;
}

void OnlineClgLatticeFastDecoder::GetLattice(datemoon::Lattice *olat, 
		bool end_of_utterance)
{
	if(_online_info._online_conf._use_second == true)
	{
		datemoon::ComposeArpaLm *lm1 = new datemoon::ComposeArpaLm(&(_online_info._lm1));
		datemoon::ComposeArpaLm *lm2 = new datemoon::ComposeArpaLm(&(_online_info._lm2));
		datemoon::Lattice lat, detfst, lat1;
		_decoder->GetRawLattice(&lat, end_of_utterance);
		datemoon::DeterminizeLatticeOptions opts;
		bool debug_ptr = false;
		LOG_ASSERT(datemoon::LatticeCheckFormat(&lat) && "ofst format it's error");
		datemoon::DeterminizeLatticeWrapper(&lat,&detfst,opts,&debug_ptr);
		LOG_ASSERT(datemoon::LatticeCheckFormat(&detfst) && "detfst format it's error");
		
		//detfst.Print();
		datemoon::ComposeLattice<datemoon::FsaStateId>( &detfst,
				static_cast<datemoon::LatticeComposeItf<datemoon::FsaStateId>* >(lm1),
				&lat1);
		//lat1.Print();
		datemoon::ComposeLattice<datemoon::FsaStateId>( &lat1,
				static_cast<datemoon::LatticeComposeItf<datemoon::FsaStateId>* >(lm2),
				olat);
		//olat->Print();
		VLOG_COM(2) << "ComposeLattice ok";
		delete lm1;
		delete lm2;
	}
	else
	{
		datemoon::Lattice lat;
		_decoder->GetRawLattice(&lat, end_of_utterance);
		datemoon::DeterminizeLatticeOptions opts;
		bool debug_ptr = false;
		LOG_ASSERT(datemoon::LatticeCheckFormat(&lat) && "ofst format it's error");
		datemoon::DeterminizeLatticeWrapper(&lat,olat,opts,&debug_ptr);
		LOG_ASSERT(datemoon::LatticeCheckFormat(olat) && "detfst format it's error");
		VLOG_COM(2) << "don't use second ok";
	}
}

void OnlineClgLatticeFastDecoder::GetBestPath(datemoon::Lattice *best_path,
		bool end_of_utterance)
{
	_decoder->GetBestPath(best_path, end_of_utterance);
}

void OnlineClgLatticeFastDecoder::GetNbest(std::vector<datemoon::Lattice> &nbest_paths, int n, bool end_of_utterance)
{
	datemoon::Lattice olat;
	GetLattice(&olat, end_of_utterance);

	datemoon::Lattice nbest_lat;
	datemoon::NShortestPath(olat, &nbest_lat, n);
	datemoon::ConvertNbestToVector(nbest_lat, &nbest_paths);
}

void OnlineClgLatticeFastDecoder::OnebestLatticeToString(datemoon::Lattice &onebest_lattice, std::string &onebest_string)
{
	onebest_string = "";
	std::vector<int> nbest_words_arr;
    std::vector<int> nbest_phones_arr;
    float nbest_tot_score=0 ,nbest_lm_score=0;
	if(datemoon::LatticeToVector(onebest_lattice, nbest_words_arr, nbest_phones_arr, nbest_tot_score, nbest_lm_score) != true)
	{
		LOG_WARN << "best paths is null!!!";
		return;
	}
	for(unsigned i = 0; i < nbest_words_arr.size(); ++i)
		onebest_string += _online_info._wordsymbol.FindWordStr(nbest_words_arr[i]) + std::string(" ");
	LOG_COM << onebest_string << " tot_score: " << nbest_tot_score << " lm_score: " << nbest_lm_score;
}
void OnlineClgLatticeFastDecoder::GetBestPathTxt(std::string &best_result, bool end_of_utterance)
{
	best_result = "";
	if(_online_info._online_conf._use_second == true)
	{
		std::vector<std::string> nbest_result;
		GetNbestTxt(nbest_result, 1, end_of_utterance);
		best_result = nbest_result[0];
	}
	else
	{
		datemoon::Lattice best_path;
		_decoder->GetBestPath(&best_path, end_of_utterance);
		OnebestLatticeToString(best_path, best_result);
	}
}

void OnlineClgLatticeFastDecoder::GetNbestTxt(std::vector<std::string> &nbest_result, int n, bool end_of_utterance)
{
	nbest_result.clear();
	std::vector<datemoon::Lattice> nbest_paths;
	GetNbest(nbest_paths, n, end_of_utterance);
	for(size_t i = 0 ; i < nbest_paths.size(); ++i)
	{
		std::string one_txt;
		OnebestLatticeToString(nbest_paths[i], one_txt);
		nbest_result.push_back(one_txt);
	}
}

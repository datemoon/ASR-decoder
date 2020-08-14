#ifndef __ASR_SOURCE_H__
#define __ASR_SOURCE_H__


#include "feat/wave-reader.h"
#include "online2/online-nnet3-decoding.h"
#include "online2/online-nnet2-feature-pipeline.h"
#include "online2/onlinebin-util.h"
#include "online2/online-timing.h"
#include "online2/online-endpoint.h"
#include "fstext/fstext-lib.h"
#include "lat/lattice-functions.h"
#include "util/kaldi-thread.h"
#include "nnet3/nnet-utils.h"

// ASR relation option.
// include feature, nnet, decoder, endpoint.
using namespace kaldi;
using namespace fst;
//typedef kaldi::int32 int32;
//typedef kaldi::int64 int64;

class ASROpts
{
public:
	// feature option
	OnlineNnet2FeaturePipelineConfig _feature_opts;
	// nnet option
	nnet3::NnetSimpleLoopedComputationOptions _decodable_opts;
	// decoder option
	LatticeFasterDecoderConfig _decoder_opts;
	// end point option
	OnlineEndpointConfig _endpoint_opts;

	BaseFloat _samp_freq;
	BaseFloat _chunk_length_secs;
	BaseFloat _output_period;
	bool _produce_time;

	ASROpts():
		_samp_freq(16000.0), _chunk_length_secs(0.15),
		_output_period(1.0),_produce_time(false) { }
/*	ASROpts(const ASROpts &asr_opt):
		_samp_freq(16000.0), _chunk_length_secs(0.15),
		_output_period(1.0),_produce_time(false),
		_feature_opts(asr_opt._feature_opts),
		_decodable_opts(asr_opt._decodable_opts),
		_decoder_opts(asr_opt._decoder_opts),
		_endpoint_opts(asr_opt._endpoint_opts) { }
*/
	~ASROpts() { }

	void Register(ParseOptions *conf)
	{
		_feature_opts.Register(conf);
		_decodable_opts.Register(conf);
		_decoder_opts.Register(conf);
		_endpoint_opts.Register(conf);
		conf->Register("samp-freq", &_samp_freq,
				"Sampling frequency of the input signal (coded as 16-bit slinear).");
		conf->Register("chunk-length", &_chunk_length_secs,
				"Length of chunk size in seconds, that we process.");
		conf->Register("output-period", &_output_period,
				"How often in seconds, do we check for changes in output.");
		conf->Register("produce-time", &_produce_time,
				"Prepend begin/end times between endpoints (e.g. '5.46 6.81 <text_output>', in seconds)");

	}
};

class ASRSource
{
public:
	friend class ASRWorker;

	ASRSource(std::string nnet_filename,
		   std::string fst_filename,
		   std::string words_filename):
		_nnet_filename(nnet_filename), _fst_filename(fst_filename),
		_words_filename(words_filename),_decode_fst(NULL),_word_syms(NULL)
	{ 
		// am load
		bool binary;
		Input ki(_nnet_filename, &binary);
		_trans_model.Read(ki.Stream(), binary);
		_am_nnet.Read(ki.Stream(), binary);
		SetBatchnormTestMode(true, &(_am_nnet.GetNnet()));
		SetDropoutTestMode(true, &(_am_nnet.GetNnet()));
		nnet3::CollapseModel(nnet3::CollapseModelConfig(), 
				&(_am_nnet.GetNnet()));

		// load fst
		_decode_fst = ReadFstKaldiGeneric(_fst_filename);
		
		// load words list
		_word_syms = fst::SymbolTable::ReadText(words_filename);

	}


	~ASRSource() 
	{
		if(_word_syms != NULL)
		{
	 		delete _word_syms;
	 		_word_syms = NULL;
		}
		if(_decode_fst != NULL)
		{
	 		delete _decode_fst;
	 		_decode_fst = NULL;
		}
	}

private:
	std::string _nnet_filename;
	std::string _fst_filename;
	std::string _words_filename;

	fst::Fst<fst::StdArc> *_decode_fst;
	fst::SymbolTable *_word_syms;
	nnet3::AmNnetSimple _am_nnet;
	TransitionModel _trans_model;
};
//typedef ASTType::int32 int32;
//typedef ASTType::BaseFloat BaseFloat;
//using namespace kaldi;
std::string LatticeToString(const Lattice &lat, const fst::SymbolTable &word_syms) 
{
	kaldi::LatticeWeight weight;
	std::vector<int32> alignment;
	std::vector<int32> words;
	GetLinearSymbolSequence(lat, &alignment, &words, &weight);

	std::ostringstream msg;
	for (size_t i = 0; i < words.size(); i++) 
	{
		std::string s = word_syms.Find(words[i]);
		if (s.empty()) 
		{
			KALDI_LOG << "Word-id " << words[i] << " not in symbol table.";
			msg << "<#" << std::to_string(i) << "> ";
		} else
			msg << s << " ";
	}
	return msg.str();
}
std::string GetTimeString(int32 t_beg, int32 t_end, BaseFloat time_unit) 
{
	char buffer[100];
	double t_beg2 = t_beg * time_unit;
	double t_end2 = t_end * time_unit;
	snprintf(buffer, 100, "%.2f %.2f", t_beg2, t_end2);
	return std::string(buffer);
}
int32 GetLatticeTimeSpan(const Lattice& lat) 
{
	std::vector<int32> times;
	LatticeStateTimes(lat, &times);
	return times.back();
}
std::string LatticeToString(const CompactLattice &clat, const fst::SymbolTable &word_syms) 
{
	if (clat.NumStates() == 0) {
		KALDI_LOG << "Empty lattice.";
		return "";
	}
	CompactLattice best_path_clat;
	CompactLatticeShortestPath(clat, &best_path_clat);

	Lattice best_path_lat;
	ConvertLattice(best_path_clat, &best_path_lat);
	return LatticeToString(best_path_lat, word_syms);
}


class ASRWorker
{
public:
	//using namespace kaldi;
	//using namespace fst;

	//typedef ASTType::int32 int32;
	//typedef ASTType::BaseFloat BaseFloat;
	ASRWorker(ASROpts *asr_opts, ASRSource *asr_source):
		_asr_opts(asr_opts), _asr_source(asr_source),_feature_pipeline(NULL) {}
	~ASRWorker() { }
	void Init(size_t *chunk_len, int32 frame_offset=0)
	{
		_produce_time = _asr_opts->_produce_time;
		_samp_freq = _asr_opts->_samp_freq;
		
		_samp_count = 0;// this is used for output refresh rate
		_frame_offset = 0;
		_chunk_len = static_cast<size_t>(_asr_opts->_chunk_length_secs * _samp_freq);
		*chunk_len = _chunk_len;
		_check_period = static_cast<int32>(_samp_freq * _asr_opts->_output_period);
		_check_count = _check_period;

		_decodable_info = new nnet3::DecodableNnetSimpleLoopedInfo(_asr_opts->_decodable_opts,
				(nnet3::AmNnetSimple*)&(_asr_source->_am_nnet));

		_feature_info = new OnlineNnet2FeaturePipelineInfo(_asr_opts->_feature_opts);

		
		_feature_pipeline = new OnlineNnet2FeaturePipeline(*_feature_info);
		_decoder = new SingleUtteranceNnet3Decoder(_asr_opts->_decoder_opts, 
				_asr_source->_trans_model,
				*_decodable_info,
				*_asr_source->_decode_fst, _feature_pipeline);
		_decoder->InitDecoding(frame_offset);

	}
	
	void Reset(bool eos)
	{
		if (eos)
		{
			_frame_offset = 0;
			_samp_count = 0;
			_check_count = _check_period;
		}
		_decoder->InitDecoding(_frame_offset);
	}

	int32 ProcessData(char *data, int32 data_len, bool eos = false)
	{
		int32 data_type=4;
		if(eos)
		{
			_feature_pipeline->InputFinished();
			_decoder->AdvanceDecoding();
			_decoder->FinalizeDecoding();
			_frame_offset += _decoder->NumFramesDecoded();
			if (_decoder->NumFramesDecoded() > 0)
			{
				std::string msg = GetProduceTime(true);
			}
			return 0; //语音结束,eos is true.
		}
		Vector<BaseFloat> wave_part;
		wave_part.Resize(data_len/data_type);
		// convert input data
		for(int i=0;i<data_len/data_type;++i)
		{
			wave_part(i) = ((BaseFloat*)(data))[i];
		}
		_feature_pipeline->AcceptWaveform(_samp_freq, wave_part);
		_samp_count += _chunk_len;

		_decoder->AdvanceDecoding();
		if (_samp_count > _check_count)
		{
			if (_decoder->NumFramesDecoded() > 0)
			{
				std::string msg = GetProduceTime(false);
			}
			_check_count += _check_period;
		}

		if (_decoder->EndpointDetected(_asr_opts->_endpoint_opts))
		{
			_decoder->FinalizeDecoding();
			_frame_offset += _decoder->NumFramesDecoded();
			std::string msg = GetProduceTime(true);
			return 1; // 语音中间截断
		}

		return 0; // eos is false.
	}
	std::string GetProduceTime(bool end)
	{
		BaseFloat frame_shift = _feature_info->FrameShiftInSeconds();
		int32 frame_subsampling = _asr_opts->_decodable_opts.frame_subsampling_factor;
		if(end == false)
		{
			Lattice lat;
			_decoder->GetBestPath(false, &lat);
			TopSort(&lat); // for LatticeStateTimes()
			std::string msg = LatticeToString(lat, *_asr_source->_word_syms);
			// get time-span after previous endpoint,
			if(_produce_time)
			{
				int32 t_beg = _frame_offset;
				int32 t_end = _frame_offset + GetLatticeTimeSpan(lat);
				msg = GetTimeString(t_beg, t_end, frame_shift * frame_subsampling) + " " + msg;
			}
			KALDI_LOG << "Temporary transcript: " << msg;
			return msg;
		}
		else
		{
			CompactLattice lat;
			_decoder->GetLattice(true, &lat);
			std::string msg = LatticeToString(lat, *_asr_source->_word_syms);
			// get time-span after previous endpoint,
			if(_produce_time)
			{
				int32 t_beg = _frame_offset - _decoder->NumFramesDecoded();
				int32 t_end = _frame_offset;
				msg = GetTimeString(t_beg, t_end, frame_shift * frame_subsampling) + " " + msg;
			}
			KALDI_LOG << "Temporary transcript: " << msg;
			return msg;
		}
	}

private:
	const ASROpts *_asr_opts;
	const ASRSource *_asr_source;

	// this object contains precomputed stuff that is used by all decodable
	// objects.  It takes a pointer to am_nnet because if it has iVectors it has
	// to modify the nnet to accept iVectors at intervals.
	nnet3::DecodableNnetSimpleLoopedInfo *_decodable_info;

	OnlineNnet2FeaturePipelineInfo *_feature_info;
	OnlineNnet2FeaturePipeline *_feature_pipeline;
	SingleUtteranceNnet3Decoder *_decoder;
	// get time-span from previous endpoint to end of audio,
	bool _produce_time;
	BaseFloat _samp_freq;
	int32 _samp_count;// this is used for output refresh rate
	size_t _chunk_len;
	int32 _check_period;
	int32 _check_count;
	int32 _frame_offset;

};

#endif

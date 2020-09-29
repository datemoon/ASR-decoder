
#include "online2/online-nnet3-decoding.h"
#include "online2/online-nnet2-feature-pipeline.h"
#include "online2/onlinebin-util.h"
#include "online2/online-timing.h"
#include "online2/online-endpoint.h"
#include "feat/wave-reader.h"
#include "nnet3/nnet-utils.h"

#include "src/decoder/lattice-faster-decoder.h"
#include "src/decoder/mem-optimize-hclg-lattice-faster-online-decoder.h"
#include "src/decoder/online-decoder-base.h"
#include "src/decoder/online-decoder-mempool-base.h"
#include "src/decoder/online-clg-decoder-mempool-base.h"
#include "src/decoder/online-decoder-mempool-base-biglm.h"

#include "src/newlm/compose-arpalm.h"

#include "src/decoder/mem-optimize-clg-lattice-faster-online-decoder.h"
#include "src/decoder/wordid-to-wordstr.h"

struct OnlineDecoderConf
{
public:
	kaldi::nnet3::NnetSimpleLoopedComputationOptions _decodable_opts;
	kaldi::OnlineNnet2FeaturePipelineConfig _feature_opts;
	std::string _config_decoder;
	//std::string _wordlist;
	std::string _phonedict;
	std::string _prondict;
	// you must be set fst type
	std::string _graph_type;
	// if you want use clg graph , you must be set _hmmfst_filen
	std::string _hmmfst_file;
	// if you want use biglm you must be set lm1_file and lm2_file
	std::string _lm1_file;
	std::string _lm2_file;
	datemoon::BaseFloat _lm1_scale;
	datemoon::BaseFloat _lm2_scale;

	bool _use_second;
	OnlineDecoderConf():
		_config_decoder(""),
		_phonedict(""),_prondict(""),
		_graph_type("mem-hclg"),_hmmfst_file(""),
		_lm1_file(""),_lm2_file(""),
		_lm1_scale(-1.0), _lm2_scale(1.0), _use_second(false) { }
		//_wordlist(""),
	void Register(kaldi::OptionsItf *conf)
	{
		_decodable_opts.Register(conf);
		_feature_opts.Register(conf);
		conf->Register("config-decoder", &_config_decoder,
				"decoder config file (default NULL)");
		//conf->Register("wordlist", &_wordlist,
		//		"wordlist file (default NULL)");
		conf->Register("phonedict", &_phonedict,
				"phonedict file (default NULL)");
		conf->Register("prondict", &_prondict,
				"prondict file (default NULL)");
		conf->Register("graph-type", &_graph_type,
				"graph type mem-hclg,hclg,biglm-hclg or clg (default mem-hclg)");
		conf->Register("hmmfst-file", &_hmmfst_file,
				"if graph type is clg, hmmfst-file isn't NULL (default NULL)");
		conf->Register("lm1-file", &_lm1_file,
				"if lm1-file and lm2-file isn't NULL, it will use biglm decoder (default NULL)");
		conf->Register("lm2-file", &_lm2_file,
				"if lm1-file and lm2-file isn't NULL, it will use biglm decoder (default NULL)");
		conf->Register("lm1-scale", &_lm1_scale,
				"lm1-file scale (default -1.0)");
		conf->Register("lm2-scale", &_lm2_scale,
				"lm2-file scale (default 1.0)");
		conf->Register("use-second", &_use_second,
				"use second biglm decoder. (default false)");
	}
};

struct OnlineDecoderInfo
{
public:
	const OnlineDecoderConf &_online_conf;
	// feature config read 
	kaldi::OnlineNnet2FeaturePipelineInfo _feature_info;
	// decoder config read
	datemoon::LatticeFasterDecoderConfig _decoder_opts;
	// words list read
	datemoon::WordSymbol _wordsymbol;
	
	kaldi::nnet3::AmNnetSimple _am_nnet;
    kaldi::TransitionModel _trans_model;
	// beacuse _graphfst can be clg or hclg, so ther _graphfst use void * 
	// and consturt decoder convert true type.
	void *_graphfst;
	datemoon::ArpaLm _lm1;
	datemoon::ArpaLm _lm2;

	const std::string &_nnet3_filename;
	const std::string &_fst_in_filename;
	const std::string &_wordlist;
	~OnlineDecoderInfo() 
	{ 
		if(_online_conf._graph_type == "clg")
		{
			datemoon::ClgFst *tmpfst = static_cast<datemoon::ClgFst *>(_graphfst);
			delete tmpfst;
		}
		else if(_online_conf._graph_type == "hclg" || 
				_online_conf._graph_type == "mem-hclg" ||
				_online_conf._graph_type == "biglm-hclg")
		{
			datemoon::Fst *tmpfst = static_cast<datemoon::Fst *>(_graphfst);
			delete tmpfst;
		}
		_graphfst = NULL;
	}

	OnlineDecoderInfo(const OnlineDecoderConf &online_conf,
			const std::string &nnet3_filename,
			const std::string &fst_in_filename,
			const std::string &wordlist):
		_online_conf(online_conf),
		_feature_info(online_conf._feature_opts),
		_nnet3_filename(nnet3_filename),
		_fst_in_filename(fst_in_filename),
		_wordlist(wordlist)
	{
		// read decoder config
		if(online_conf._config_decoder != "")
		{
			ReadConfigFromFile(online_conf._config_decoder, &_decoder_opts);
		}
		_decoder_opts.Print();
		// word list read
		if(_wordlist != "")
		{
			if(0 != _wordsymbol.ReadText(_wordlist.c_str()))
			{
				LOG_WARN << "read wordlist " << _wordlist << " failed!!!";
			}
		}
		// am load
		{
			bool binary;
			kaldi::Input ki(_nnet3_filename, &binary);
			_trans_model.Read(ki.Stream(), binary);
			_am_nnet.Read(ki.Stream(), binary);
			kaldi::nnet3::SetBatchnormTestMode(true, &(_am_nnet.GetNnet()));
			kaldi::nnet3::SetDropoutTestMode(true, &(_am_nnet.GetNnet()));
			kaldi::nnet3::CollapseModel(kaldi::nnet3::CollapseModelConfig(),
					&(_am_nnet.GetNnet()));
		} // load am ok
		if(_online_conf._graph_type == "clg")
		{
			datemoon::ClgFst *tmpfst = new datemoon::ClgFst();
			LOG_ASSERT(tmpfst->Init(_fst_in_filename.c_str(), _online_conf._hmmfst_file.c_str()));
			_graphfst = static_cast<void *>(tmpfst);
		}
		else if(_online_conf._graph_type == "hclg" || 
				_online_conf._graph_type == "mem-hclg" ||
				_online_conf._graph_type == "biglm-hclg")
		{
			datemoon::Fst *tmpfst = new datemoon::Fst();
			LOG_ASSERT(tmpfst->ReadFst(_fst_in_filename.c_str()));
			_graphfst = static_cast<void *>(tmpfst);
		}
		if(_online_conf._use_second == true ||(_online_conf._lm1_file != "" && _online_conf._lm2_file != ""))
		{
			if(_lm1.Read(_online_conf._lm1_file.c_str()) != true)
			{
				LOG_ERR << "load arpalm1 " << _online_conf._lm1_file << " failed!!!";
			}
			_lm1.Rescale(_online_conf._lm1_scale);
			if(_lm2.Read(_online_conf._lm2_file.c_str()) != true)
			{
				LOG_ERR << "load arpalm2 " << _online_conf._lm2_file << " failed!!!";
			}
			_lm2.Rescale(_online_conf._lm2_scale);
			LOG_COM << "Use second decoder and load " << _online_conf._lm1_file << " and " <<
				_online_conf._lm2_file << " ok";
		}
	} // construct ok

};

class OnlineClgLatticeFastDecoder
{
public:

public:
	OnlineClgLatticeFastDecoder(OnlineDecoderInfo &online_info):
		_online_info(online_info),
		_decodable_info(online_info._online_conf._decodable_opts, &online_info._am_nnet),
		_decodable(NULL),_feature_pipeline(NULL)
	{
	//datemoon::MemOptimizeClgLatticeFasterOnlineDecoder _decoder;
	//datemoon::OnlineClgLatticeDecoderMempool _decoder;
	//datemoon::LatticeFasterDecoder _decoder;
	//datemoon::MemOptimizeHclgLatticeFasterOnlineDecoder _decoder;
	//datemoon::OnlineLatticeDecoder _decoder;
	//datemoon::OnlineLatticeDecoderMempool _decoder;
	//datemoon::OnlineLatticeDecoderMempoolBiglm _decoder;
		if(online_info._online_conf._graph_type == "clg")
		{
			datemoon::OnlineClgLatticeDecoderMempool *tmpdecoder = 
				new datemoon::OnlineClgLatticeDecoderMempool(
						static_cast<datemoon::ClgFst*>(online_info._graphfst),
					   _online_info._decoder_opts);
			_decoder = static_cast<datemoon::DecoderItf *>(tmpdecoder);
		}
		else if(online_info._online_conf._graph_type == "hclg")
		{
			datemoon::OnlineLatticeDecoder *tmpdecoder =
				new datemoon::OnlineLatticeDecoder(
						static_cast<datemoon::Fst*>(online_info._graphfst),
						_online_info._decoder_opts);
			_decoder = static_cast<datemoon::DecoderItf *>(tmpdecoder);
		}
		else if(online_info._online_conf._graph_type == "mem-hclg")
		{
			datemoon::OnlineLatticeDecoderMempool *tmpdecoder = 
				new datemoon::OnlineLatticeDecoderMempool(
						static_cast<datemoon::Fst*>(online_info._graphfst),
						_online_info._decoder_opts);
			_decoder = static_cast<datemoon::DecoderItf *>(tmpdecoder);
		}
		else if(online_info._online_conf._graph_type == "biglm-hclg")
		{
			datemoon::OnlineLatticeDecoderMempoolBiglm *tmpdecoder =
				new datemoon::OnlineLatticeDecoderMempoolBiglm(
						static_cast<datemoon::Fst*>(online_info._graphfst),
						_online_info._decoder_opts,
						&online_info._lm1,
						&online_info._lm2);
			_decoder = static_cast<datemoon::DecoderItf *>(tmpdecoder);
		}
	   InitDecoding(0, true);
	}

	~OnlineClgLatticeFastDecoder()
	{
		if (_feature_pipeline != NULL)
			delete _feature_pipeline;
		_feature_pipeline = NULL;
		if(_decodable != NULL)
			delete _decodable;
		_decodable = NULL;
		if(_decoder != NULL)
			delete _decoder;
		_decoder = NULL;
	}

	// send_end if send end will init _decodable and _feature_pipeline
	void InitDecoding(int frame_offset=0, bool send_end = false)
	{
		if(send_end == true)
		{
			// new _feature_pipeline
			if (_feature_pipeline != NULL)
				delete _feature_pipeline;
			_feature_pipeline = new kaldi::OnlineNnet2FeaturePipeline(_online_info._feature_info);
			// new _decodable
			if(_decodable != NULL)
				delete _decodable;
			_decodable = new kaldi::nnet3::DecodableAmNnetLoopedOnline(
					_online_info._trans_model,
				   	_decodable_info,
					_feature_pipeline->InputFeature(), 
					_feature_pipeline->IvectorFeature());
			_decodable->SetFrameOffset(frame_offset);
		}
		_decoder->InitDecoding();
		if(_online_info._online_conf._use_second == true)
		{
			//_diff_lm.Reset();
		}
	}
	
	// input data to decoder
	// data_type is data type: 2 -> short, 4-> float
	// if eos is 1,it's end point and not send end, 
	// nest send data must be InitDecoding(frame_offset, send_end=false)
	int ProcessData(char *data, int date_len, int eos, int data_type);

	/// Gets the lattice.  The output lattice has any acoustic scaling in it
	/// (which will typically be desirable in an online-decoding context); if you
	/// want an un-scaled lattice, scale it using ScaleLattice() with the inverse
	/// of the acoustic weight.  "end_of_utterance" will be true if you want the
	/// final-probs to be included.
	void GetLattice(datemoon::Lattice *clat, 
			bool end_of_utterance);

	/// Outputs an FST corresponding to the single best path through the current
	/// lattice. If "use_final_probs" is true AND we reached the final-state of
	/// the graph then it will include those as final-probs, else it will treat
	/// all final-probs as one.
	void GetBestPath(datemoon::Lattice *best_path,
			bool end_of_utterance);

	void GetBestPathTxt(std::string &best_result, bool end_of_utterance);

	void GetNbest(std::vector<datemoon::Lattice> &nbest_paths, int n, bool end_of_utterance);

	void GetNbestTxt(std::vector<std::string> &nbest_result, int n, bool end_of_utterance);

	// convert onebest lattice convert string text. word_sys must be have.
	void OnebestLatticeToString(datemoon::Lattice &onebest_lattice, std::string &onebest_string);
	
	inline int NumFramesDecoded()
	{
		return _decoder->NumFramesDecoded();
	}
	/// This function calls EndpointDetected from online-endpoint.h,
	/// with the required arguments.
	bool EndpointDetected(const kaldi::OnlineEndpointConfig &config);

private:
	// configure information
	OnlineDecoderInfo &_online_info;
	// nnet forward info
	kaldi::nnet3::DecodableNnetSimpleLoopedInfo _decodable_info;
	
	// decoder
/*#ifdef USE_CLG
	//datemoon::MemOptimizeClgLatticeFasterOnlineDecoder _decoder;
	datemoon::OnlineClgLatticeDecoderMempool _decoder;
#else
	//datemoon::LatticeFasterDecoder _decoder;
	//datemoon::MemOptimizeHclgLatticeFasterOnlineDecoder _decoder;
	//datemoon::OnlineLatticeDecoder _decoder;
	datemoon::OnlineLatticeDecoderMempool _decoder;
	//datemoon::OnlineLatticeDecoderMempoolBiglm _decoder;
#endif
*/
	datemoon::DecoderItf *_decoder;

	// nnet forward
	kaldi::nnet3::DecodableAmNnetLoopedOnline *_decodable;
	// feature pipe
	kaldi::OnlineNnet2FeaturePipeline *_feature_pipeline;
	//datemoon::DiffArpaLm _diff_lm;
};


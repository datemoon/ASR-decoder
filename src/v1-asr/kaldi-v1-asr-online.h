#ifndef __KALDI_V1_ASR_ONLINE_H__
#define __KALDI_V1_ASR_ONLINE_H__

#include <typeinfo>
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

#include "src/v1-asr/v1-online-nnet3-decoding.h"
#include "src/v1-asr/online-nnet3-feature-pipeline-io.h"
#include "src/vad/energy-vad.h"
#include "src/v1-asr/online-vad.h"

namespace kaldi
{
struct V1AsrOpts
{
	// feature option
	OnlineNnet2FeaturePipelineConfig _feature_opts;
	// nnet option
	nnet3::NnetSimpleLoopedComputationOptions _decodable_opts;

	// decoder option
	LatticeFasterDecoderConfig _decoder_opts;
	// end point option
	OnlineEndpointConfig _endpoint_opts;

	bool _decoder_use_final;

	std::string _nnet_vad_config;
	std::string _energy_vad_config;
	bool _use_endpoint;
	bool _use_energy_vad;
	bool _use_model_vad;

	// 这个参数区分是否对vad进行二次平滑
	bool _use_realtime_vad;
	int32 _min_sil_frames_interval;

	// 静音超过这个值，才调用feature_pipeline.InputFinished()
	// 把nnet和decoder全部重置，否则即使vad切开也只重置解码器，
	// 不重置特征和nnet
	int32 _sil_frames_cut;
	std::string _vad_model_filename;


	V1AsrOpts():
		_decoder_use_final(true),
		_use_endpoint(false),
		_use_energy_vad(false),
		_use_model_vad(false), 
		_use_realtime_vad(false),
		_min_sil_frames_interval(50),
		_sil_frames_cut(50),
		_vad_model_filename("") { }

	void Register(OptionsItf *opts)
	{
		_feature_opts.Register(opts);
		_decodable_opts.Register(opts);
		_decoder_opts.Register(opts);
		_endpoint_opts.Register(opts);
		opts->Register("decoder-use-final", &_decoder_use_final,
				"Whether use final in decoder end."
				"If use_final=true and use vad and eos is false," 
				"then will be use FinalizeDecoding,"
				"else not use FinalizeDecoding.");
		opts->Register("use-endpoint", &_use_endpoint,
				"use endpoint (default, false)");
		opts->Register("use-energy-vad", &_use_energy_vad,
				"use energy_vad (default, false)");
		opts->Register("use-model-vad", &_use_model_vad,
				"use model_vad (default, false)");
		opts->Register("use-realtime-vad", &_use_realtime_vad,
				"use_realtime_vad (default, false)");
		opts->Register("min-sil-frames-interval", &_min_sil_frames_interval,
				"min_sil_frames_interval (default, false)");
		opts->Register("nnet-vad-config", &_nnet_vad_config,
				"vad_config file (default, \"\")");
		opts->Register("energy-vad-config", &_energy_vad_config,
				"energy_config file (default, \"\")");
		opts->Register("vad-model-filename", &_vad_model_filename,
				"vad_model_filename (default, \"\")");
	}
};

class V1AsrModel
{
public:
	V1AsrModel(const V1AsrOpts &v1_asr_opt,
			std::string nnet_filename,
			std::string fst_filename,
			std::string words_filename,
			std::string vad_model_filename=""):
		_nnet_filename(nnet_filename), _fst_filename(fst_filename),
		_words_filename(words_filename), 
		_vad_model_filename(vad_model_filename),
		_decoder_fst(NULL),_word_syms(NULL)
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
		_decoder_fst = fst::ReadFstKaldiGeneric(_fst_filename);
		// load words list
		_word_syms = fst::SymbolTable::ReadText(words_filename);
		// load vad nnet
		if(v1_asr_opt._use_model_vad == true)
		{
			if(_vad_model_filename != "")
			{
				ReadKaldiObject(_vad_model_filename, &_vad_nnet);
			}
			else if(v1_asr_opt._vad_model_filename != "")
			{
				_vad_model_filename = v1_asr_opt._vad_model_filename;
				ReadKaldiObject(_vad_model_filename, &_vad_nnet);
			}
			else
			{
				KALDI_ERR << "Use model vad but it's not _vad_model_filename";
			}
		}
	}

	~V1AsrModel()
	{
		if(_decoder_fst != NULL)
			delete _decoder_fst;
		_decoder_fst = NULL;
		if(_word_syms != NULL)
			delete _word_syms;
		_word_syms= NULL;
	}

public:
	std::string _nnet_filename;
	std::string _fst_filename;
	std::string _words_filename;

	std::string _vad_model_filename;

	const fst::Fst<fst::StdArc> *_decoder_fst;
	const fst::SymbolTable *_word_syms;
	TransitionModel _trans_model;
	nnet3::AmNnetSimple _am_nnet;

	nnet3::Nnet _vad_nnet;
};
// 这个类记录ASR所有需要的资源名字和资源加载
// 所有线程共享下面资源
// 其中nnet_filename，fst_filename，words_filename
// 分别是声学模型，解码图HCLG，解码图对应的文本，是必要的
// _vad_model_filename是可选项
class V1AsrSource: public V1AsrModel
{
public:
	V1AsrSource(const V1AsrOpts &v1_asr_opt,
			std::string nnet_filename,
			std::string fst_filename,
			std::string words_filename,
			std::string vad_model_filename=""):
		V1AsrModel(v1_asr_opt, nnet_filename, 
				fst_filename, words_filename, vad_model_filename),
		_feature_info(v1_asr_opt._feature_opts),
		_decodable_info(v1_asr_opt._decodable_opts, &_am_nnet),
		_vad_nnet_info(NULL), 
		_nnet_vad_judge_opts("nnet-vad-judge"), _energy_vad_judge_opts("energy-vad-judge")
	{
		// vad 选项在这里注册，从vad配置文件中读入
		if(v1_asr_opt._use_model_vad == true)
		{
			ReadConfigsFromFile(v1_asr_opt._nnet_vad_config,
					&_model_vad_opts, &_nnet_vad_judge_opts);
			KALDI_ASSERT((v1_asr_opt._vad_model_filename != ""
						|| vad_model_filename != "") 
					&& "use vad, but vad_model_filename is NULL");
			_vad_nnet_info = new VadNnetInfo(_model_vad_opts, &_vad_nnet);
		}
		if(v1_asr_opt._use_energy_vad == true)
		{
			ReadConfigFromFile(v1_asr_opt._energy_vad_config,
					&_energy_vad_judge_opts);
		}
	}

	~V1AsrSource() 
	{
	   if(_vad_nnet_info != NULL)
		   delete _vad_nnet_info;
	}

public:
//	std::string _nnet_filename;
//	std::string _fst_filename;
//	std::string _words_filename;
//
//	std::string _vad_model_filename;
//
//	const fst::Fst<fst::StdArc> *_decoder_fst;
//	const fst::SymbolTable *_word_syms;
//	TransitionModel _trans_model;
//	nnet3::AmNnetSimple _am_nnet;

//	nnet3::Nnet _vad_nnet;

	OnlineNnet2FeaturePipelineInfo _feature_info;
	// this object contains precomputed stuff that is used by all decodable
	// objects.  It takes a pointer to am_nnet because if it has iVectors it has
	// to modify the nnet to accept iVectors at intervals.
	nnet3::DecodableNnetSimpleLoopedInfo _decodable_info;

	VadNnetInfo *_vad_nnet_info;
	// 模型vad选项
	VadNnetSimpleLoopedComputationOptions _model_vad_opts;
	VadJudgeOptions _nnet_vad_judge_opts;

	// 能量vad选项
	VadJudgeOptions _energy_vad_judge_opts;
};

class V1AsrWorker
{
public:
	static int32 GetSampleFreq(const OnlineNnet2FeaturePipelineInfo &feature_opts)
	{
		if(feature_opts.feature_type == "mfcc")
		{
			return feature_opts.mfcc_opts.frame_opts.samp_freq;
		}
		else if(feature_opts.feature_type == "fbank")
		{
			return feature_opts.fbank_opts.frame_opts.samp_freq;
		}
		else if(feature_opts.feature_type == "plp")
		{
			return feature_opts.plp_opts.frame_opts.samp_freq;
		}
		else
		{
			KALDI_ERR << "Unknown feature type " << feature_opts.feature_type;
			return -1;
		}
	}
public:
	V1AsrWorker(const V1AsrOpts &v1_asr_opts, const V1AsrSource &v1_asr_source):
		_v1_asr_opts(v1_asr_opts), _v1_asr_source(v1_asr_source), 
		_feature_pipeline(NULL), _decoder(NULL), _feature_pipeline_io(NULL),
		_vad_nnet3(NULL),
		_v1_energy_vad(_v1_asr_source._energy_vad_judge_opts, 
				"sum_square_root", 16000, 0.025, 0.01, 0, 32768*0.005, 32768*0.05),
		_frame_offset(0),
		_decoder_start_offset(0),
	   	_pre_seg_flag(SIL),
		_continue_sil_frames(0),
		_pre_energy_seg_flag(SIL),
		_continue_energy_sil_frames(0),
		_tot_sil_frames(0),
		_tot_nosil_frames(0)
	{ 
		_sample_freq = GetSampleFreq(_v1_asr_source._feature_info);
	}

	~V1AsrWorker() { Destory(true); }

	void Destory(bool des_vad = true)
	{
		if(_feature_pipeline != NULL && des_vad == true)
		{
			delete _feature_pipeline;
			_feature_pipeline = NULL;
		}
		if(_decoder != NULL)
			delete _decoder;
		_decoder = NULL;
		if(_feature_pipeline_io != NULL)
			delete _feature_pipeline_io;
		_feature_pipeline_io = NULL;
		if(_vad_nnet3 != NULL && des_vad == true)
		{
			delete _vad_nnet3;
			_vad_nnet3 = NULL;
		}
	}

	// 当使用模型vad的时候，模型vad切割音频时，需要在每次切割点，重置解码器，
	// 只有当模型vad结束的时候，才会调用init_nnet3_vad=true，
	// 重置模型vad特征和模型vad前向，清空_best_result结果，
	// 否则会一直缓存从开始到结束的识别结果
	void Init(bool init_all = true, bool init_nnet3_vad = true,int32 frames_offset = 0)
	{
		if(true == init_all)
		{
			_tot_sil_frames = 0;
			_tot_nosil_frames = 0;
		}
		Destory(init_nnet3_vad);
		int32 nnet_frame_offset = 0;
		if(_v1_asr_opts._use_model_vad == true)
		{ // 使用模型vad
			if(init_nnet3_vad == true)
			{
				_pre_seg_flag = SIL;
				_continue_sil_frames = 0;
				_decoder_start_offset = 0;
				_frame_offset = 0;
				_best_result.clear();
				_pre_energy_seg_flag = SIL;
				_cache_energy_best_result.clear();
				_feature_pipeline = new OnlineNnet2FeaturePipeline(_v1_asr_source._feature_info);

				_vad_nnet3 = new VadNnet3(*_v1_asr_source._vad_nnet_info,
						_feature_pipeline->InputFeature(),
						_v1_asr_source._nnet_vad_judge_opts,
						frames_offset,
						nnet_frame_offset,
						_v1_asr_source._model_vad_opts._apply_exp);
			}
			// 如果使用模型vad，特征经过vad判断之后给出是否是静音，实现
			// OnlineNnet3FeaturePipelineIo接口实现流式接收特征，
			// 保证特征不需要重新计算，也通过vad判断是否切断音频，
			// 终止解码或者前向提供策略
			_feature_pipeline_io = new OnlineNnet3FeaturePipelineIo(_v1_asr_source._feature_info,
					_feature_pipeline->Dim());

			_decoder = new V1SingleUtteranceNnet3Decoder(_v1_asr_opts._decoder_opts,
					_v1_asr_source._trans_model,
					_v1_asr_source._decodable_info,
					*_v1_asr_source._decoder_fst,
					_feature_pipeline_io);
			_decoder->InitDecoding(frames_offset);
		}
		else
		{ // 不使用模型vad
			_pre_seg_flag = SIL;
			_continue_sil_frames = 0;
			_decoder_start_offset = 0;
			_frame_offset = 0;
			_best_result.clear();

			_pre_energy_seg_flag = SIL;
			_cache_energy_best_result.clear();
			_feature_pipeline = new OnlineNnet2FeaturePipeline(_v1_asr_source._feature_info);
			_decoder = new V1SingleUtteranceNnet3Decoder(_v1_asr_opts._decoder_opts,
					_v1_asr_source._trans_model,
					_v1_asr_source._decodable_info,
					*_v1_asr_source._decoder_fst,
					_feature_pipeline);
			_decoder->InitDecoding(frames_offset);
		}
	}

	// 如果你不想重置解码前向计算，可以调用这个函数，设置_frame_offset
	void Reset(bool eos = true)
	{
		if(eos)
		{
			_frame_offset = 0;
		}
		_decoder->InitDecoding(_frame_offset);
	}

	VadSeg VadNnet3Process(const VectorBase<BaseFloat> &wav_data, 
			bool eos = false)
	{
		if(wav_data.Dim() > 0)
		{
			_feature_pipeline->AcceptWaveform(_sample_freq, wav_data);
		}
		if(eos == true)
		{
			_feature_pipeline->InputFinished();
		}
		return _vad_nnet3->ComputerNnet(eos);
	}

	template<class OutT>
	void ConvertData(const char *data, int32 data_len, int32 data_type, Vector<OutT> &out_data)
	{
		if(data_len%data_type != 0)
		{
			KALDI_WARN << "Data have some problem!!!";
		}
		int32 len = data_len/data_type;
		out_data.Resize(len);
		for(int32 i=0; i < len ; ++i)
		{
			if(data_type == 2)
				out_data(i) = ((short*)(data))[i];
			else if(data_type == 4)
				out_data(i) = ((BaseFloat*)(data))[i];
		}
	}

	void GetSilAndNosil(int32 &sil_frames, int32 &nosil_frames)
	{
		sil_frames = _tot_sil_frames;
		nosil_frames = _tot_nosil_frames;
		return ;
		if(_v1_asr_opts._use_model_vad == true)
		{
			VadSeg vad_ali;
			if(_v1_asr_opts._use_realtime_vad == false)
			{
				_vad_nnet3->GetTotalVadAli(vad_ali,
						sil_frames, nosil_frames,
						true, _v1_asr_opts._min_sil_frames_interval);
			}
			else
			{
				_vad_nnet3->GetTotalVadAli(vad_ali,
						sil_frames, nosil_frames);
			}
		}
		else
		{
			sil_frames = 0; 
			nosil_frames = 0;
		}
	}

	// 带能量vad，模型vad，处理
	std::string Process(const char *data, int32 data_len, int32 data_type,
			int32 nbest = 0, bool all_eos = false)
	{
		if(true == _v1_asr_opts._use_energy_vad)
		{
			Vector<BaseFloat> wav_data;
			ConvertData(data, data_len, data_type, wav_data);
			_v1_energy_vad.AcceptData(wav_data);
			VadSeg vad_ali = _v1_energy_vad.ComputerVad(all_eos);
			if(_v1_asr_opts._use_realtime_vad == false)
			{
				if(all_eos == true)
				{ // 压缩vad的切割，将非静音间静音段小于_v1_asr_opts._min_sil_frames_interval的静音和前后非静音合并为一段非静音
					int32 sil_frames = -1, nosil_frames = -1;
					_v1_energy_vad.GetTotalVadAli(vad_ali, sil_frames, nosil_frames, 
							true, _v1_asr_opts._min_sil_frames_interval);
					PrintVadSeg(vad_ali, "compress-energy-vad-ali-> ", 3);
				}
				else
				{
					return _cache_energy_best_result;
				}
			}

			for(VadSeg::iterator it = vad_ali.begin();
					it != vad_ali.end(); ++it)
			{
				int32 cur_seg_freames = it->second.second - it->second.first;
				if(it->first == SIL)
				{ 
					_tot_sil_frames += cur_seg_freames;
					// 上一段是语音
					if(_pre_energy_seg_flag == AUDIO)
					{
						std::vector<BaseFloat> cur_wav_data;
						int32 start_frame = it->second.first;
						int32 end_frame = start_frame;
						_v1_energy_vad.GetData(cur_wav_data, start_frame, end_frame, true);
	
						// 送入数据， 获取解码结果
						// 能量vad切断，表示结束这段解码，all_eos=true
						std::string best_result = Process(cur_wav_data, nbest, true);
						// 重置解码器，如果启用模型vad，也重置模型vad，但不重置语音帧和静音帧
						Init(false, true, 0);
						// 记录之前的结果
						_best_result = best_result;
						_cache_energy_best_result = best_result;
				   	}
					else if(_pre_energy_seg_flag == SIL)
					{ // 上一段和现在都是静音
						;
					}
					_pre_energy_seg_flag = SIL;
				}
				// 无论上一段是什么，当前是非静音就将特征放入解码器
				else if(it->first == AUDIO)
				{
					// 获取数据，送入解码
					{
						std::vector<BaseFloat> cur_wav_data;
						int32 start_frame = it->second.first; 
						int32 end_frame = it->second.second;
						std::string best_result;
						if(it+1 == vad_ali.end() && all_eos == true)
						{ // 最后一段音频
							_v1_energy_vad.GetData(cur_wav_data, start_frame, end_frame, true);
							_cache_energy_best_result = Process(cur_wav_data, nbest, true);
						}
						else
						{
							_v1_energy_vad.GetData(cur_wav_data, start_frame, end_frame, false);
						   _cache_energy_best_result = Process(cur_wav_data, nbest, false);
						}
					}
					_pre_energy_seg_flag = AUDIO;
				}
				else
				{
					KALDI_WARN << "It\'s shouldn\'t happen!!!";
				}
			} // energy vad getdata ok
			if(true == all_eos)
				_v1_energy_vad.Reset();
			return _cache_energy_best_result;
		}
		// no use energy vad
		else
		{
			Vector<BaseFloat> wav_data;
			ConvertData(data, data_len, data_type, wav_data);
			return Process(wav_data, nbest, all_eos);
		}
	}

	// 模型vad处理，或者不带模型vad
	std::string Process(const std::vector<BaseFloat> &wav_data, int32 nbest = 0,
			bool all_eos = false)
	{
		Vector<BaseFloat> use_wav_data;
		use_wav_data.Resize(wav_data.size());
		memcpy(use_wav_data.Data(), wav_data.data(), wav_data.size()*sizeof(BaseFloat));
		return Process(use_wav_data, nbest, all_eos);
	}

	std::string Process(const VectorBase<BaseFloat> &wav_data, int32 nbest = 0,
			bool all_eos = false)
	{
		// 首先判断是否使用vad
		VadSeg vad_ali;
		if(_v1_asr_opts._use_model_vad == true)
		{
			// 获取vad分段信息
			vad_ali = VadNnet3Process(wav_data, all_eos);
			if(_v1_asr_opts._use_realtime_vad == false)
			{
				if(all_eos == true)
				{ // 压缩vad的切割，将非静音间静音段小于_v1_asr_opts._min_sil_frames_interval的静音和前后非静音合并为一段非静音
					int32 sil_frames = -1, nosil_frames = -1;
					_vad_nnet3->GetTotalVadAli(vad_ali,
						   	sil_frames, nosil_frames,
							true, _v1_asr_opts._min_sil_frames_interval);
					PrintVadSeg(vad_ali, "compress-nnet3-vad-ali-> ", 3);
				}
				else
				{
					return _best_result;
				}
			}
			for(VadSeg::iterator it = vad_ali.begin();
					it != vad_ali.end(); ++it)
			{
				//if(it + 1 == vad_ali.end())
				//	_current_offset += it->second.second;
				// 只有是最后一段all_eos=true时，eos=true
				//bool eos = (it + 1 == vad_ali.end()) ? all_eos : false;
				int32 cur_seg_freames = it->second.second - it->second.first;
				// 当前是静音，之前是语音，则获取上一段最终结果
				if(it->first == SIL)
				{
					_tot_sil_frames += cur_seg_freames;
					// 计算连续语音帧的长度
					_continue_sil_frames += cur_seg_freames;
					if(_pre_seg_flag == AUDIO)
					{
						// 获取解码结果
						std::vector<std::string> nbest_result;
						bool use_final = _v1_asr_opts._decoder_use_final;
						bool eos = true;
						// 最后一段音频
						if(it+1 == vad_ali.end())
						{
							if(all_eos == true)
							{
								use_final = true;
							}
						}
						//if(_continue_sil_frames > _v1_asr_opts._sil_frames_cut)
						{ // 切断，删掉中间的静音，不做前向和解码
							
							Decoding(nbest_result, eos, nbest, use_final);
							if(nbest_result.size() > 0 && nbest_result[0].size() > 0)
							{
								_best_result += nbest_result[0];
								if(it+1 != vad_ali.end())
								{
									_best_result += ",";
								}
							}
							// 初始化前向和解码器，释放原有解码器和前向
							// 但不重新初始化vad
							Init(false, false);
						}
					}
					else if(_pre_seg_flag == SIL)
					{// 上一段和现在都是静音
						;
					}
					//else
					//{
					//	use_final = false;
					//	int32 nres = GetResult(nbest_result, &time_begin, &time_end,
					//			nbest, use_final, eos);
						// 只初始化解码器(to do)
					//}
					// 设置当前段标记
					_pre_seg_flag = SIL;
				}
				// 无论上一段是什么，当前是非静音就将特征放入解码器
				else if(it->first == AUDIO)
				{
					_tot_nosil_frames += cur_seg_freames;
					// 获取这段特征送入解码
					int32 begin_input_frame = it->second.first;
					int32 end_input_frame = it->second.second;
					Vector<BaseFloat> this_feature(_feature_pipeline->Dim(), 
							kUndefined);
					for(int32 i = begin_input_frame; i < end_input_frame; ++i)
					{
						_feature_pipeline->GetFrame(i, &this_feature);
						_feature_pipeline_io->AcceptFeature(&this_feature);
					}
					_continue_sil_frames = 0;
					if(_pre_seg_flag == SIL)
						_decoder_start_offset = begin_input_frame;

					// 最后一段音频
					if(it+1 == vad_ali.end())
					{
						if(nbest > 0 || all_eos == true)
						{
							std::vector<std::string> nbest_result;
							bool use_final = false;
							bool eos = false;
							// 只有是真正的最后一段送入音频时候才终止解码，采用use_final
							if(all_eos == true)
							{
								use_final = true;
								eos = true;
							}
							Decoding(nbest_result, eos, nbest, use_final);
							if(nbest_result.size() > 0)
							{
								if(all_eos == true)
									_best_result += nbest_result[0];
							}
						}
					}
					else
					{
						std::vector<std::string> nbest_result;
						bool use_final = false;
						bool eos = false;
						Decoding(nbest_result, eos, nbest, use_final);
					}
					_pre_seg_flag = AUDIO;
				}
				else
				{
					KALDI_WARN << "It\'s shouldn\'t happen!!!";
				}
			}
		}
		else // no model vad
		{
			AcceptWaveform(wav_data);
			bool use_final = false;
			bool eos = false;
			if(all_eos == true)
			{
				use_final = _v1_asr_opts._decoder_use_final;
				eos = true;
			}
			std::vector<std::string> nbest_result;
			Decoding(nbest_result, eos, nbest, use_final);
			_tot_nosil_frames = _decoder->NumFramesDecoded() * _v1_asr_opts._decodable_opts.frame_subsampling_factor;;
			if(nbest_result.size() > 0 )
			{
				return _best_result + nbest_result[0];
			}
			else
			{
				return _best_result;
			}
		}
		return _best_result;
	}
	
	int32 Decoding(std::vector<std::string> &nbest_result,
			bool eos = false, int32 nbest = 0, bool use_final = false)
	{
		if(eos == true)
		{ 
			// 结束必须将特征全部送入，保证nnet前向计算全部特征
			// decoder解码全部输入
			if(_v1_asr_opts._use_model_vad == true)
			{
				_feature_pipeline_io->InputFinished();
			}
			else
			{
				_feature_pipeline->InputFinished();
			}
			_decoder->AdvanceDecoding();
			if(use_final == true)
				_decoder->FinalizeDecoding();
		}
		else
		{
			_decoder->AdvanceDecoding();
		}
		if(_decoder->NumFramesDecoded() > 0 && 
				(nbest > 0 || eos == true))
		{ // 获取结果
			int32 time_begin, time_end;
			int32 nres = GetResult(nbest_result, &time_begin, &time_end,
					nbest, use_final, eos);
			return nres;
		}
		return 0;
	}

	// 这个主要被用于接收特征，当经过模型vad之后，重用特征
	void AcceptFeature(const MatrixBase<BaseFloat> *features)
	{
		_feature_pipeline_io->AcceptFeatureMatrix(features);
	}

	void AcceptWaveform(const VectorBase<BaseFloat> &wav_data)
	{
		_feature_pipeline->AcceptWaveform(_sample_freq, wav_data);
	}

	// 直接送入数据获取结果，没有vad
	int32 ProcessData(const VectorBase<BaseFloat> &wav_data, std::vector<std::string> &nbest_result, 
			bool eos = false, int32 nbest = 0, bool use_final = false)
	{
		AcceptWaveform(wav_data);
		return Decoding(nbest_result, eos, nbest, use_final);
	}

	// 送入特征，如果经过模型vad，之后获取非静音特征，然后送入
	int32 ProcessData(const MatrixBase<BaseFloat> *feature, std::vector<std::string> &nbest_result, 
			bool eos = false, int32 nbest = 0, bool use_final = false)
	{
		AcceptFeature(feature);
		return Decoding(nbest_result, eos, nbest, use_final);
	}

	// 如果你希望得到time_begin， time_end
	int32 GetResult(std::vector<std::string> &nbest_result, 
			int32 *time_begin, int32 *time_end, 
			int32 nbest = 0, bool use_final = false, bool eos = false)
	{
		//BaseFloat frame_shift = _v1_asr_source._feature_info.FrameShiftInSeconds();
		int32 frame_subsampling = _v1_asr_opts._decodable_opts.frame_subsampling_factor;
		*time_begin = _decoder_start_offset;
		*time_end = *time_begin + _decoder->NumFramesDecoded() * frame_subsampling;
		GetNbest(nbest_result, nbest, use_final);
		
		if (nbest_result.size() > 0)
			KALDI_LOG << "start_time : " << *time_begin
				<< " end_time : " << *time_end
				<< " result: " << nbest_result[0];
		else
			KALDI_LOG << "start_time : " << *time_begin
				<< " end_time : " << *time_end
				<< " result: (NULL)";

		return nbest_result.size();
	}

	// 获取nbest结果
	void GetNbest(std::vector<std::string> &nbest_result, 
			int32 n = 0,  bool use_final=false)
	{
		if(n > 1)
		{
			std::vector<CompactLattice> nbest_lats;
			CompactLattice nbest_lat;
			CompactLattice clat;
			_decoder->GetLattice(use_final, &clat);
			fst::ShortestPath(clat, &nbest_lat, n);
			fst::ConvertNbestToVector(nbest_lat, &nbest_lats);
			for(size_t i = 0 ; i < nbest_lats.size(); ++i)
			{
				//Lattice olat;
				//ConvertLattice(nbest_lats[i], &olat);
				typename CompactLattice::Arc::Weight weight;
				std::string result = ConvertLat2Onebest(nbest_lats[i], &weight);
				KALDI_VLOG(3) << "Cost for utterance : " << result << " "
				   	<< weight.Weight().Value1() 
					<< " + " << weight.Weight().Value2() 
					<< " = " << weight.Weight().Value1()+ weight.Weight().Value2();
				nbest_result.push_back(result);
			}
		}
		else
		{
			std::string result;
			GetOnebest(result, use_final);
			nbest_result.push_back(result);
		}
	}
	// 获取one best结果
	void GetOnebest(std::string &one_result, bool use_final=false)
	{
		Lattice olat;
		_decoder->GetBestPath(use_final, &olat);
		TopSort(&olat);

		typename Lattice::Arc::Weight weight;
		one_result = ConvertLat2Onebest(olat, &weight);
		KALDI_VLOG(3) << "Cost for utterance : " << one_result << " "
			<< weight.Value1() 
			<< " + " << weight.Value2() 
			<< " = " << weight.Value1()+ weight.Value2();

	}

	// 将onebest lattice 转换为文本结果
	template<class Arc>
	std::string ConvertLat2Onebest(const fst::Fst<Arc> &olat, typename Arc::Weight *weight=NULL)
	{
		std::vector<int32> alignment;
		std::vector<int32> words;
		//LatticeWeight weight;
		GetLinearSymbolSequence(olat, &alignment, &words, weight);
		return ConvertWordId2Str(words);
	}

	std::string ConvertWordId2Str(std::vector<int32> &words)
	{
		std::ostringstream result;
		for(size_t i = 0 ; i < words.size() ; ++i)
		{
			 std::string s = _v1_asr_source._word_syms->Find(words[i]);
			 if(s.empty())
			 {
				 KALDI_WARN << "wrod-id " << words[i] << " not in symbol table!!!";
				 result << "<#" << std::to_string(words[i]) << "> ";
			 }
			 else
				 result << s << " ";
		}
		return result.str();
	}
private:
	const V1AsrOpts &_v1_asr_opts;
	const V1AsrSource &_v1_asr_source;
	//const OnlineNnet2FeaturePipelineInfo &_feature_info;

	//const nnet3::DecodableNnetSimpleLoopedInfo &_decodable_info;


	OnlineNnet2FeaturePipeline *_feature_pipeline; // 接收wav数据
	V1SingleUtteranceNnet3Decoder *_decoder;
	// 如果使用 vad 将使用 这个特征管道来接收vad之后的特征
	OnlineNnet3FeaturePipelineIo *_feature_pipeline_io; // 接收特征数据
	VadNnet3 *_vad_nnet3;

	V1EnergyVad<BaseFloat> _v1_energy_vad;

	// 记录解码器切断的位置，但是nnet不切断，
	// 前向计算和特征偏移量
	int32 _frame_offset;
	int32 _sample_freq;
	// 记录解码器开始位置
	int32 _decoder_start_offset;
	// 记录到上一次进来数据，处理了多少帧
	//int32 _current_offset;

	// 如果使用nnet3 vad，流式处理这个将判断之前一段是否是语音
	int32 _pre_seg_flag;
	int32 _continue_sil_frames;

	int32 _pre_energy_seg_flag;
	int32 _continue_energy_sil_frames;

	std::string _best_result;
	// 为了对比和之前的结果是否一致，如果一致，则不返回best_result而是""
	// 暂时不用
	std::string _cache_energy_best_result;

	int32 _tot_sil_frames;
	int32 _tot_nosil_frames;
};

} // namespace kaldi

#endif


#include "feat/wave-reader.h"
#include "util/common-utils.h"
#include "src/online-vad/online-vad.h"

int main(int argc, char* argv[])
{
	try
	{
		using namespace kaldi;
		const char *usage = "Test kaldi nnet3 vad code.\n";
		int32 samp_freq = 16000;
		int32 channel = -1;
		bool apply_exp = true, use_priors = false;
		ParseOptions po(usage);

		OnlineNnet2FeaturePipelineConfig feature_opts;
		VadNnetSimpleLoopedComputationOptions vad_nnet_opts;
		VadJudgeOptions vad_judge_opt;
		feature_opts.Register(&po);
		vad_nnet_opts.Register(&po);
		vad_judge_opt.Register(&po);


		po.Register("samp-freq", &samp_freq, "sample frequecy.");
		po.Register("channel", &channel, "Channel to extract (-1 -> expect mono, "
				"0 -> left, 1 -> right)");
		po.Register("use-priors", &use_priors, "If true, subtract the logs of the "
				"priors stored with the model (in this case, "
				"a .mdl file is expected as input).");
		po.Register("apply-exp", &apply_exp, "If true, apply exp function to "
				"output");

		po.Read(argc, argv);

		if(po.NumArgs() != 2)
		{
			po.PrintUsage();
			exit(1);
		}

		std::string nnet3_filename = po.GetArg(1);
		std::string wav_rspecifier = po.GetArg(2);
		nnet3::AmNnetSimple am_nnet;
		nnet3::Nnet raw_nnet;
		TransitionModel trans_model;
		if(use_priors)
		{
			bool binary;
			Input ki(nnet3_filename, &binary);
			trans_model.Read(ki.Stream(), binary);
			am_nnet.Read(ki.Stream(), binary);
		}
		else
		{
			ReadKaldiObject(nnet3_filename, &raw_nnet);
		}// load am ok.
		nnet3::Nnet &nnet = (use_priors ? am_nnet.GetNnet() : raw_nnet);
		nnet3::SetBatchnormTestMode(true, &nnet);
		nnet3::SetDropoutTestMode(true, &nnet);
		nnet3::CollapseModel(nnet3::CollapseModelConfig(),
					&nnet);


		VadNnetInfo vad_nnet_info(vad_nnet_opts, &nnet);
		// feature
		OnlineNnet2FeaturePipelineInfo feature_info(feature_opts);

		//int32 outdim = am_nnet.GetNnet().OutputDim("output");

		SequentialTableReader<WaveHolder> reader(wav_rspecifier);

		BaseFloat min_duration = 0.0;
		int32 num_utts = 0;// num_success = 0;
		for(; !reader.Done(); reader.Next())
		{
			num_utts++;
			std::string utt = reader.Key();
			const WaveData &wave_data = reader.Value();
			if (wave_data.Duration() < min_duration)
			{
				KALDI_WARN << "File: " << utt << " is too short ("
					<< wave_data.Duration() << " sec): producing no output.";
				continue;
			}
			int32 num_chan = wave_data.Data().NumRows(), this_chan = channel;

			{  // This block works out the channel (0=left, 1=right...)
				KALDI_ASSERT(num_chan > 0);  // This should have been caught in
				// reading code if no channels.
				if (channel == -1) 
				{
					this_chan = 0;
					if (num_chan != 1)
						KALDI_WARN << "Channel not specified but you have data with "
							<< num_chan  << " channels; defaulting to zero";
				} 
				else 
				{
					if (this_chan >= num_chan) 
					{
						KALDI_WARN << "File with id " << utt << " has "
							<< num_chan << " channels but you specified channel "
							<< channel << ", producing no output.";
						continue;
					}
				}
			}

			OnlineNnet2FeaturePipeline feature_pipeline(feature_info);
			int32 frames_offset = 0;
			int32 nnet_frame_offset = 0;
			VadNnet3 vad_nnet3(vad_nnet_info, feature_pipeline.InputFeature(),
					vad_judge_opt,
				   	frames_offset,
					nnet_frame_offset,
					apply_exp);
			SubVector<BaseFloat> waveform(wave_data.Data(), this_chan);
			int32 wav_len = waveform.Dim();
			VadSeg tot_ali;
			VadSeg tot_ali_tmp;
			int32 pack_id=0;
			int32 m_dur_time_speech;
			int32 m_dur_time_sil;
			int32 start_time;
			int32 begin_time;
			int32 end_time;
			int32 send_len = 8000;
			bool m_is_start = false;
			bool m_is_decode = false;
			int32 num_speech=0;
			int32 num_sil=0;
			int32 m_adv_fnum=40;
			int32 m_end_fnum=25;
			int32 reset_fnum=70;
			int32 tos_fnum=3000;


			for(int32 offset=0; offset < wav_len ; )
			{
				if(offset + send_len > wav_len)
					send_len = wav_len - offset;
				SubVector<BaseFloat> cur_wav(waveform, offset, send_len);
				offset += send_len;
				feature_pipeline.AcceptWaveform(samp_freq, cur_wav);
				if(offset == wav_len)
				{
					pack_id *= -1;   // 11
					feature_pipeline.InputFinished();
					VadSeg cur_ali = vad_nnet3.ComputerNnet(1);
					tot_ali.insert(tot_ali.end(),
							cur_ali.begin(), cur_ali.end());
					tot_ali_tmp.insert(tot_ali_tmp.end(),
							cur_ali.begin(), cur_ali.end());
					
					

				}
				else
				{
					pack_id++; // 11 
					VadSeg cur_ali =vad_nnet3.ComputerNnet(0);
					tot_ali.insert(tot_ali.end(),
							cur_ali.begin(), cur_ali.end());
					tot_ali_tmp.insert(tot_ali_tmp.end(),
							cur_ali.begin(), cur_ali.end());
					
				}

				VadSeg compress_vad_ali_tmp;
				CompressAlignVad(tot_ali_tmp, compress_vad_ali_tmp, 50);
                                PrintVadSeg(compress_vad_ali_tmp, "compress-vad-ali-tmp-> ");
				for(auto it = compress_vad_ali_tmp.begin(); it != compress_vad_ali_tmp.end(); ++it)
	                          {
		                      std::string type;
				      if(m_is_decode && it == compress_vad_ali_tmp.end()-1 && pack_id <=0 ) // 11 
				      {
				         if(it->second.second - begin_time > tos_fnum)
					 {
					     end_time=begin_time+tos_fnum;
					 }
					 else
					 {
					     if(it->first == SIL)
					     { 
					          if(it->second.first + m_end_fnum <= it->second.second )
						  {
					            end_time=it->second.first + m_end_fnum;
						  }
					          else
					          {
					             end_time=it->second.second;
					          }
					     }
					     else
					     {
					        end_time=it->second.second;
					     }
					 }
					 printf("EOS %d \n", end_time);
					 m_is_decode=false;
					 m_is_start = false;
					 compress_vad_ali_tmp.erase(compress_vad_ali_tmp.begin(),compress_vad_ali_tmp.end());
                                         tot_ali_tmp.erase(tot_ali_tmp.begin(),tot_ali_tmp.end());
					 break;
					 
				      }
		                      
				      if(it->first == SIL)
				      {
				         type = "sil   ";
					 m_dur_time_sil=it->second.second - it->second.first;
					  
					 if(m_dur_time_sil>reset_fnum && m_is_decode && num_sil >=1)
					 {
					    end_time=it->second.first + m_end_fnum;
					    printf("EOS %d \n", end_time);
					    m_is_decode=false;
                                            m_is_start = false;
					    int32 end_val=it->second.second;

					    for(auto itt = tot_ali_tmp.begin(); itt != tot_ali_tmp.end(); ++itt)
					    {
					        int32 end_val_tmp=itt->second.first; // 11
						if(end_val_tmp==end_val)
						{
                                                   tot_ali_tmp.erase(tot_ali_tmp.begin(),itt);//11
						   break;
						}
					    }
                        if(pack_id>0) // long chunk
					    {
					      compress_vad_ali_tmp.erase(compress_vad_ali_tmp.begin(),compress_vad_ali_tmp.end()); 
					      break;
					    }
					    //break; //11
					 }
                                         num_sil++;
				      }
		                      else
				      {
                                         type = "nosil ";
					 m_dur_time_speech=it->second.second - it->second.first;
					 
					 if(m_dur_time_speech >=50)
					 {
					    
					    if(m_is_start == false)
					    {
					       begin_time=it->second.first;
					       start_time=it->second.first - m_adv_fnum;
					       if(start_time<0)
					        {
					          start_time=0;
					        }
					       m_is_start = true;
					       printf("START %d \n", start_time);
                                               //compress_vad_ali_tmp.erase(compress_vad_ali_tmp.begin(),compress_vad_ali_tmp.end());
                                               for(auto ittt = tot_ali_tmp.begin(); ittt != tot_ali_tmp.end(); ++ittt)
					        {
					           int32 end_val_tmp1=ittt->second.first;//11 
						   if(end_val_tmp1==begin_time)
						    {
                                                       tot_ali_tmp.erase(tot_ali_tmp.begin(),ittt);//11
						       break;
						    }
					         }                                     					     }
					     m_is_decode=true;
					   if(it == compress_vad_ali_tmp.end()-1 && pack_id <= 0)//11
					   {
					     end_time=it->second.second;
                                             printf("EOS %d \n", end_time);
					     m_is_decode=false;
					     m_is_start = false;
					     compress_vad_ali_tmp.erase(compress_vad_ali_tmp.begin(),compress_vad_ali_tmp.end());
                                             tot_ali_tmp.erase(tot_ali_tmp.begin(),tot_ali_tmp.end());
					     break;
					   }
				        }
					 num_speech++;
				      }

	                           }
			}
			PrintVadSeg(tot_ali, "tot-> ");
			VadSeg compress_vad_ali;
			CompressAlignVad(tot_ali, compress_vad_ali, 50);
			PrintVadSeg(compress_vad_ali, "compress-vad-ali-> ");
		}
		return 0;
	}
	catch(const std::exception &e)
	{
		std::cerr << e.what();
		return -1;
	}
}


#include "feat/wave-reader.h"
#include "util/common-utils.h"
#include "src/v1-asr/online-vad.h"

int main(int argc, char* argv[])
{
	try
	{
		using namespace kaldi;
		const char *usage = "Test online energy vad code.\n"
			"online-energy-vad-stream-test scp:wav.scp";
		ParseOptions po(usage);

		VadJudgeOptions vad_judge_opt("energy-vad-judge");
		vad_judge_opt.Register(&po);

		po.Read(argc, argv);

		if(po.NumArgs() != 1)
		{
			po.PrintUsage();
			exit(1);
		}
		std::string wav_rspecifier = po.GetArg(1);

		V1EnergyVad<BaseFloat> v1_energv_vad(vad_judge_opt,
				"sum_square_root",
				16000, 0.025, 0.01, 0, 32768*0.005, 32768*0.05);

		SequentialTableReader<WaveHolder> reader(wav_rspecifier);

		BaseFloat min_duration = 0.0;
		int32 num_utts = 0;// num_success = 0;
		for(; !reader.Done(); reader.Next())
		{
			v1_energv_vad.Reset();
			num_utts++;
			std::string utt = reader.Key();
			const WaveData &wave_data = reader.Value();
			if (wave_data.Duration() < min_duration)
			{
				KALDI_WARN << "File: " << utt << " is too short ("
					<< wave_data.Duration() << " sec): producing no output.";
				continue;
			}
			int32 this_chan = 0;

			SubVector<BaseFloat> waveform(wave_data.Data(), this_chan);
			int32 wav_len = waveform.Dim();
			int32 send_len = 8000;
			VadSeg tot_ali;
			for(int32 offset=0; offset < wav_len ; )
			{
				if(offset + send_len > wav_len)
					send_len = wav_len - offset;
				SubVector<BaseFloat> cur_wav(waveform, offset, send_len);
				offset += send_len;

				v1_energv_vad.AcceptData(cur_wav);
				if(offset == wav_len)
				{
					VadSeg cur_ali = v1_energv_vad.ComputerVad(1);
					tot_ali.insert(tot_ali.end(),
							cur_ali.begin(), cur_ali.end());
				}
				else
				{
					VadSeg cur_ali = v1_energv_vad.ComputerVad();
					tot_ali.insert(tot_ali.end(),
							cur_ali.begin(), cur_ali.end());
				}
			}
			PrintVadSeg(tot_ali, "tot-> ", 0);
			VadSeg compress_vad_ali;
			CompressAlignVad(tot_ali, compress_vad_ali, 50);
			PrintVadSeg(compress_vad_ali, "compress-vad-ali-> ", 0);
		}
		return 0;
	}
	catch(const std::exception &e)
	{
		std::cerr << e.what();
		return -1;
	}
}

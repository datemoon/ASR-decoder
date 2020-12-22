#include "src/vad/energy-vad.h"
#include <iostream>
#include <vector>
#include <string.h>
#include <assert.h>

#ifdef NAMESPACE
using namespace datemoon;
#endif

template<typename T>
bool WriteSegVad(const std::string &vad_outfile, std::vector<T> &data, int len_num = 160)
{
	FILE* fp = fopen(vad_outfile.c_str(), "a+");
	if(fp == NULL)
	{
		LOG_WARN << "Open " << vad_outfile << " failed!!!";
		return false;
	}
	for(size_t i =0; i < data.size() ; ++i)
	{
		int elem = data[i];
		if(i % len_num == 0 && i != 0)
			fprintf(fp, "\n");
		fprintf(fp, "%d ", elem);
	}
	fprintf(fp, "\n");
	fclose(fp);
	return true;
}

int main(int argc, char *argv[])
{
	if(argc != 4)
	{
		std::cerr << argv[0] << " wavlist head_length read_time(0/1)" << std::endl;
		return -1;
	}
	char *wavlist = argv[1];
	int head_length = atoi(argv[2]);
	int real_time = atoi(argv[3]);

	//EnergyVad energy_vad(16000, 0.025, 0.01, 5, 5, 10, 10*2, "short", "sum_square_root");
	//EnergyVad energy_vad(16000, 0.025, 0.01, 0.3, 10, 10, 32768*0.005, 32768*0.05, "short",  "sum_abs");
	EnergyVad<short> energy_vad(16000, 0.025, 0.01, -1, EnergyVad<short>::SIL, 0.5, 0.8, 20, 20, 32768*0.01, 32768*0.01, "sum_square_root");
	FILE* fp = fopen(wavlist, "rb");
	if(fp == NULL)
	{
		std::cerr << "fopen " << wavlist << " failed!!!" <<std::endl;
		return -1;
	}
	std::vector<short> wav_data;
	while(1)
	{
		wav_data.clear();
		energy_vad.Reset();
		char line[1024];
		memset(line,0x00,1024);
		char *tmp = fgets(line, 1024, fp);
		if(tmp != NULL)
		{
			line[strlen(line)-1] = '\0';
		}
		else
		{
			break;
		}

		std::string vad_outfile = std::string(line) + ".vad_outfile";
		std::string source_file = std::string(line) + ".source_file";
		FILE *wavfp = fopen(line, "rb");
		if(wavfp == NULL)
		{
			std::cerr << "open " << line << " failed!!!" << std::endl;
			continue;
		}
#define DATA_LEN 8000
		short data[DATA_LEN];
		std::vector<float> frames_energy;
		std::vector<int> frames_judge;
		int read_len = 0;
		if( head_length > 0 && head_length < DATA_LEN*sizeof(short))
		{
			read_len = fread(data,sizeof(char), head_length, wavfp);
		}
		std::vector<short> out_data;
		int getdata_audio_flag = 0;
		while(1)
		{
			std::vector<float> cur_frames_energy;
			std::vector<int> cur_frames_judge;
			read_len = fread(data,sizeof(short),DATA_LEN, wavfp);
			if (read_len == 0)
			{
				cur_frames_judge = energy_vad.JudgeFramesFromEnergy(cur_frames_energy,1);
				frames_judge.insert(frames_judge.end(),
						cur_frames_judge.begin(), cur_frames_judge.end());
				assert(frames_energy.size() == frames_judge.size());
			
				if(real_time)
				{
					EnergyVad<short>::VadSeg vad_res = energy_vad.CompressVadRes(cur_frames_judge);
					while(1)
					{
						int vad_ret = energy_vad.GetData(out_data, getdata_audio_flag);
						if(out_data.size() > 0 )
						{
							bool wret = WriteSegVad(vad_outfile, out_data, 160);
							if(wret != true)
							{
								LOG_WARN << "WriteSegVad " << vad_outfile << " failed!!!";
							}
							std::cout << "out_data size is " << out_data.size() << std::endl;
						}
						if(vad_ret == 0)
							break;
					}
				}
				else
				{
					EnergyVad<short>::VadSeg tot_vad_res = energy_vad.CompressVadRes(frames_judge);
					energy_vad.Print(tot_vad_res);
					while(1)
					{
						int vad_ret = energy_vad.GetData(out_data, getdata_audio_flag);
						if(out_data.size() > 0 )
						{
							bool wret = WriteSegVad(vad_outfile, out_data, 160);
							if(wret != true)
							{
								LOG_WARN << "WriteSegVad " << vad_outfile << " failed!!!";
							}
							std::cout << "out_data size is " << out_data.size() << std::endl;
						}
						if(vad_ret == 0)
							break;
					}
				}
				int sil_frames = 0 , nosil_frames = 0;
				energy_vad.GetSilAndNosil(sil_frames, nosil_frames);
				std::cout << "sil_frames = " << sil_frames << 
					", nosil_frames = " << nosil_frames << std::endl;
				break;
			}
			std::vector<short> v_data(data, data+read_len);
			wav_data.insert(wav_data.end(), v_data.begin(), v_data.end());
			cur_frames_energy = energy_vad.FramesEnergy(v_data);
			frames_energy.insert(frames_energy.end(),
						cur_frames_energy.begin(), cur_frames_energy.end());
			cur_frames_judge = energy_vad.JudgeFramesFromEnergy(cur_frames_energy,0);
			frames_judge.insert(frames_judge.end(),
				   	cur_frames_judge.begin(), cur_frames_judge.end());

			if(real_time)
			{
				EnergyVad<short>::VadSeg vad_res = energy_vad.CompressVadRes(cur_frames_judge);
				while(1)
				{
					int vad_ret = energy_vad.GetData(out_data, getdata_audio_flag);
					if(out_data.size() > 0 )
					{
						bool wret = WriteSegVad(vad_outfile, out_data, 160);
						if(wret != true)
						{
							LOG_WARN << "WriteSegVad " << vad_outfile << " failed!!!";
						}
						std::cout << "out_data size is " << out_data.size() << std::endl;
					}
					if(vad_ret == 0)
						break;
				}
			}
		}
		bool wret = WriteSegVad(source_file, wav_data, 160);
		if(wret != true)
		{
			LOG_WARN << "WriteSegVad " << source_file << " failed!!!";
		}


		int linenum = 100;
		for(int f=0;f<frames_energy.size();f+=linenum)
		{
			int end = linenum < frames_energy.size()-f ? linenum : frames_energy.size()-f;
			std::cout << line <<  " ";
			for(int i=f;i<f+end;++i)
			{
				std::cout << "(" << i << "," << frames_energy[i] << "," << frames_judge[i] << ") ";
			}
			std::cout << std::endl;
		}
		fclose(wavfp);
	}

	fclose(fp);
	return 0;
}

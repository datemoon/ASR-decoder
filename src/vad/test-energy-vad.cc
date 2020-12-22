#include "src/vad/energy-vad.h"
#include <iostream>
#include <vector>
#include <string.h>
#include <assert.h>

#ifdef NAMESPACE
using namespace datemoon;
#endif


int main(int argc, char *argv[])
{
	if(argc != 3)
	{
		std::cerr << argv[0] << " wavlist head_length" << std::endl;
		return -1;
	}
	char *wavlist = argv[1];
	int head_length = atoi(argv[2]);

	//EnergyVad energy_vad(16000, 0.025, 0.01, 5, 5, 10, 10*2, "short", "sum_square_root");
	//EnergyVad energy_vad(16000, 0.025, 0.01, 0.3, 10, 10, 32768*0.005, 32768*0.05, "short",  "sum_abs");
	EnergyVad<short> energy_vad(16000, 0.025, 0.01, -1, EnergyVad<short>::SIL, 0.5, 0.8, 20, 20, 32768*0.01, 32768*0.01, "sum_square_root");
	FILE* fp = fopen(wavlist, "rb");
	if(fp == NULL)
	{
		std::cerr << "fopen " << wavlist << " failed!!!" <<std::endl;
		return -1;
	}
	while(1)
	{
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
			
				EnergyVad<short>::VadSeg tot_vad_res = energy_vad.CompressVadRes(frames_judge);
				energy_vad.Print(tot_vad_res);
/*
				EnergyVad<short>::VadSeg vad_res = energy_vad.CompressVadRes(cur_frames_judge);
				while(energy_vad.GetData(out_data, getdata_audio_flag) != 0)
				{
					std::cout << "out_data size is " << out_data.size() << std::endl;;
				}
*/
				break;
			}
			std::vector<short> v_data(data, data+read_len);
			cur_frames_energy = energy_vad.FramesEnergy(v_data);
			frames_energy.insert(frames_energy.end(),
						cur_frames_energy.begin(), cur_frames_energy.end());
			cur_frames_judge = energy_vad.JudgeFramesFromEnergy(cur_frames_energy,0);
			frames_judge.insert(frames_judge.end(),
				   	cur_frames_judge.begin(), cur_frames_judge.end());

			/*
			EnergyVad<short>::VadSeg vad_res = energy_vad.CompressVadRes(cur_frames_judge);
			while(energy_vad.GetData(out_data, getdata_audio_flag) != 0)
			{
				std::cout << "out_data size is " << out_data.size() << std::endl;;
			}
			*/
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

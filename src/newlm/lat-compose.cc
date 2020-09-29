
#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include "src/newfst/compose-lat.h"
#include "src/newlm/compose-arpalm.h"
#include "src/newfst/lattice-functions.h"
#include "src/newfst/lattice-to-nbest.h"
#include "src/decoder/wordid-to-wordstr.h"

#ifdef NAMESPACE
using namespace datemoon;
#endif
void PrintNbest(Lattice &lat, WordSymbol &wordsymbol, int n = 5)
{
	Lattice nbest_lat;
	vector<Lattice> nbest_paths;
	NShortestPath(lat, &nbest_lat, n);
	ConvertNbestToVector(nbest_lat, &nbest_paths);
	for(unsigned i = 0; i < nbest_paths.size(); ++i)
	{
		vector<int> nbest_words_arr;
		vector<int> nbest_phones_arr;
		float nbest_tot_score=0 ,nbest_lm_score=0;
		if(LatticeToVector(nbest_paths[i], nbest_words_arr, nbest_phones_arr, nbest_tot_score, nbest_lm_score))
		{
			for(unsigned j = 0; j < nbest_words_arr.size(); ++j)
			{
				printf("%s ",wordsymbol.FindWordStr(nbest_words_arr[j]));
			}
			printf("%f %f\n",nbest_tot_score,nbest_lm_score);
		}			
	}
}
int main(int argc, char *argv[])
{
	if(argc != 6)
	{
		std::cerr << argv[0] << " arpafile wordlist scale latfile latout" << std::endl;
		return -1;
	}
	std::string fsafile = argv[1];
	std::string wordlist = argv[2];
	float scale = atof(argv[3]);
	std::string latfile = argv[4];
	std::string latoutfile = argv[5];
	ArpaLm arpalmscore;
	WordSymbol wordsymbol;
	if(0 != wordsymbol.ReadText(wordlist.c_str()))
	{
		LOG_WARN << "read wordlist " << wordlist << " failed!!!";
		return -1;
	}
	if(arpalmscore.Read(fsafile.c_str()) != true)
	{
		std::cerr << "read fsa failed." << std::endl;
		return -1;
	}
	ComposeArpaLm composearpalm(static_cast<ArpaLm*>(&arpalmscore));

	FILE *fp = fopen(latfile.c_str(), "rb");
	FILE *fpout = fopen(latoutfile.c_str(), "wb");
	Lattice lat;
	struct timeval start,end;
	float tot_time = 0.0;
	int nlat = 0;
	while(lat.Read(fp))
	{
		nlat++;
		Lattice olat,olat_tmp;

		gettimeofday(&start,NULL);
		ComposeLattice<FsaStateId>( &lat,
				static_cast<LatticeComposeItf<FsaStateId>* >(&composearpalm),
				&olat, scale);
		gettimeofday(&end,NULL);
		tot_time += end.tv_sec-start.tv_sec +
			(end.tv_usec - start.tv_usec)*1.0/(1000*1000);

		PrintNbest(olat, wordsymbol, 10);
		olat.Write(fpout);
	}
	std::cout << "total time is : " << tot_time << " s " <<
		" rt is : " << tot_time/nlat << std::endl;
		
	fclose(fp);
	fclose(fpout);
	return 0;
}


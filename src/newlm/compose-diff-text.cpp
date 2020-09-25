
#include <iostream>
#include <fstream>
#include <string>
#include "src/newfst/compose-lat.h"
#include "src/newlm/compose-arpalm.h"
#include "src/newlm/diff-lm.h"

#ifdef NAMESPACE
using namespace datemoon;
#endif
int main(int argc, char *argv[])
{
	if(argc != 5)
	{
		std::cerr << argv[0] << " arpafile1 arpafile2 wordlist inputfile" << std::endl;
		return -1;
	}
	std::string fsafile1 = argv[1];
	std::string fsafile2 = argv[2];

	std::string wordlist = argv[3];
	std::string textfile = argv[4];
	ArpaLm arpalm1,arpalm2;
	if(arpalm1.Read(fsafile1.c_str()) != true)
	{
		std::cerr << "read arpafile1 failed." << fsafile1 << std::endl;
		return -1;
	}
	if(arpalm2.Read(fsafile2.c_str()) != true)
	{
		std::cerr << "read arpafile1 failed." << fsafile2 << std::endl;
		return -1;
	}

	arpalm1.Rescale(-1.0);
	DiffArpaLm diff_lm(&arpalm1, &arpalm2);

	ArpaLmScore *arpalmscore = new ArpaLmScore();
	arpalmscore->ReadSymbols(wordlist.c_str());

	std::fstream fp(textfile);
	std::string line;
	while(getline(fp,line))
	{
		Lattice lat, olat;
		arpalmscore->ConvertText2Lat((char*)line.c_str(), &lat);

		ComposeLattice<FsaStateId>( &lat,
				static_cast<LatticeComposeItf<FsaStateId>* >(&diff_lm),
				&olat);
		olat.Print();
	}
	delete arpalmscore;
	return 0;
}

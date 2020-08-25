
#include <iostream>
#include <fstream>
#include <string>
#include "src/newfst/compose-lat.h"
#include "src/newlm/compose-arpalm.h"

#ifdef NAMESPACE
using namespace datemoon;
#endif
int main(int argc, char *argv[])
{
	if(argc != 4)
	{
		std::cerr << argv[0] << " arpafile wordlist inputfile" << std::endl;
		return -1;
	}
	std::string fsafile = argv[1];
	std::string wordlist = argv[2];
	std::string textfile = argv[3];
	ArpaLmScore *arpalmscore = new ArpaLmScore();
	if(arpalmscore->Read(fsafile.c_str()) != true)
	{
		std::cerr << "read fsa failed." << std::endl;
		return -1;
	}
	arpalmscore->ReadSymbols(wordlist.c_str());
	ComposeArpaLm *composearpalm = new ComposeArpaLm(static_cast<ArpaLm*>(arpalmscore));

	std::fstream fp(textfile);
	std::string line;
	while(getline(fp,line))
	{
		Lattice lat, olat;
		arpalmscore->ConvertText2Lat((char*)line.c_str(), &lat);

		ComposeLattice<FsaStateId>( &lat,
				static_cast<LatticeComposeItf<FsaStateId>* >(composearpalm),
				&olat);
		olat.Print();
	}
	delete arpalmscore;
	delete composearpalm;
	return 0;
}

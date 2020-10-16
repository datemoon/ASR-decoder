
#include <iostream>
#include <fstream>
#include "src/hmm/hmm-topology.h"
#ifdef NAMESPACE
using namespace datemoon ;
#endif


int main(int argc,char *argv[])
{
	std::ifstream ifs;
	bool binary = false;
	ifs.open(argv[1],
			binary ? std::ios_base::in | std::ios_base::binary
			: std::ios_base::in);

	std::string str;
	ifs >> str;
	HmmTopology hmm;

	if(ifs.is_open())
	{
		hmm.Read(ifs, binary);
	}
	else
	{
		std::cerr << "hmm file open fail." << std::endl;
	}

	return 0;
}

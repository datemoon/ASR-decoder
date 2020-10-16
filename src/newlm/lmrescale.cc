#include <iostream>
#include <fstream>
#include <string>
#include "src/newlm/arpa2fsa.h"
#ifdef NAMESPACE
using namespace datemoon;
#endif

int main(int argc, char *argv[])
{
	if(argc != 4)
	{
		std::cerr << argv[0] << " inarpafile rescale scalearpafile" << std::endl;
		return -1;
	}
	std::string fsafile = argv[1];
	float scale = atof(argv[2]);
	std::string scalefsa = argv[3];
	ArpaLm arpalm;
	if(arpalm.Read(fsafile.c_str()) != true)
	{
		std::cerr << "read fsa failed." << std::endl;
		return -1;
	}
	arpalm.Rescale(scale);
	return 0;
}

#include <iostream>
#include "src/newlm/arpa2fsa.h"

#ifdef NAMESPACE
using namespace datemoon;
#endif

int main(int argc,char *argv[])
{
	if(argc != 4)
	{
		std::cerr << argv[0] << " arpafile wordlist outputfile" << std::endl;
		return -1;
	}
	std::string arpafile = argv[1];
	std::string wordlist = argv[2];
	std::string outputfile = argv[3];
	Arpa2Fsa *arpa2fsa = new Arpa2Fsa(1, arpafile, wordlist);

	if(arpa2fsa->ConvertArpa2Fsa() != true)
	{
		std::cerr << "Convert arpa to fsa failed."<< std::endl;
		return -1;
	}
	if(arpa2fsa->Write(outputfile.c_str()) != true)
	{
		std::cerr << "Write " + outputfile + " failed." << std::endl;
		return -1;
	}
	return 0;
}

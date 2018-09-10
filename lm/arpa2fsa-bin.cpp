#include <iostream>
#include "lm/arpa2fsa.h"


int main(int argc,char *argv[])
{
	if(argc != 3)
	{
		std::cerr << argv[0] << " arpafile wordlist." << std::endl;
		return -1;
	}
	std::string arpafile = argv[1];
	std::string wordlist = argv[2];
	Arpa2Fsa *arpa2fsa = new Arpa2Fsa(1, arpafile, wordlist);

	if(arpa2fsa->ConvertArpa2Fsa() != true)
	{
		std::cerr << "Convert arpa to fsa failed."<< std::endl;
		return -1;
	}
	return 0;
}

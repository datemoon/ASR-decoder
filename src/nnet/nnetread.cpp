#include <iostream>
#include "src/nnet/nnet-nnet.h"


#ifdef NAMESPACE
using namespace datemoon;
#endif
int main(int argc,char *argv[])
{
	Nnet *nnet = new Nnet;
	
//#define BINARY
#ifdef BINARY
	if(false == nnet->ReadNnet(argv[1]))
	{
		fprintf(stderr,"read nnet %s error.\n",argv[1]);
		return -1;
	}
#else
	nnet->ReadNnet(argv[1],true);
	nnet->ReadNnet(argv[2],true);
	nnet->ReadNnet(argv[3],true);
	if(false == nnet->WriteNnet(argv[4]))
		std::cout << "write nnet error!" << std::endl;
#endif
	delete nnet;
	return 0;
}

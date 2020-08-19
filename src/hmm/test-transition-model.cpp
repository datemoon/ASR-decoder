
#include <iostream>
#include <fstream>
#include "src/hmm/transition-model.h"
#ifdef NAMESPACE
using namespace datemoon ;
#endif


int main(int argc,char *argv[])
{
	std::ifstream ifs;
	std::ofstream ofs;
	bool binary = false;
	ifs.open(argv[1],
			binary ? std::ios_base::in | std::ios_base::binary
			: std::ios_base::in);

	ofs.open(argv[2],
			binary ? std::ios_base::out | std::ios_base::binary
			: std::ios_base::out);

	TransitionModel trans_model;

	if(ifs.is_open())
	{
		trans_model.Read(ifs, binary);
	}
	else
	{
		std::cerr << "trainsmodel file open fail. " << argv[1] << std::endl;
	}

	// test TransitionIdToPdf
	for(int i=1;i<trans_model.NumTransitionIds()+1;++i)
	{
		std::cerr << "trans_id " << i << ": " << "pdf " 
			<< trans_model.TransitionIdToPdf(i) << std::endl;
	}
	trans_model.ExchangePdfId();
	if(ofs.is_open())
	{
		trans_model.Write(ofs,binary);
	}
	else
	{
		std::cerr << "trainsmodel file write fail. " << argv[2] << std::endl;
	}
	return 0;
}

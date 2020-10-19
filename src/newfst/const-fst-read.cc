
#include <iostream>
#include "src/newfst/const-fst.h"
#include "src/newfst/optimize-fst.h"
#include "src/newfst/arc.h"

#ifdef NAMESPACE
using namespace datemoon;
#endif

int main(int argc, char *argv[])
{
	if(argc != 2)
	{
		std::cout << argv[0] << " constfst." << std::endl;
		return -1;
	}
	std::string constfstfile(argv[1]);

	ConstFst<StdArc, int> constfst;
	if(constfst.Read(constfstfile) != true)
	{
		std::cerr << "read " << constfstfile << " failed!!!" << std::endl;
		return -1;
	}

	Fst fst(constfst);

	fst.PrintFst();

	return 0;
}

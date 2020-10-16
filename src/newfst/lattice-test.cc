#include <iostream>
#include <string>
//#include "src/newfst/optimize-fst.h"
#include "src/newfst/connect-fst.h"

#ifdef NAMESPACE
using namespace datemoon;
#endif


int main(int argc,char *argv[])
{
	std::string file("test.lat");
	Lattice lat;
	lat.SetStart(0);
	for(int i=0;i<6;++i)
	   	lat.AddState();
	LatticeArc arc1(1,0,1,LatticeWeight(0.5,0));
	LatticeArc arc2(2,0,2,LatticeWeight(0.5,0));
	LatticeArc arc3(3,0,3,LatticeWeight(0.5,0));
	LatticeArc arc4(4,0,4,LatticeWeight(0.5,0));
	LatticeArc arc5(5,0,5,LatticeWeight(0.5,0));
	LatticeArc arc6(6,0,3,LatticeWeight(0.5,0));
	lat.AddArc(0,arc1);
	lat.AddArc(1,arc2);
	lat.AddArc(1,arc3);
	lat.AddArc(1,arc4);
	lat.AddArc(2,arc5);
	lat.AddArc(4,arc6);
	lat.SetFinal(3);

	std::cout << "old fst" << std::endl;
	lat.Print();
	if(lat.Write(file) != true)
	{
		std::cout << "Write lat " << file << " error." << std::endl;
		return 1;
	}
	if(lat.Read(file) != true)
	{
		std::cout << "Read lat " << file << " error." << std::endl;
		return 1;
	}
	std::cout << "write and read" << std::endl;
	lat.Print();
	Connect(&lat);
	std::cout << "new fst" << std::endl;
	lat.Print();
	return 0;
}

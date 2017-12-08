#include <iostream>

#include "decoder/optimize-fst.h"
#include "fst/connect-fst.h"



int main(int argc,char *argv[])
{
	Lattice lat;
	lat.SetStart(0);
	for(int i=0;i<6;++i)
	   	lat.AddState();
	Arc arc1(1,0,1,0.5);
	Arc arc2(2,0,2,0.5);
	Arc arc3(3,0,3,0.5);
	Arc arc4(4,0,4,0.5);
	Arc arc5(5,0,5,0.5);
	Arc arc6(6,0,3,0.5);
	lat.AddArc(0,arc1);
	lat.AddArc(1,arc2);
	lat.AddArc(1,arc3);
	lat.AddArc(1,arc4);
	lat.AddArc(2,arc5);
	lat.AddArc(4,arc6);
	lat.SetFinal(3);

	std::cout << "old fst" << std::endl;
	lat.Print();
	Connect(&lat);
	std::cout << "new fst" << std::endl;
	lat.Print();
	return 0;
}

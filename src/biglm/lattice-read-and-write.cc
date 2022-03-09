#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <iostream>
#include "lat/kaldi-lattice.h"


using namespace kaldi;
using namespace fst;
int main(int argc, char *argv[])
{
	bool binary=false;
	std::string infile(argv[1]);

	std::ifstream ifs;

	std::ostringstream oss;
	std::ostream os(oss.rdbuf());

	ifs.open(infile.c_str(), 
			binary ? std::ios_base::in | std::ios_base::binary
			: std::ios_base::in);
	if(!ifs.is_open())
	{
		std::cerr << "Open " << infile << " failed." << std::endl;
		return -1;
	}
	CompactLattice *clat=NULL;
	bool read_ret = ReadCompactLattice(ifs, binary, &clat);
	if(read_ret != true)
	{
		std::cerr << "ReadCompactLattice failed." << std::endl; 
		return -1;
	}

	bool w_binary=true;
	bool write_ret = WriteCompactLattice(os, w_binary, *clat);
	if(write_ret != true)
	{
		std::cerr << "WriteCompactLattice failed." << std::endl;
		return -1;
	}
	delete clat;
	if(w_binary != true)
	{
		std::cout << oss.str() <<std::endl;
	}
	std::string clatstr(oss.str());
	std::cout << "WriteCompactLattice ok." << std::endl;
	// 拷贝内存到iss
	std::istringstream iss(oss.str());
	std::istream is(iss.rdbuf());
	CompactLattice *clat1=NULL;
	read_ret = ReadCompactLattice(is, w_binary, &clat1);
	if(read_ret != true)
	{
		std::cerr << "ReadCompactLattice istream failed." << std::endl;
		return -1;
	}
	std::cout << "ReadCompactLattice from istringstream to CompactLattice ok" << std::endl;
	if(w_binary != true)
	{
		std::cout << iss.str() << std::endl;
	}
	delete clat1;
	return 0;
}

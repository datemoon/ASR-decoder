#include "text-util.h"
#include <string>
#include <vector>
#include <iostream>

#ifdef NAMESPACE
using namespace datemoon;
#endif
int main(int argc,char *argv[])
{
	std::string line("hello.  \n  today is good day.   \n");
	std::vector<std::string> str;

	CutLineToStr(line, str);
	for(unsigned i=0;i<str.size();++i)
	{
		std::cout << str[i] << std::endl;
	}
	//std::cout << '\n' << std::endl;
	return 0;
}

#include <iostream>
#include <string>
#include "config-parse-options.h"

using namespace std;
#ifdef NAMESPACE
using namespace datemoon;
#endif
int main(int argc,char *argv[])
{
	bool binary = true;
	int integer = 0;
	unsigned uinteger = 0;
	float fl = 0.0;
	string str("yes");
	double db = 0.0;

	const char *usage = "This is a test code\n";
	ConfigParseOptions conf(usage);
	conf.Register("binary",&binary,"default is true");
	conf.Register("integer",&integer,"default is 0");
	conf.Register("uinteger",&uinteger,"default is 0");
	conf.Register("fl",&fl,"default is 0.0");
	conf.Register("str",&str,"default is yes");
	conf.Register("db",&db,"default is 0.0");

	conf.Read(argc, argv);
	conf.PrintUsage();
	std::cout << "--binary=" << (binary ? "true" : "false") 
		<< "\n--integer=" << integer 
		<< "\n--uinteger=" << uinteger 
		<< "\n--fl=" << fl 
		<< "\n--str=" << str 
		<< "\n--db=" << db <<std::endl;
	return 0;
}

#include <iostream>
#include <unordered_map>
#include "util/hash-key.h"

int main(int argc,char*argv[])
{
	typedef std::unordered_map<const std::string, int, StringKey, StringEqual> WordHash;
	WordHash word_dict;
	word_dict["abc"] = 1;
	word_dict["bcd"] = 2;
	std::cout << word_dict["abc"] << " " << word_dict["bcd"] << std::endl;
	return 0;
}

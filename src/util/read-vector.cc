#include <stdlib.h>
#include <iostream>
#include "src/util/read-vector.h"
#include "src/util/io-funcs.h"
#include "src/util/log-message.h"

#include "src/util/namespace-start.h"

void ReadVector(std::istream &is, std::vector<float> &vec, bool binary)
{
	int pos_at_start = is.tellg();
	if(is.fail())
	{
		LOG_ERR << "Failed to read token [started at file position "
			<< pos_at_start << "]" << std::endl;
	}
	if(!binary)
		is >> std::ws; // consume whitespace.
	ExpectToken(is, binary, "[");
	while(1)
	{
		std::string str;
		ReadToken(is, binary, &str);
		if(str == "]")
			break;
		vec.push_back(atof(str.c_str()));
	}
}
#include "src/util/namespace-end.h"


#include <stdio.h>
#include <string>
#include <iostream>
#include "text-util.h"

#include "src/util/namespace-start.h"

bool ConvertStringToInteger(const std::string &str, int *out)
{
	sscanf(str.c_str(), "%d",out);
	return true;
}

void TrimString(std::string &str)
{
	const char *white_chars = " \t\n\r\f\v";

	std::string::size_type pos = str.find_last_not_of(white_chars);
	if (pos != std::string::npos)  
	{
		str.erase(pos + 1);
		pos = str.find_first_not_of(white_chars);
		if (pos != std::string::npos) str.erase(0, pos);
	} 
	else 
	{
		str.erase(str.begin(), str.end());
	}

	/*
	size_t i=0,j=0;
	size_t len = str.length();
	char *s = (char*)str.c_str();
	for(i=0,j=0; i < len;++i)
	{
		switch(s[i])
		{
			case ' ':
			case '\t':
			case '\n':
			case '\r':
			case '\f':
			case '\v':
				break;
			default:
				s[j] = s[i];
				++j;
				break;
		}
	}
	s[j] = '\0';
	*/
}

void CutLineToStr(std::string line,std::vector<std::string> &str)//, char ch)
{
	str.clear();
	size_t i=0,j=0;
	size_t len = line.length();
	char *s = (char*)line.c_str();
	int k = 0;
	for(i=0,j=0;i<len;++i)
	{
		switch(s[i])
		{
			case ' ':
			case '\t':
			case '\n':
			case '\r':
			case '\f':
			case '\v':
//			case ch:
				s[i] = '\0';
				if(j==0)
					break;
				//std::string astr((const char *)(s));
				//astr = (char *)(s+i-j);
				str.push_back(s+i-j);
				++k;
				j=0;
				break;
			default:
				++j;
				break;
		}
	}
	// add the last word
	if(0 != j)
	{
		str.push_back(s+i-j);
	}
}
#include "src/util/namespace-end.h"

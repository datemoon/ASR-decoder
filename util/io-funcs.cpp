
#include <iostream>
#include <istream>
#include <ostream>
#include <assert.h>
#include <string.h>
#include <string>
#include <stdexcept>
#include "io-funcs.h"

bool ReadWordList(std::string file, std::vector<std::string> &wordlist, bool binary)
{
	std::ifstream ifs;
//	binary = false;
	ifs.open(file.c_str(),
			binary ? std::ios_base::in | std::ios_base::binary
			: std::ios_base::in);
	if(!ifs.is_open())
	{
		return false;
	}

	wordlist.clear();
	int wordid;
	std::string word;
	while(EOF != ifs.peek())
	{
		ifs >> word;
		ifs >> wordid;
//		std::cout << word << " " <<  wordid << std::endl;
		while(wordid >= (int)(wordlist.size()))
		{
			wordlist.push_back("");
		}
		wordlist[wordid] = word;
		ifs.get();
	}
	ifs.close();
	return true;
}

void ExpectToken(std::istream &is, bool binary, const char *token)
{
	int pos_at_start = is.tellg();
	assert(NULL != token);
	if(!binary)
		is >> std::ws; // consume whitespace.
	std::string str;

	is >> str;
	is.get(); // consume the space
	if(is.fail())
	{
		std::cerr << "Failed to read token [started at file position "
			<< pos_at_start << "], expected " << token << std::endl;
	}
	if(strcmp(str.c_str(), token) != 0)
	{
		std::cerr << "Expected token \"" << token << "\", got instead \""
			<< str <<"\"." << std::endl;
	}
}

void ExpectToken(std::istream &is, bool binary, const std::string &token) 
{
	ExpectToken(is, binary, token.c_str());
}

void ReadToken(std::istream &is, bool binary, std::string *str) 
{
	assert(str != NULL);
	if (!binary) is >> std::ws;  // consume whitespace.
	is >> *str;
	if (is.fail()) 
	{
		std::cerr << "ReadToken, failed to read token at file position "
			<< is.tellg() << std::endl;
	}
	if (!isspace(is.peek())) 
	{
		std::cerr << "ReadToken, expected space after token, saw instead "
			<< static_cast<char>(is.peek())
			<< ", at file position " << is.tellg() << std::endl;
	}
	is.get();  // consume the space.
}

void CheckToken(const char *token) 
{
	if (*token == '\0')
		std::cerr << "Token is empty (not a valid token)" << std::endl;
	const char *orig_token = token;
	while (*token != '\0') {
		if (::isspace(*token))
			std::cerr << "Token is not a valid token (contains space): '"
				<< orig_token << "'" << std::endl;
		token++;
	}
}
void WriteToken(std::ostream &os, bool binary, const char *token)
{
	// binary mode is ignored;
	// we use space as termination character in either case.
	assert(token != NULL);
	CheckToken(token);  // make sure it's valid (can be read back)
	os << token << " ";
	if (os.fail())
   	{
		throw std::runtime_error("Write failure in WriteToken.");
	}
}

template<> void WriteBasicType<float>(std::ostream &os, bool binary, float f)
{
	if (binary)
	{
		char c = sizeof(f);
		os.put(c);
		os.write(reinterpret_cast<const char *>(&f), sizeof(f));
	}
	else
	{
		os << f << " ";
	}
}

template<> void WriteBasicType<int>(std::ostream &os, bool binary, int f)
{
	if (binary)
	{
		char c = sizeof(f);
		os.put(c);
		os.write(reinterpret_cast<const char *>(&f), sizeof(f));
	}
	else
	{
		os << f << " ";
	}
}

template<> void ReadBasicType<float>(std::istream &is, bool binary, float *f) 
{
	assert(f != NULL);
	if (binary) 
	{
		double d;
		int c = is.peek();
		if (c == sizeof(*f)) 
		{
			is.get();
			is.read(reinterpret_cast<char*>(f), sizeof(*f));
		} 
		else if (c == sizeof(d)) 
		{
			ReadBasicType(is, binary, &d);
			*f = d;
		} 
		else 
		{
			std::cerr << "ReadBasicType: expected float, saw " << is.peek()
				<< ", at file position " << is.tellg() << std::endl;
		}
	} 
	else 
	{
		is >> *f;
	}
	if (is.fail()) 
	{
		std::cerr << "ReadBasicType: failed to read, at file position "
			<< is.tellg() << std::endl;
	}
}

template<> void ReadBasicType<double>(std::istream &is, bool binary, double *f) 
{
	assert(f != NULL);
	if (binary) 
	{
		double d;
		int c = is.peek();
		if (c == sizeof(*f)) 
		{
			is.get();
			is.read(reinterpret_cast<char*>(f), sizeof(*f));
		} 
		else if (c == sizeof(d)) 
		{
			ReadBasicType(is, binary, &d);
			*f = d;
		} 
		else 
		{
			std::cerr << "ReadBasicType: expected float, saw " << is.peek()
				<< ", at file position " << is.tellg() << std::endl;
		}
	} 
	else 
	{
		is >> *f;
	}
	if (is.fail()) 
	{
		std::cerr << "ReadBasicType: failed to read, at file position "
			<< is.tellg() << std::endl;
	}
}


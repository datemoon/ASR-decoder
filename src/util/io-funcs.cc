
#include <iostream>
#include <istream>
#include <ostream>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <string>
#include <stdexcept>
#include <unordered_map>

#include "src/util/io-funcs.h"
#include "src/util/log-message.h"
#include "src/util/util-type.h"
#include "src/util/text-util.h"

#include "src/util/namespace-start.h"

// load lexicon , format:
//   word phone1 ... phonen
//   ...
bool ReadStringVector(std::string &lexiconfile, 
		std::vector<std::vector<std::string> > &readvector,  bool binary)
{
	std::ifstream ifs;
	ifs.open(lexiconfile.c_str(),
			binary ? std::ios_base::in | std::ios_base::binary
			: std::ios_base::in);
	if(!ifs.is_open())
	{
		return false;
	}
	std::string line;
	std::vector<std::string> token_list;
	typedef std::unordered_map<std::string, int>::iterator IterType;
	while(getline(ifs, line))
	{
		token_list.clear();
		CutLineToStr(line, token_list);
		readvector.push_back(std::move(token_list));
	}
	return true;
}


bool ReadWordList(std::string &file, std::vector<std::string> &wordlist, bool binary)
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
//		LOG_COM << word << " " <<  wordid << std::endl;
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

template<typename T=int>
bool ReadWordMap(std::string &file, std::unordered_map<std::string, T> &word_map, bool binary)
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

	word_map.clear();
	T wordid;
	std::string word;
	while(EOF != ifs.peek())
	{
		ifs >> word;
		ifs >> wordid;
		word_map.insert(std::make_pair(std::move(word), wordid));
		ifs.get();
	}
	ifs.close();
	return true;
}

template bool ReadWordMap<int>(std::string &file, std::unordered_map<std::string, int>&word_map, bool binary);

void ExpectToken(std::istream &is, bool binary, const char *token)
{
	int pos_at_start = is.tellg();
	LOG_ASSERT(NULL != token);
	if(!binary)
		is >> std::ws; // consume whitespace.
	std::string str;

	is >> str;
	is.get(); // consume the space
	if(is.fail())
	{
		LOG_ERR << "Failed to read token [started at file position "
			<< pos_at_start << "], expected " << token << std::endl;
	}
	if(strcmp(str.c_str(), token) != 0)
	{
		LOG_ERR << "Expected token \"" << token << "\", got instead \""
			<< str <<"\"." << std::endl;
	}
}

void ExpectToken(std::istream &is, bool binary, const std::string &token) 
{
	ExpectToken(is, binary, token.c_str());
}

void ReadToken(std::istream &is, bool binary, std::string *str) 
{
	LOG_ASSERT(str != NULL);
	if (!binary) is >> std::ws;  // consume whitespace.
	is >> *str;
	if (is.fail()) 
	{
		LOG_ERR << "ReadToken, failed to read token at file position "
			<< is.tellg() << std::endl;
	}
	if (!isspace(is.peek())) 
	{
		LOG_ERR << "ReadToken, expected space after token, saw instead "
			<< static_cast<char>(is.peek())
			<< ", at file position " << is.tellg() << std::endl;
	}
	is.get();  // consume the space.
}

void CheckToken(const char *token) 
{
	if (*token == '\0')
		LOG_ERR << "Token is empty (not a valid token)" << std::endl;
	const char *orig_token = token;
	while (*token != '\0') {
		if (::isspace(*token))
			LOG_ERR << "Token is not a valid token (contains space): '"
				<< orig_token << "'" << std::endl;
		token++;
	}
}
void WriteToken(std::ostream &os, bool binary, const char *token)
{
	// binary mode is ignored;
	// we use space as termination character in either case.
	LOG_ASSERT(token != NULL);
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
	LOG_ASSERT(f != NULL);
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
			LOG_ERR << "ReadBasicType: expected float, saw " << is.peek()
				<< ", at file position " << is.tellg() << std::endl;
		}
	} 
	else 
	{
		is >> *f;
	}
	if (is.fail()) 
	{
		LOG_ERR << "ReadBasicType: failed to read, at file position "
			<< is.tellg() << std::endl;
	}
}

template<> void ReadBasicType<double>(std::istream &is, bool binary, double *f) 
{
	LOG_ASSERT(f != NULL);
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
			LOG_ERR << "ReadBasicType: expected float, saw " << is.peek()
				<< ", at file position " << is.tellg() << std::endl;
		}
	} 
	else 
	{
		is >> *f;
	}
	if (is.fail()) 
	{
		LOG_ERR << "ReadBasicType: failed to read, at file position "
			<< is.tellg() << std::endl;
	}
}

ssize_t ReadN(int fd, void *vptr, ssize_t n)
{
	ssize_t nr = 0;
	char *ptr = static_cast<char*>(vptr);
	ssize_t nleft = n;
	while(nleft > 0)
	{
		nr = read(fd, ptr, nleft);
		if(nr < 0)
		{
			if(errno == EINTR)
				continue; // call read again
			else
				return -1;
		}
		else if(nr == 0)
			break; // end fd

		nleft -= nr;
		ptr += nr;
	}
	return n-nleft;
}

ssize_t WriteN(int fd, const void *vptr, ssize_t n)
{
	ssize_t nw = 0;
	const char *ptr = static_cast<const char*>(vptr);
	ssize_t nleft = n;
	while(nleft > 0)
	{
		nw = write(fd, ptr, nleft);
		if(nw <= 0)
		{
			if(nw < 0 && errno == EINTR)
				continue; // call read again
			else
				return -1;
		}
		nleft -= nw;
		ptr += nw;
	}
	return n-nleft;
}

/*
inline std::istream &ReadType(std::istream &strm, std::string *s)
{
	s->clear();
	int32 ns = 0;
	strm.read(reinterpret_cast<char *>(&ns), sizeof(ns));
	for (int32 i = 0; i < ns; ++i) 
	{
		char c;
		strm.read(&c, 1);
		*s += c;
	}
	return strm;
}

template <class T, typename std::enable_if<std::is_class<T>::value, T>::type* = nullptr>
inline std::istream &ReadType(std::istream &strm, T *t)
{
	return t->Read(strm);
}

template <class T, typename std::enable_if<std::is_arithmetic<T>::value, T>::type* = nullptr>
inline std::istream &ReadType(std::istream &strm, T *t) 
{
	return strm.read(reinterpret_cast<char *>(t), sizeof(T)); 
}

// Generic case.
template <class T, typename std::enable_if<std::is_class<T>::value, T>::type* = nullptr>
inline std::ostream &WriteType(std::ostream &strm, const T t) 
{
	t.Write(strm);
	return strm;
}

// Numeric (boolean, integral, floating-point) case.
template <class T, typename std::enable_if<std::is_arithmetic<T>::value, T>::type* = nullptr>
inline std::ostream &WriteType(std::ostream &strm, const T t) 
{
	return strm.write(reinterpret_cast<const char *>(&t), sizeof(T));
}

// String case.
inline std::ostream &WriteType(std::ostream &strm, const std::string &s) 
{  // NOLINT
	int32 ns = s.size();
	strm.write(reinterpret_cast<const char *>(&ns), sizeof(ns));
	return strm.write(s.data(), ns);
}
*/

#include "src/util/namespace-end.h"

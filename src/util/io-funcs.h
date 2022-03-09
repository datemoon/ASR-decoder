#ifndef __IO_FUNCS_H__
#define __IO_FUNCS_H__
#include <string>
#include <istream>
#include <ostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <cassert>
#include <type_traits>
#include <unordered_map>

#include <list>
#include <map>
#include <set>
#include <unordered_set>
#include <utility>

#include "src/util/util-type.h"

#include "src/util/namespace-start.h"


bool ReadStringVector(std::string &lexiconfile, 
		std::vector<std::vector<std::string> > &readvector,  bool binary=false);
// input file:
//     word  wordid
//     ....
bool ReadWordList(std::string &file, std::vector<std::string> &wordlist, bool binary=false);

// input file:
//     word  wordid
//     ....
template<typename T=int>
bool ReadWordMap(std::string &file, std::unordered_map<std::string, T> &word_map, bool binary=false);

void ExpectToken(std::istream &is, bool binary, const char *token);

void ExpectToken(std::istream &is, bool binary, const std::string &token);

void ReadToken(std::istream &is, bool binary, std::string *str);

void WriteToken(std::ostream &os, bool binary, const char *token);

// WriteBasicType is the name of the write function for bool, integer types,
// and floating-point types. They all throw on error.
template<class T> void WriteBasicType(std::ostream &os, bool binary, T t);


// Declare specialization for bool.
//template<> void WriteBasicType<bool>(std::ostream &os, bool binary, bool b);

template<> void WriteBasicType<float>(std::ostream &os, bool binary, float f);

template<> void WriteBasicType<int>(std::ostream &os, bool binary, int f);

// ReadBasicType is the name of the read function for bool, integer types,
// and floating-point types. They all throw on error.
template<class T> void ReadBasicType(std::istream &is, bool binary, T *t);

template<> void ReadBasicType<float>(std::istream &is, bool binary, float *f);

template<> void ReadBasicType<double>(std::istream &is, bool binary, double *f);

//template<> void ReadBasicType<int>(std::istream &is, bool binary, int *f);

void CheckToken(const char *token);

ssize_t ReadN(int fd, void *vptr, ssize_t n);

ssize_t WriteN(int fd,const void *vptr, ssize_t n);

// read
// string
// copy from openfst util.h
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

// Declares types that can be read from an input stream.
template <class... T>
std::istream &ReadType(std::istream &strm, std::vector<T...> *c);
template <class... T>
std::istream &ReadType(std::istream &strm, std::list<T...> *c);
template <class... T>
std::istream &ReadType(std::istream &strm, std::set<T...> *c);
template <class... T>
std::istream &ReadType(std::istream &strm, std::map<T...> *c);
template <class... T>
std::istream &ReadType(std::istream &strm, std::unordered_map<T...> *c);
template <class... T>
std::istream &ReadType(std::istream &strm, std::unordered_set<T...> *c);

// Pair case.
template <typename S, typename T>
inline std::istream &ReadType(std::istream &strm, std::pair<S, T> *p) 
{
	ReadType(strm, &p->first);
	ReadType(strm, &p->second);
	return strm;
}

template <typename S, typename T>
inline std::istream &ReadType(std::istream &strm, std::pair<const S, T> *p) 
{
	ReadType(strm, const_cast<S *>(&p->first));
	ReadType(strm, &p->second);
	return strm;
}

namespace internal 
{
template <class C, class ReserveFn>
std::istream &ReadContainerType(std::istream &strm, C *c, ReserveFn reserve) 
{
	c->clear();
	int64 n = 0;
	ReadType(strm, &n);
	reserve(c, n);
	auto insert = std::inserter(*c, c->begin());
	for (int64 i = 0; i < n; ++i) 
	{
		typename C::value_type value;
		ReadType(strm, &value);
		*insert = value;
	}
	return strm;
}
}  // namespace internal

template <class... T>
std::istream &ReadType(std::istream &strm, std::vector<T...> *c) 
{
	return internal::ReadContainerType(
			strm, c, [](decltype(c) v, int n) { v->reserve(n); });
}

template <class... T>
std::istream &ReadType(std::istream &strm, std::list<T...> *c) 
{
	return internal::ReadContainerType(strm, c, [](decltype(c) v, int n) {});
}

template <class... T>
std::istream &ReadType(std::istream &strm, std::set<T...> *c) 
{
	return internal::ReadContainerType(strm, c, [](decltype(c) v, int n) {});
}

template <class... T>
std::istream &ReadType(std::istream &strm, std::map<T...> *c) 
{
	return internal::ReadContainerType(strm, c, [](decltype(c) v, int n) {});
}

template <class... T>
std::istream &ReadType(std::istream &strm, std::unordered_set<T...> *c) 
{
	return internal::ReadContainerType(
			strm, c, [](decltype(c) v, int n) { v->reserve(n); });
}

template <class... T>
std::istream &ReadType(std::istream &strm, std::unordered_map<T...> *c) 
{
	return internal::ReadContainerType(
			strm, c, [](decltype(c) v, int n) { v->reserve(n); });
}

// Writes types to an output stream.
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

// Declares types that can be written to an output stream.

template <typename... T>
std::ostream &WriteType(std::ostream &strm, const std::vector<T...> &c);
template <typename... T>
std::ostream &WriteType(std::ostream &strm, const std::list<T...> &c);
template <typename... T>
std::ostream &WriteType(std::ostream &strm, const std::set<T...> &c);
template <typename... T>
std::ostream &WriteType(std::ostream &strm, const std::map<T...> &c);
template <typename... T>
std::ostream &WriteType(std::ostream &strm, const std::unordered_map<T...> &c);
template <typename... T>
std::ostream &WriteType(std::ostream &strm, const std::unordered_set<T...> &c);

// Pair case.
template <typename S, typename T>
inline std::ostream &WriteType(std::ostream &strm,
		const std::pair<S, T> &p) 
{  // NOLINT
	WriteType(strm, p.first);
	WriteType(strm, p.second);
	return strm;
}
namespace internal 
{
template <class C>
std::ostream &WriteContainer(std::ostream &strm, const C &c) 
{
	const int64 n = c.size();
	WriteType(strm, n);
	for (const auto &e : c) {
		WriteType(strm, e);
	}
	return strm;
}
} // namespace internal

template <typename... T>
std::ostream &WriteType(std::ostream &strm, const std::vector<T...> &c) 
{
	return internal::WriteContainer(strm, c);
}

template <typename... T>
std::ostream &WriteType(std::ostream &strm, const std::list<T...> &c) 
{
	return internal::WriteContainer(strm, c);
}

template <typename... T>
std::ostream &WriteType(std::ostream &strm, const std::set<T...> &c) 
{
	return internal::WriteContainer(strm, c);
}

template <typename... T>
std::ostream &WriteType(std::ostream &strm, const std::map<T...> &c) 
{
	return internal::WriteContainer(strm, c);
}

template <typename... T>
std::ostream &WriteType(std::ostream &strm, const std::unordered_map<T...> &c) 
{
  	return internal::WriteContainer(strm, c);
}

template <typename... T>
std::ostream &WriteType(std::ostream &strm, const std::unordered_set<T...> &c) 
{
	return internal::WriteContainer(strm, c);
}

template <typename T1, typename T2>
bool TypeToStr(const T1 &val1, const T2 &val2, std::string str)
{
	std::ostringstream oss;
	std::ostream &otrm = WriteType(oss, val1);
	if(!otrm)
	{
		return false;
	}
	otrm = WriteType(oss, val2);
	str = oss.str();
	return true;
}

template<typename T1, typename T2>
bool StrToType(const std::string &str, T1 val1, T2 val2)
{
	std::istringstream iss(str);
	std::istream &istrm = ReadType(iss, &val1);
	ReadType(iss, &val2);
	return true;
}

template <typename I, typename S>
std::ostream &WriteIntPairs(std::ostream &strm, 
		const std::vector<std::pair<I, S>> &pairs, 
		bool binary=false)
{
	if(binary == true)
	{
		return WriteType(strm, pairs);
	}
	else
	{
		strm << pairs.size() << " ";
		for (ssize_t n = 0; n < pairs.size(); ++n)
		{
			strm << pairs[n].first << " " << pairs[n].second << " ";
		}
		return strm;
	}
}

template <typename I, typename S>
std::istream &ReadIntPairs(std::istream &strm, 
		std::vector<std::pair<I, S>> *pairs, bool binary=false)
{
	if(binary == true)
	{
		return ReadType(strm, pairs);
	}
	else
	{
		ssize_t n = 0;
		strm >> n ;
		for(ssize_t i=0;i<n;++i)
		{
			I i1;
			S i2;
			strm >> i1 >> i2;
			pairs->push_back(std::make_pair(i1,i2));
		}
		return strm;
	}
}
/////////////////////////////////////////////////
// read text data

template <class T, typename std::enable_if<std::is_class<T>::value, T>::type* = nullptr>
inline std::istream &ImportContainerType(std::istream &strm, T *t)
{
	return t->ImportType(strm);
}

// Numeric (boolean, integral, floating-point) case.
template <class T, typename std::enable_if<std::is_arithmetic<T>::value, T>::type* = nullptr>
inline std::istream &ImportContainerType(std::istream &istrm, T *t) 
{
	istrm >> std::ws;
	return istrm >> *t;
}

// String case.
inline std::istream &ImportContainerType(std::istream &istrm, std::string *s)
{
	istrm >> std::ws;
	s->clear();
	char c;
	while(true)
	{
		if(istrm.peek() == static_cast<int>(']') || // list ,vector, map, set
				istrm.peek() == static_cast<int>(')') || // pair
				istrm.peek() == static_cast<int>(',') ||
				istrm.peek() == static_cast<int>('\0') ||
				istrm.peek() == static_cast<int>('\n') ||
				istrm.eof())
			break;
		istrm.get(c);
		s->push_back(c);
	}
	return istrm;
}

// import types to an container type.
template <typename... T>
std::istream &ImportContainerType(std::istream &istrm, std::vector<T...> *c);
template <typename... T>
std::istream &ImportContainerType(std::istream &istrm, std::list<T...> *c);
template <typename... T>
std::istream &ImportContainerType(std::istream &istrm, std::set<T...> *c);
template <typename... T>
std::istream &ImportContainerType(std::istream &istrm, std::map<T...> *c);
template <typename... T>
std::istream &ImportContainerType(std::istream &istrm, std::unordered_map<T...> *c);
template <typename... T>
std::istream &ImportContainerType(std::istream &istrm, std::unordered_set<T...> *c);

namespace internal
{
template <class C>
std::istream &ImportContainer(std::istream &istrm, C *c)
{
	istrm >> std::ws;
	assert(istrm.peek() == static_cast<int>('['));
	istrm.get();
	auto insert = std::inserter(*c, c->end());
	while(true)
	{
		typename C::value_type elem;
		istrm >> std::ws;
		if(istrm.peek() == static_cast<int>(']'))
		{
			istrm.get();
			break;
		}
		ImportContainerType(istrm, &elem);
		*insert = elem;
		istrm >> std::ws;
		if(istrm.peek() == static_cast<int>(','))
		{
			istrm.get();
		}
	}
	return istrm;
}
}// namespace internal

template <typename S, typename T> 
std::istream &ImportContainerType(std::istream &istrm,
		std::pair<S, T> *p)
{
	istrm >> std::ws;
	assert(istrm.peek() == static_cast<int>('('));
	istrm.get();
	istrm >> std::ws;
	ImportContainerType(istrm, &p->first);
	istrm >> std::ws;
	assert(istrm.peek() == static_cast<int>(','));
	istrm.get();
	istrm >> std::ws;
	ImportContainerType(istrm, &p->second);
	istrm >> std::ws;
	assert(istrm.peek() == static_cast<int>(')'));
	istrm.get();
	return istrm;
}

template <typename S, typename T> 
std::istream &ImportContainerType(std::istream &istrm,
		std::pair<const S, T> *p)
{
	istrm >> std::ws;
	assert(istrm.peek() == static_cast<int>('('));
	istrm.get();
	istrm >> std::ws;
	ImportContainerType(istrm, const_cast<S *>(&p->first));
	istrm >> std::ws;
	assert(istrm.peek() == static_cast<int>(','));
	istrm.get();
	istrm >> std::ws;
	ImportContainerType(istrm, &p->second);
	istrm >> std::ws;
	assert(istrm.peek() == static_cast<int>(')'));
	istrm.get();
	return istrm;
}

template <typename... T>
std::istream &ImportContainerType(std::istream &istrm, std::vector<T...> *c)
{
	return internal::ImportContainer(istrm, c);
}

template <typename... T>
std::istream &ImportContainerType(std::istream &istrm, std::list<T...> *c)
{
	return internal::ImportContainer(istrm, c);
}

template <typename... T>
std::istream &ImportContainerType(std::istream &istrm, std::set<T...> *c)
{
	return internal::ImportContainer(istrm, c);
}

template <typename... T>
std::istream &ImportContainerType(std::istream &istrm, std::map<T...> *c)
{
	return internal::ImportContainer(istrm, c);
}

template <typename... T>
std::istream &ImportContainerType(std::istream &istrm, std::unordered_map<T...> *c)
{
	return internal::ImportContainer(istrm, c);
}

template <typename... T>
std::istream &ImportContainerType(std::istream &istrm, std::unordered_set<T...> *c)
{
	return internal::ImportContainer(istrm, c);
}




// Numeric (boolean, integral, floating-point) case.
template <class T, typename std::enable_if<std::is_arithmetic<T>::value, T>::type* = nullptr>
inline std::ostream &PrintContainerType(std::ostream &strm, const T t) 
{
	return strm << t ;
}

// String case.
inline std::ostream &PrintContainerType(std::ostream &strm, const std::string &s) 
{  // NOLINT
	return strm << s ;
}

// Print types to an output stream.
template <typename... T>
std::ostream &PrintContainerType(std::ostream &strm, const std::vector<T...> &c);
template <typename... T>
std::ostream &PrintContainerType(std::ostream &strm, const std::list<T...> &c);
template <typename... T>
std::ostream &PrintContainerType(std::ostream &strm, const std::set<T...> &c);
template <typename... T>
std::ostream &PrintContainerType(std::ostream &strm, const std::map<T...> &c);
template <typename... T>
std::ostream &PrintContainerType(std::ostream &strm, const std::unordered_map<T...> &c);
template <typename... T>
std::ostream &PrintContainerType(std::ostream &strm, const std::unordered_set<T...> &c);

template <typename S, typename T>
std::ostream &PrintContainerType(std::ostream &strm,
		const std::pair<S, T> &p) 
{  // NOLINT
	strm << "( " ;
	PrintContainerType(strm, p.first);
	strm << ", " ;
	PrintContainerType(strm,p.second);
	strm << " )";
	return strm;
}

namespace internal 
{
template <class C>
std::ostream &PrintContainer(std::ostream &strm, const C &c) 
{
	strm << "[ ";
	for (const auto &e : c) {
		PrintContainerType(strm, e);
		strm << "," ;
	}
	strm << "]\n";
	return strm;
}
} // namespace internal

template <typename... T>
std::ostream &PrintContainerType(std::ostream &strm, const std::vector<T...> &c)
{
	return internal::PrintContainer(strm, c);
}
template <typename... T>
std::ostream &PrintContainerType(std::ostream &strm, const std::list<T...> &c)
{
	return internal::PrintContainer(strm, c);
}
template <typename... T>
std::ostream &PrintContainerType(std::ostream &strm, const std::set<T...> &c)
{
	return internal::PrintContainer(strm, c);
}
template <typename... T>
std::ostream &PrintContainerType(std::ostream &strm, const std::map<T...> &c)
{
	return internal::PrintContainer(strm, c);
}
template <typename... T>
std::ostream &PrintContainerType(std::ostream &strm, const std::unordered_map<T...> &c)
{
	return internal::PrintContainer(strm, c);
}
template <typename... T>
std::ostream &PrintContainerType(std::ostream &strm, const std::unordered_set<T...> &c)
{
	return internal::PrintContainer(strm, c);
}

#include "src/util/namespace-end.h"
#endif

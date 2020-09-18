#ifndef __IO_FUNCS_H__
#define __IO_FUNCS_H__
#include <string>
#include <istream>
#include <ostream>
#include <fstream>
#include <vector>

#include "src/util/namespace-start.h"

bool ReadWordList(std::string file, std::vector<std::string> &wordlist, bool binary=false);

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

#include "src/util/namespace-end.h"
#endif

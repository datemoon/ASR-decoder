#ifndef __FST_IO__
#define __FST_IO__
#include "fst.h"

struct FstHead
{
	char fsttype_[64];                   // E.g. "vector"
	char arctype_[64];                   // E.g. "standard"
	int32 version_;                    // Type version #
	int32 flags_;                      // File format bits
	uint64 properties_;                // FST property bits
	int64 start_;                      // Start state
	int64 numstates_;                  // # of states
	int64 numarcs_;                    // # of arcs
};

int read_fst(const char *filename , struct FstHead *fsthead , struct Fst *fst);

int write_fst(const char *filename,struct Fst *fst);

#endif

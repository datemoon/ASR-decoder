#ifndef __UTIL_STL_UTIL_H__
#define __UTIL_STL_UTIL_H__

#include <utility>

#include "src/util/namespace-start.h"

template<typename Int1, typename Int2 = Int1>
struct PairHasher {  // hashing function for pair<int, long long int>
	size_t operator()(const std::pair<Int1, Int2> &x) const noexcept
	{ // 7853 was chosen at random from a list of primes.
		//return x.first + x.second * 7853;
		return x.first * 7853 + x.second ;
		//return x.first * 389 + x.second * 7853;
	}
	PairHasher() { }
};
template<typename Int1, typename Int2 = Int1>
struct PairCmp
{
	bool operator()(const std::pair<Int1, Int2> &x1, 
			const std::pair<Int1, Int2> &x2) const 
	{
		return x1.first == x2.first && x1.second == x2.second;
	}
	PairCmp() { }
};

#include "src/util/namespace-end.h"

#endif

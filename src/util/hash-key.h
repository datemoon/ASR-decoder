#ifndef __HASH_KEY_H__
#define __HASH_KEY_H__

#include <cstring>

#include "src/util/namespace-start.h"
/* bitmask used to compute hash
 * code modulo maxEntries */
#define hashMask(nbits) (~((~(size_t)0L)<<(nbits)))

class StringKey
{
public:
	inline size_t HashKey(unsigned long key, unsigned maxBits) const 
	{
		return (((key * 1103515245 + 12345) >> (30-maxBits)) & hashMask(maxBits));
	}

	size_t operator ()(const std::string &str) const
	// hashes only the state and string.
	{
		unsigned maxBits = 16;
		unsigned long i = 0;
		const char *key = str.c_str();
		for (; *key; key++)
		{
			i += (i << 3) + *key;
		}
		return HashKey(i, maxBits);
	}
};

class StringEqual
{
public:
	bool operator ()(const std::string &key1, const std::string &key2) const
	{
		return (strcmp(key1.c_str(), key2.c_str()) == 0);
	}
};

#include "src/util/namespace-end.h"

#endif

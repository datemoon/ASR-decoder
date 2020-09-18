#ifndef __HASH_LIST_H__
#define __HASH_LIST_H__

#include <vector>
#include <set>
#include <algorithm>
#include <limits>
#include <cassert>
#include "src/util/log-message.h"

#include "src/util/namespace-start.h"

template<class I, class T> class HashList
{
public:
	struct Elem
	{
		I key;
		T val;
		Elem *tail;
	};

	// Constructor takes no arguments.
	// Call SetSize to inform it of the likely size.
	HashList();

	// Clears the hash and gives the head of the current list to the user;
	// ownership is transferred to the user (the user must call Delete()
	// for each element in the list, at his/her leisure).
	Elem *Clear();

	// Gives the head of the current list to the user. Ownership retained in the
	// class. 
	const Elem *GetList() const;

	// Think of this like delete(). It is to be called for each Elem in turn
	// after you "obtained ownership" by doing Clear(). This is not the opposite
	// of. Insert, it is the opposite of New. It's really a memory operation.
	inline void Delete(Elem *e);

	// This should probably not be needed to be called directly by the user.
	// Think of it as opposite to Delete();
	inline Elem *New();

	// Find tries to find this element in the current list using the hashtable.
	// It returns NULL if not present. The Elem it returns is not owned by the
	// user, it is part of the internal list owned by this object, but the user
	// is free to modify the "val" element.
	inline Elem *Find(I key);

	// Insert inserts a new element into then hashtable/stored list. By calling
	// this, the user asserts that it is not already present (e.g. Find was 
	// called and return NULL). With current code, calling this if an element 
	// already exists will result in duplicate elements in the structure, and
	// Find() will find the first one that was added.
	// [but we don't guarantee this behavior].
	inline Elem *Insert(I key, T val);

	// Insert inserts another element with same key into hashtable/
	// stored list.
	// By calling this, the user asserts that one element with that key is
	// already present.
	// We insert it that way, that all element with the same key
	// follow each other.
	// Find() will return the first one of the elements with the same key.
	inline void InsertMore(I key, T val);

	// SetSize tells the object how many hash buckets to allocate (should
	// typically be at least twice the number of objects we expect to go in the
	// structure, for fastest performance). It must be called while the hash
	// is empty (e.g. after Clear() or after initializing the object, but before
	// adding anything to the hash.
	void SetSize(size_t sz);

	// Return current number of hash buckets.
	inline size_t Size() { return _hash_size; }

	void DeleteElems();

	~HashList();
private:
	struct HashBucket
	{
		size_t prev_bucket; // index to next bucket( -1 if list tail). Note:
		// list of buckets goes in opposite direction to list of Elems.
		Elem *last_elem;
		inline HashBucket(size_t i,Elem *e):prev_bucket(i), last_elem(e) { }
	};

	Elem *_list_head; // head of currently stored list.
	size_t _bucket_list_tail; // tail of list of active hash buckets.

	size_t _hash_size;

	std::vector<HashBucket> _buckets;

	Elem *_freed_head; // head of list of currently freed elements.
	// [ready for allocation]
	
	std::vector<Elem *> _allocated; // list of allocated blocks.

	static const size_t _allocate_block_size = 1024;
	// Numbers of Elements to allocate in one block. Must be largish so storing
	//_allocated doesn't become a problem.
};

#include "src/util/namespace-end.h"

#include "src/util/hash-list-inl.h"

#endif

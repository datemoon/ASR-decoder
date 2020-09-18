#ifndef __HASH_LIST_INL_H__
#define __HASH_LIST_INL_H__

#include "src/util/namespace-start.h"

template<class I, class T> HashList<I, T>::HashList()
{
	_list_head = NULL;
	_bucket_list_tail = static_cast<size_t>(-1); // invalid.
	_hash_size = 0;
	_freed_head = NULL;
}

template<class I, class T> 
void HashList<I, T>::SetSize(size_t size)
{
	_hash_size = size;
	assert(_list_head == NULL &&
			_bucket_list_tail == static_cast<size_t> (-1)); // make sure empty.
	if(size > _buckets.size())
		_buckets.resize(size, HashBucket(0, NULL));
}

template<class I, class T>
typename HashList<I, T>::Elem* HashList<I, T>::Clear()
{ // Clears the hashtable and gives ownership of the currently contained list
	// to the user.
	for(size_t cur_bucket = _bucket_list_tail;
			cur_bucket != static_cast<size_t>(-1);
			cur_bucket = _buckets[cur_bucket].prev_bucket)
	{
		_buckets[cur_bucket].last_elem = NULL; // this is how we indicate "empty".
	}
	_bucket_list_tail = static_cast<size_t>(-1);
	Elem *ans = _list_head;
	_list_head = NULL;
	return ans;
}

template<class I, class T>
const typename HashList<I, T>::Elem* HashList<I, T>::GetList() const
{
	return _list_head;
}

template<class I, class T>
inline void HashList<I, T>::Delete(Elem *e)
{
	e->tail = _freed_head;
	_freed_head = e;
}

template<class I, class T>
void HashList<I, T>::DeleteElems()
{
	for(Elem *e = this->Clear(), *e_tail = NULL; e != NULL; e = e_tail)
	{
		e_tail = e->tail;
		this->Delete(e);
	}
}

template<class I, class T>
inline typename HashList<I, T>::Elem* HashList<I, T>::Find(I key)
{
	size_t index = (static_cast<size_t>(key) % _hash_size);
	HashBucket &bucket = _buckets[index];
	if(bucket.last_elem == NULL)
	{
		return NULL; // empty bucket.
	}
	else
	{
		Elem *head = (bucket.prev_bucket == static_cast<size_t>(-1) ?
				_list_head : _buckets[bucket.prev_bucket].last_elem->tail),
			 *tail = bucket.last_elem->tail;
		for(Elem *e = head; e != tail; e = e->tail)
			if(e->key == key)
				return e;
		return NULL; // Not found.
	}
}

template<class I, class T>
inline typename HashList<I, T>::Elem* HashList<I, T>::New()
{
	if(_freed_head)
	{
		Elem *ans = _freed_head;
		_freed_head = _freed_head->tail;
		return ans;
	}
	else
	{
		Elem *tmp = new Elem[_allocate_block_size];
		for(size_t i = 0 ; i+1 < _allocate_block_size; ++i)
			tmp[i].tail = tmp+i+1;
		tmp[_allocate_block_size-1].tail = NULL;
		_freed_head = tmp;
		_allocated.push_back(tmp);
		return this->New();
	}
}

template<class I, class T>
HashList<I, T>::~HashList()
{ // First test whether we had any memory leak within the
	// HashList, i.e. things for which the user did not call Delete().
	//this->DeleteElems();
	size_t num_in_list = 0, num_allocated = 0;
	for(Elem *e = _freed_head; e != NULL; e=e->tail)
		num_in_list++;
	for(size_t i=0; i<_allocated.size(); i++)
	{
		num_allocated += _allocate_block_size;
		delete[] _allocated[i];
	}
	if(num_in_list != num_allocated)
	{
		LOG_WARN << "Possible memory leak: " << num_in_list
			<< " != " << num_allocated
			<< ": you might have forgotten to call Delete on "
			<< "some Elems";
	}
}

template<class I, class T>
inline typename HashList<I, T>::Elem* HashList<I, T>::Insert(I key, T val)
{
	size_t index = (static_cast<size_t>(key) % _hash_size);
	HashBucket &bucket = _buckets[index];
	// Check the element is existing or not.
	if (bucket.last_elem != NULL)
	{
		Elem *head = (bucket.prev_bucket == static_cast<size_t>(-1) ?
				_list_head :
				_buckets[bucket.prev_bucket].last_elem->tail),
			 *tail = bucket.last_elem->tail;
		for (Elem *e = head; e != tail; e = e->tail)
		{
			if (e->key == key) 
				return e;
		}
	}
	Elem *elem = New();
	elem->key = key;
	elem->val = val;
	if(bucket.last_elem == NULL)
	{ // Unoccupied bucket. Insert at head of bucket list (which is tail of 
		// regularlist, they go in opposite directions).
		if(_bucket_list_tail == static_cast<size_t>(-1))
		{ // list was empty so this is the first elem.
			assert(_list_head == NULL);
			_list_head = elem;
		}
		else
		{ // link in to the chain of Elems
			_buckets[_bucket_list_tail].last_elem->tail = elem;
		}
		elem->tail = NULL;
		bucket.last_elem = elem;
		bucket.prev_bucket = _bucket_list_tail;
		_bucket_list_tail = index;
	}
	else
	{ // Already-occupied bucket. Insert at tail of list of elements within
		// the bucket.
		elem->tail = bucket.last_elem->tail;
		bucket.last_elem->tail = elem;
		bucket.last_elem = elem;
	}
	return elem;
}

template<class I, class T>
void HashList<I, T>::InsertMore(I key, T val)
{
	size_t index = (static_cast<size_t>(key) % _hash_size);
	HashBucket &bucket = _buckets[index];
	Elem *elem = New();
	elem->key = key;
	elem->val = val;
	
	assert(bucket.last_elem != NULL); // assume one element is already here
	if(bucket.last_elem->key == key)
	{ // standard behavior: add as last element
		elem->tail = bucket.last_elem->tail;
		bucket.last_elem->Tail = elem;
		bucket.last_elem = elem;
		return;
	}
	Elem *e = (bucket.prev_bucket == static_cast<size_t>(-1) ?
			_list_head : _buckets[bucket.prev_bucket].last_elem->tail);
	while(e != bucket.last_elem->tail && e->key != key)
		e = e->tail;
	assert(e->key == key); // not found? - should not happen
	elem->tail = e->tail;
	e->tail = elem;
}

#include "src/util/namespace-end.h"

#endif

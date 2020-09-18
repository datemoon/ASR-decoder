#ifndef __UTIL_MEM_POOL_H__
#define __UTIL_MEM_POOL_H__
#include <vector>
#include <assert.h>
#include <string.h>

#include "src/util/namespace-start.h"

class TypeBase
{
private:
public:
	TypeBase *_next;
};

template <typename T>
class MemPool
{
public:
	MemPool(unsigned int allocate_block_size = 128):_pool_head(NULL), _pool_size(0), _allocate_block_size(allocate_block_size){ }

	T* NewT()
	{
		if(_pool_head == NULL)
		{
			T* val = new T[_allocate_block_size];
			memset(val, 0x00, sizeof(T)*_allocate_block_size);
			for(size_t i=0; i+1<_allocate_block_size; ++i)
				val[i]._next = val+i+1;
			val[_allocate_block_size-1]._next = NULL;
			_pool_head = val;
			_pool_size += _allocate_block_size;
			_pool.push_back(val);
		}
		T *ret = _pool_head;
		_pool_head = _pool_head->_next;
		_pool_size--;
		return ret;
	}

	void DeleteT(T* val)
	{
		val->_next = _pool_head;
		_pool_head = val;
		_pool_size++;
	}

	~MemPool()
	{
		for(size_t i=0;i<_pool.size();++i)
		{
			delete []_pool[i];
			_pool_size -= _allocate_block_size;
		}
		_pool.clear();
		_pool_head = NULL;
		assert(_pool_size == 0 && _pool_head == NULL);
	}
private:
	std::vector<T *> _pool;
	T *_pool_head;
	size_t _pool_size;
	unsigned int _allocate_block_size;

};

#include "src/util/namespace-end.h"
#endif

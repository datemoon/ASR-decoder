#include <map>  // for baseline.
#include <cstdlib>
#include <iostream>
#include "src/util/hash-list.h"

#ifdef NAMESPACE
using namespace datemoon;
#endif
template<class Int, class T> void TestHashList() 
{
  typedef typename HashList<Int, T>::Elem Elem;

  HashList<Int, T> hash;
  hash.SetSize(200);  // must be called before use.
  std::map<Int, T> m1;
  for (size_t j = 0; j < 50; j++) {
    Int key = random() % 200;
    T val = random() % 50;
    m1[key] = val;
    Elem *e = hash.Find(key);
    if (e) e->val = val;
    else  hash.Insert(key, val);
  }


  std::map<Int, T> m2;

  for (int i = 0; i < 100; i++) {
    m2.clear();
    for (typename std::map<Int, T>::const_iterator iter = m1.begin();
        iter != m1.end();
        iter++) {
      m2[iter->first + 1] = iter->second;
    }
    std::swap(m1, m2);

    Elem *h = hash.Clear(), *tmp;

    hash.SetSize(100 + random() % 100);  // note, SetSize is relatively cheap
    // operation as long as we are not increasing the size more than it's ever
    // previously been increased to.

    for (; h != NULL; h = tmp) {
      hash.Insert(h->key + 1, h->val);
      tmp = h->tail;
      hash.Delete(h);  // think of this like calling delete.
    }

    // Now make sure h and m2 are the same.
    const Elem *list = hash.GetList();
    size_t count = 0;
    for (; list != NULL; list = list->tail, count++) {
      assert(m1[list->key] == list->val);
    }

    for (size_t j = 0; j < 10; j++) {
      Int key = random() % 200;
      bool found_m1 = (m1.find(key) != m1.end());
      if (found_m1) m1[key];
      Elem *e = hash.Find(key);
      assert((e != NULL) == found_m1);
      if (found_m1)
        assert(m1[key] == e->val);
    }

    assert(m1.size() == count);
  }
	Elem *tmp = hash.Clear();
	Elem *e_tail = NULL;
	for(;tmp != NULL; tmp = e_tail)
	{
		e_tail = tmp->tail;
		hash.Delete(tmp);
	}
}

int main() {
  for (size_t i = 0;i < 3;i++) {
    TestHashList<int, unsigned int>();
    TestHashList<unsigned int, int>();
    TestHashList<short, int>();
    TestHashList<short, int>();
    TestHashList<char, unsigned char>();
    TestHashList<unsigned char, int>();
  }
  LOG_COM << "Test OK.\n";
}

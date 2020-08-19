#ifndef __FST_TOPSORT_H__
#define __FST_TOPSORT_H__

#include <vector>
#include "fst/lattice-fst.h"

void StateSort(Lattice *fst,const std::vector<StateId> &order);

void TopSort(Lattice *fst);

#endif

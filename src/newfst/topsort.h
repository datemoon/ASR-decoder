#ifndef __FST_TOPSORT_H__
#define __FST_TOPSORT_H__

#include <vector>
#include "src/newfst/lattice-fst.h"

#include "src/util/namespace-start.h"
void StateSort(Lattice *fst,const std::vector<StateId> &order);

void TopSort(Lattice *fst);
#include "src/util/namespace-end.h"
#endif

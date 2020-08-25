#ifndef __LATTICE_FUNCTIONS_H__
#define __LATTICE_FUNCTIONS_H__

#include "src/newfst/lattice-fst.h"

#include "src/util/namespace-start.h"
void LatticeShortestPath(Lattice *ilat, Lattice *shortest_lat);

// check top fst
bool TopCheck(Lattice &fst);

bool LatticeCheckFormat(Lattice *fst);

void LatticeRmInput(Lattice &fst);

void AddSuperFinalState(Lattice &fst);

#include "src/util/namespace-end.h"
#endif

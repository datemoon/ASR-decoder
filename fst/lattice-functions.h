#ifndef __LATTICE_FUNCTIONS_H__
#define __LATTICE_FUNCTIONS_H__

#include "fst/lattice-fst.h"

void LatticeShortestPath(Lattice *ilat, Lattice *shortest_lat);

// check top fst
bool TopCheck(Lattice &fst);

bool LatticeCheckFormat(Lattice *fst);

void LatticeRmInput(Lattice &fst);

void AddSuperFinalState(Lattice &fst);

#endif

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

bool LatticeToVector(Lattice &best_path, 
		std::vector<int> &best_words_arr, std::vector<int> &best_phones_arr,
		float &best_tot_score, float &best_lm_score);
#include "src/util/namespace-end.h"
#endif

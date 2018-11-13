#include <stdio.h>
#include <string.h>
#include "fst/lattice-fst.h"

void Lattice::Invert()
{
	for(size_t i = 0; i < static_cast<size_t>(NumStates()); ++i)
	{
		LatticeState *state = _state[i];
		for(size_t j = 0; j < static_cast<size_t>(state->GetArcSize()); ++j)
		{
			LatticeArc *arc = state->GetArc(j);
			arc->Invert();
		}
	}
}

void Lattice::ArcSort()
{
	for(size_t i = 0; i < static_cast<size_t>(NumStates()); ++i)
	{
		LatticeState *state = _state[i];
		state->SortArc(0,state->GetArcSize()-1);
	}
}

void Lattice::DelSameArc()
{
	this->ArcSort();
	for(size_t i = 0; i < static_cast<size_t>(NumStates()); ++i)
	{
		LatticeState *state = _state[i];
		state->DelSameArc();
	}
}


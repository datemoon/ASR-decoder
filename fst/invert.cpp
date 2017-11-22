#include "fst/lattice-fst.h"
#include "fst/invert.h"

void Invert(Lattice &fst)
{
	StateId num_states = fst.NumStates();
	for(StateId s = 0 ; s < num_states; ++s)
	{
		LatticeState *state = fst.GetState(s);
		for(unsigned i = 0; i < state->GetArcSize();++i)
		{
			Arc *arc = state->GetArc(i);
			arc->Invert();
		}
	}
}



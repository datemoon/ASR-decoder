#include "src/newfst/lattice-fst.h"
#include "src/newfst/invert.h"

#include "src/util/namespace-start.h"
void Invert(Lattice &fst)
{
	StateId num_states = fst.NumStates();
	for(StateId s = 0 ; s < num_states; ++s)
	{
		LatticeState *state = fst.GetState(s);
		for(unsigned i = 0; i < state->GetArcSize();++i)
		{
			//LatticeArc *arc = state->GetArc(i);
			state->GetArc(i)->Invert();
		}
	}
}
#include "src/util/namespace-end.h"


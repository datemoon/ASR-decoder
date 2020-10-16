#include <vector>
#include "src/newfst/lattice-fst.h"
#include "src/newfst/reverse.h"

#include "src/util/namespace-start.h"
void Reverse(Lattice &ifst, Lattice *ofst)
{
	StateId num_states = ifst.NumStates();

	StateId istart = ifst.Start();
	StateId ostart = ofst->AddState();
	ofst->SetStart(ostart);

	for(StateId i = 0; i < num_states; ++i)
	{
		LatticeState * icur_state = ifst.GetState(i);
		StateId is = i;     // ifst state id.
		StateId os = i + 1; // ofst corresponding state id.
		while(ofst->NumStates() <= os)
			ofst->AddState();
		if(is == istart)
			ofst->SetFinal(os);

		// if ifst state is final link this state to start.
		if(icur_state->IsFinal())
		{
			LatticeArc oarc(0 , 0, os, LatticeWeight::One());
			ofst->AddArc(0, oarc);
		}

		for(unsigned j = 0; j < icur_state->GetArcSize(); ++j)
		{
			LatticeArc *iarc = icur_state->GetArc(j);
			LatticeArc oarc(iarc->_input, iarc->_output, os, iarc->_w);
			StateId nos = iarc->_to + 1; // arrive state
			while(ofst->NumStates() <= nos)
				ofst->AddState();
			ofst->AddArc(nos, oarc);
		}
	}
}
#include "src/util/namespace-end.h"

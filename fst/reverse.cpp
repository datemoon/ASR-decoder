#include <vector>
#include "fst/lattice-fst.h"
#include "fst/reverse.h"

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
			Arc oarc(0 , 0,os, 0.0);
			ofst->AddArc(0, oarc);
		}

		for(unsigned j = 0; j < icur_state->GetArcSize(); ++j)
		{
			Arc *iarc = icur_state->GetArc(j);
			Arc oarc(iarc->_input, iarc->_output, os, iarc->_w);
			StateId nos = iarc->_to + 1; // arrive state
			while(ofst->NumStates() <= nos)
				ofst->AddState();
			ofst->AddArc(nos, oarc);
		}
	}
}


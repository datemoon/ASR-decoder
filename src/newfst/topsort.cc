#include "src/newfst/topsort.h"
#include "src/newfst/dfs-visit-fst.h"

using namespace std;

#include "src/util/namespace-start.h"
void StateSort(Lattice *fst,const vector<StateId> &order)
{
	if(order.size() != (unsigned)(fst->NumStates()))
	{
		LOG_WARN << "StateSort: bad order vector size: " << order.size() ;
		return ;
	}
	if(fst->Start() == kNoStateId)
		return;

	StateId startid = fst->Start();
	fst->SetStart(order[startid]);
	// reorder fst state.
	LatticeState *tmp = NULL;
	LatticeState *cur = NULL;
	StateId nstates = fst->NumStates();
	vector<bool> reorder(nstates, false);
	for(StateId id = 0; id < nstates ; ++id)
	{ // reorder fst state
		if(reorder[id] == false)
		{
			StateId true_posi = order[id];
			cur = fst->GetState(id);
			while(true_posi != id)
			{
				tmp = fst->GetState(true_posi);
				fst->SetState(true_posi, cur);
				reorder[true_posi] = true;
				true_posi = order[true_posi];
				cur = tmp;
			}
			fst->SetState(true_posi,cur);
		}
	}
}

void TopSort(Lattice *fst)
{
	vector<bool> access,coaccess;
	vector<StateId> order;
	DfsVisit(fst, &access, &coaccess, &order);

	StateSort(fst, order);
	StateId nstates = fst->NumStates();
	// modefy tostate
	for(StateId id = 0; id < nstates ; ++id)
	{
		LatticeState *state = fst->GetState(id);
		for(size_t i = 0 ; i < state->GetArcSize(); ++i)
		{
			//LatticeArc *arc = state->GetArc(i);
			//arc->_to = order[arc->_to];
			StateId nextstate = state->GetArc(i)->_to;
			state->GetArc(i)->_to = order[nextstate];
		}
	}
}

#include "src/util/namespace-end.h"

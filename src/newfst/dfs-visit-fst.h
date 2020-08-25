#ifndef __DFS_VISIT_FST_H__
#define __DFS_VISIT_FST_H__

#include <stack>
using std::stack;
#include <iostream>
#include <vector>

using namespace std;
using std::vector;

#include "src/newfst/lattice-fst.h"

#include "src/util/namespace-start.h"
const int kDfsWhite = 0;   // Undiscovered
const int kDfsGrey =  1;   // Discovered & unfinished
const int kDfsBlack = 2;   // Finished

bool DfsSearch(Lattice *fst, StateId root ,vector<char> *state_color, 
		vector<bool> *access, vector<bool> *coaccess,
	   	vector<StateId> *order=NULL, StateId *curstateid = NULL);

void DfsVisit(Lattice *fst, vector<bool> *access, vector<bool> *coaccess,vector<StateId> *order=NULL);

#ifdef FST_NEW
// An Fst state's DFS stack state
template <class Arc>
struct DfsState 
{
	typedef typename Arc::StateId StateId;

	DfsState(StateId state_id):_state_id(state_id), _arc_id(0) { }

	void ArcAddOne(Fst &fst)
	{
//		FstState *state = fst.GetState(_state_id);
//		if(_arc_id + 1 < state->GetArcSize())
		{
			_arc_id ++;
//			return true;
		}
//		else
//			return false;
	}
	StateId _state_id;
	int _arc_id;
};
// Performs depth-first visitation. Visitor class argument determines
// actions and contains any return data. ArcFilter determines arcs
// that are considered.
//
// Note this is similar to Visit() in visit.h called with a LIFO
// queue except this version has a Visitor class specialized and
// augmented for a DFS.
template <class Arc, class V, class ArcFilter>
void DfsVisit(const Fst<Arc> &fst, V *visitor, ArcFilter filter)
{
	typedef typename Arc::StateId StateId;

	visitor->InitVisit(fst);

	StateId start = fst.Start();
	if(start == kNoStateId)
	{
		vistor->FinishVisit();
		return ;
	}

	vector<char> state_color;       // Fst state Dfs status
	stack<DfsState> state_stack;     // DFS execution stack , don't use recursion

	StateId nstates = start + 1;    // # of known states in general case
	bool expanded = false;

	{ // tests if expanded case , but now don't check
	   	nstates = fst.NumStates();
		expanded = true
	}

	state_color.resize(nstates, kDfsWhite);
	
	// Continue DFS while true
	bool dfs = true;

	// Iterate over trees in DFS forest.
	for( StateId root = start; dfs && root < nstates; )
	{
		state_color[root] = kDfsGrey;
		state_stack.push(DfsState(root));
		dfs = visitor->InitState(root, root);
		while(!state_stack.empty())
		{
			DfsState dfs_state = state_stack.top();
			StateId s = dfs_state._state_id;
			int arcid = dfs_state._arc_id;
			if(s > state_color.size())
			{
				nstates = s + 1;
				state_color.resize(nstates, kDfsWhite);
			}
			FstState *state = fst->GetState(s);
			if(!dfs || arcid >= state->GetArcSize())
			{ // Ok this state
				state_color[s] = kDfsBlack;
				state_stack.pop();
				if(!state_stack.empty())
				{
					DfsState parent_state = state_stack.top();
					StateId p = parent_state->_state_id;
					visitor->FinishState(s, p, 0);
					parent_state.AddArcOne();
				}
				else
				{
					visitor->FinishState(s, kNoStateId, 0);
				}
				continue;
			}
			Arc *arc = state->GetArc(s);
			if(arc->_to >= state_color.size())
			{
				nstates = arc->_to + 1;
				state_color.resize(nstates, kDfsWhite);
			}

			int next_color = state_color[arc->_to];

			switch(next_color)
			{
				case kDefsWhite:
					dfs  = visitor->TreeArc(s, arc);
					if(!dfs)
						break;
					state_color[arc->_to] = kDfsGrey;
					state_stack.push(DfsState(arc->_to));
					break;
				case kDfsGrey:
					dfs = visitor->BackArc(s, *arc);
					dfs_state.AddArcOne();
					break;
				case kDfsBlack:
					dfs = visitor->ForwardOrCrossArc(s, arc);
					dfs_state.AddArcOne();
					break;
				default:
					break;
			}

			// Find next tree root
			for(root = (root == start ? 0 : root + 1);
					root < nstates && state_color[root] != kDfsWhite;
					++root) { }

			// Check for a state beyond the largest known state
		}
	}
	visitor->FinishVisit();
}
#endif
#include "src/util/namespace-end.h"
#endif

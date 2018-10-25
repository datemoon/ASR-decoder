#include <iostream>
#include <vector>
#include <cassert>
#include <utility>
#include <algorithm>
#include <limits>

#include "fst/lattice-fst.h"
#include "fst/lattice-functions.h"
#include "fst/topsort.h"

void LatticeShortestPath(Lattice *ilat, Lattice *shortest_lat)
{
	// here ilat must be topologically sort lattice
	if(!TopCheck(*ilat))
	{
		TopSort(ilat);
		assert(TopCheck(*ilat));
	}
	shortest_lat->DeleteStates();
	if (ilat->Start() == kNoStateId) 
		return;
	assert(ilat->Start() == 0);
	vector<std::pair<float, StateId> > best_cost_and_pred(ilat->NumStates() + 1);
	StateId superfinal = ilat->NumStates();
	for(StateId s =0; s <= ilat->NumStates(); ++s)
	{
		best_cost_and_pred[s].first = numeric_limits<float>::infinity();
		best_cost_and_pred[s].second = kNoStateId;
	}
	best_cost_and_pred[0].first = 0;
	for(StateId s = 0; s < ilat->NumStates(); ++s)
	{
		float my_cost = best_cost_and_pred[s].first;
		LatticeState * cur_state = ilat->GetState(s);
		for(unsigned i = 0; i < cur_state->GetArcSize(); ++i)
		{
			Arc *arc = cur_state->GetArc(i);
			float arc_cost = arc->_w;
			float next_cost = my_cost + arc_cost;
			if (next_cost < best_cost_and_pred[arc->_to].first)
			{
				best_cost_and_pred[arc->_to].first = next_cost;
				best_cost_and_pred[arc->_to].second = s;
			}
		}
		if(cur_state->IsFinal())
		{
			float tot_final = my_cost;
			if(tot_final < best_cost_and_pred[superfinal].first)
			{
				best_cost_and_pred[superfinal].first = tot_final;
				best_cost_and_pred[superfinal].second = s;
			}
		}
	}

	std::vector<StateId> states; // states on best path.
	StateId cur_state = superfinal;
	while(cur_state != 0)
	{
		StateId prev_state = best_cost_and_pred[cur_state].second;
		if(prev_state == kNoStateId)
		{
			LOG_WARN << "Failure in best-path algorithm for lattice (infinite costs?)" ;
			return ; // return empty best-path.
		}
		states.push_back(prev_state);
		assert(cur_state != prev_state && "Lattice with cycles");
		cur_state = prev_state;
	}
	std::reverse(states.begin(), states.end());
	for (size_t i = 0; i < states.size(); i++)
		shortest_lat->AddState();
	for (StateId s = 0; static_cast<size_t>(s) < states.size(); s++)
	{
		if(s == 0)
			shortest_lat->SetStart(s);
		if (static_cast<size_t>(s + 1) < states.size())
		{ // transition to next state.
			bool have_arc = false;
			Arc cur_arc;
			LatticeState * cur_state = ilat->GetState(states[s]);
			for(unsigned i = 0; i < cur_state->GetArcSize(); ++i)
			{
				Arc *arc = cur_state->GetArc(i);
				if(arc->_to == states[s+1])
				{
					if(!have_arc || arc->_w < cur_arc._w)
					{
						cur_arc = *arc;
						have_arc = true;
					}
				}
			}
			assert(have_arc && "Code error.");
			cur_arc._to = s+1;
			shortest_lat->AddArc(s, cur_arc);
		}
		else // final-prob
		{
			shortest_lat->SetFinal(s);
		}
	}
}


bool TopCheck(Lattice &fst)
{
	StateId num_states = fst.NumStates();
	for(StateId s = 0; s < num_states ; ++s)
	{
		LatticeState * cur_state = fst.GetState(s);
		for(unsigned i = 0 ; i < cur_state->GetArcSize(); ++i)
		{
			Arc *arc = cur_state->GetArc(i);
			if(s >= arc->_to)
				return false;
		}
	}
	return true;
}

void LatticeRmInput(Lattice &fst)
{
	StateId num_states = fst.NumStates();
	for(StateId s = 0; s < num_states ; ++s)
	{
		LatticeState * cur_state = fst.GetState(s);
		for(unsigned i = 0 ; i < cur_state->GetArcSize(); ++i)
		{
			Arc *arc = cur_state->GetArc(i);
			arc->_input = 0;
		}
	}
}
bool LatticeCheckFormat(Lattice *fst)
{
	StateId num_states = fst->NumStates();
	for(StateId s = 0; s < num_states ; ++s)
	{
		LatticeState * cur_state = fst->GetState(s);
		if(cur_state->IsFinal())
		{
			if(cur_state->GetArcSize() != 0)
			{
				LOG_WARN << "it's error format lattice. num arc " << cur_state->GetArcSize() ;
				//return false;
			}
		}
		else
		{
			if(cur_state->GetArcSize() == 0)
			{
				LOG_WARN << "it's error format lattice. non final state no arc" ;
				return false;
			}
		}
	}
	return true;
}

void AddSuperFinalState(Lattice &fst)
{
	StateId final_state = fst.AddState();
	fst.SetFinal(final_state);
	StateId num_states = fst.NumStates();
	for(StateId s = 0; s < num_states ; ++s)
	{
		LatticeState * cur_state = fst.GetState(s);
		if(cur_state->IsFinal() && s != final_state)
		{
			cur_state->UnsetFinal();
			Arc arc(0,0,final_state,0.0);
			cur_state->AddArc(arc);
		}
	}
}

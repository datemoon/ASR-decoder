#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
#include <limits>

#include "src/newfst/lattice-fst.h"
#include "src/newfst/lattice-functions.h"
#include "src/newfst/topsort.h"

#include "src/util/namespace-start.h"
void LatticeShortestPath(Lattice *ilat, Lattice *shortest_lat)
{
	// here ilat must be topologically sort lattice
	if(!TopCheck(*ilat))
	{
		TopSort(ilat);
		LOG_ASSERT(TopCheck(*ilat));
	}
	shortest_lat->DeleteStates();
	if (ilat->Start() == kNoStateId) 
		return;
	LOG_ASSERT(ilat->Start() == 0);
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
			LatticeArc *arc = cur_state->GetArc(i);
			float arc_cost = arc->_w.Value();
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
		LOG_ASSERT(cur_state != prev_state && "Lattice with cycles");
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
			LatticeArc cur_arc;
			LatticeState * cur_state = ilat->GetState(states[s]);
			for(unsigned i = 0; i < cur_state->GetArcSize(); ++i)
			{
				LatticeArc *arc = cur_state->GetArc(i);
				if(arc->_to == states[s+1])
				{
					if(!have_arc || arc->_w.Value() < cur_arc._w.Value())
					{
						cur_arc = *arc;
						have_arc = true;
					}
				}
			}
			LOG_ASSERT(have_arc && "Code error.");
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
			LatticeArc *arc = cur_state->GetArc(i);
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
			LatticeArc *arc = cur_state->GetArc(i);
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
			LatticeArc arc(0,0,final_state,LatticeWeight(0.0, 0.0));
			cur_state->AddArc(arc);
		}
	}
}
bool LatticeToVector(Lattice &best_path, 
		std::vector<int> &best_words_arr, std::vector<int> &best_phones_arr,
		float &best_tot_score, float &best_lm_score)
{
	if(best_path.Start() == kNoStateId)
		return false;
	best_tot_score = 0;
	best_lm_score = 0;
	// here best_path can't be top sort
	StateId start_start = best_path.Start();
	LatticeState * cur_state = best_path.GetState(start_start);
	while(!cur_state->IsFinal())
	{
		// beacause one best path , so every state have only one arc.
		LatticeArc *arc = cur_state->GetArc(0);
		StateId next_stateid = arc->_to;
		if(arc->_input != 0)
			best_phones_arr.push_back(arc->_input);
		if(arc->_output != 0)
			best_words_arr.push_back(arc->_output);
		best_lm_score += arc->_w.Value1();
		best_tot_score += arc->_w.Value1() + arc->_w.Value2();
		cur_state = best_path.GetState(next_stateid);
	}/*
	for(StateId s = 0 ; s < best_path.NumStates(); ++s)
	{
		// beacause one best path , so every state have only one arc.
		if(best_path.GetState(s)->GetArcSize() == 1)
		{
			Arc *arc = best_path.GetState(s)->GetArc(0);
			if(arc->_input != 0)
				best_phones_arr.push_back(arc->_input);
			if(arc->_output != 0)
				best_words_arr.push_back(arc->_output);
			best_tot_score += arc->_w;
		}// else is final
	}*/
	return true;
}
#include "src/util/namespace-end.h"


#include <vector>
#include <utility>
#include "src/newfst/rmepsilon.h"
#include "src/newfst/connect-fst.h"

#include "src/util/namespace-start.h"

void RmEpsilon(Lattice &ifst, int ilabel)
{
	// first delete no use arc and state
	Connect(&ifst);
	typedef LatticeWeight Weight;
	typedef pair<StateId ,Weight> Pair;
	// the ifst must be no eps loop, here I don't check.default it's.
	StateId start_state = ifst.Start();
	if(start_state == kNoStateId)
		return ;
	StateId num_states = ifst.NumStates();
	vector<bool> rmeps_state(num_states,false);
	vector<Pair> cur_state_queue;// equal state
	vector<StateId> state_queue; // current delete eps arc state
	
	state_queue.push_back(start_state);
	rmeps_state[start_state] = true;    
	
	int delete_arc_num = 0;
	while(!state_queue.empty())
	{
		StateId s = state_queue.back();
		state_queue.pop_back();
		cur_state_queue.clear();
		cur_state_queue.push_back(Pair(s, Weight::One()));
		LatticeState *cur_state = ifst.GetState(s);
		// delete this state all eps, and add equal arc , and can be arrive state.
		while(!cur_state_queue.empty())
		{
			Pair statepair = cur_state_queue.back();
			cur_state_queue.pop_back();
			StateId cur_stateid = statepair.first;
			LatticeState *state = ifst.GetState(cur_stateid);
			Weight w = statepair.second;
			// the i muset be int ,because it can be -1.
			for(int i = 0 ; i< (int)state->GetArcSize();++i)
			{
				LatticeArc *arc = state->GetArc(i);
				if(arc->_input == ilabel && arc->_output == 0 && !ifst.Final(arc->_to))
				{
					if(s == cur_stateid) // delete this arc
					{
					/*	if(ifst.Final(arc->_to))
						{ // don't delete arrive final state arc.
							rmeps_state[arc->_to] = true;
							arc->_w += w;
							continue;
						}*/
						cur_state_queue.push_back(Pair(arc->_to, 
									Times(w, arc->_w)));
						cur_state->DeleteArc(i);
//						VLOG_COM(3) << "state " << s << " delete arc " << i ;
						delete_arc_num++;
						// beacuse delete this arc, so I need i--.
						i--;
					}
					else
						cur_state_queue.push_back(Pair(arc->_to, 
									Times(w, arc->_w)));
					//if(ifst.Final(arc->_to))
					//	ifst.SetFinal(s);
					continue;
				}
				else
				{
					if(s == cur_stateid)
					{
						// add this tostate to state_queue
						if(rmeps_state[arc->_to] != true)
						{
							// add this state to state_queue, and after don't add.
							state_queue.push_back(arc->_to);
							rmeps_state[arc->_to] = true;
						}
						continue;
					}
					// eps arrive state, so nextstate should be add state_queue
					cur_state->AddArc(LatticeArc(arc->_input,arc->_output,arc->_to,
								Times(arc->_w, w)));
					if(rmeps_state[arc->_to] != true)
					{
						state_queue.push_back(arc->_to);
						rmeps_state[arc->_to] = true;
					}
				}
			}
		}// one state
		rmeps_state[s] = true;
	} // all state

	VLOG_COM(2) << "delete arc input " << ilabel << " num:" << delete_arc_num ;
	Connect(&ifst);
}
#include "src/util/namespace-end.h"

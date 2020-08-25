#include <vector>
#include "src/newfst/lattice-functions.h"
#include "src/newfst/lattice-to-nbest.h"
#include "src/newfst/reverse.h"
#include "src/newfst/connect-fst.h"
#include "src/newfst/topsort.h"

#include <unordered_map>
#include <iostream>

using namespace std;
using std::unordered_map;
#include "src/util/namespace-start.h"

void NShortestPath(Lattice &ifst, Lattice *ofst,
		size_t n)
{
	typedef float Weight;
	typedef pair<StateId, Weight> Pair;

	if(!TopCheck(ifst))
	{
		TopSort(&ifst);
		LOG_ASSERT(TopCheck(ifst));
	}

	AddSuperFinalState(ifst);

//#define TEST
#ifdef TEST
	unordered_map<LatticeArc *, bool> hash_arc;
#endif
	// here ifst must be topologically sort fst
	// don't check
	ofst->DeleteStates();
	if(ifst.Start() == kNoStateId)
		return ;
	LOG_ASSERT(ifst.Start() == 0);

	vector<float> best_cost_and_back(ifst.NumStates());

//	StateId superfinal = -1;

	for(StateId s = ifst.NumStates()-1; s>=0 ; --s)
	{
		best_cost_and_back[s] = numeric_limits<float>::infinity();
	}
	// now calculate back score
	for(StateId s = ifst.NumStates()-1 ; s>=0 ; --s)
	{
		LatticeState * cur_state = ifst.GetState(s);
		if(cur_state->IsFinal())
		{
			best_cost_and_back[s] = 0.0;
			continue;
		}

		for(unsigned i = 0; i < cur_state->GetArcSize(); ++i)
		{
			LatticeArc *arc = cur_state->GetArc(i);
			float arc_cost = arc->_w.Value();
			StateId nextstate = arc->_to;
			LOG_ASSERT(s < nextstate); // because it's topologically sort
			float next_cost = arc_cost + best_cost_and_back[nextstate];
			if(best_cost_and_back[s] > next_cost)
				best_cost_and_back[s] = next_cost;
		}
	}

	// Each state in 'ofst' corresponds to a path with weight w from the
	// initial state of 'ifst' to a state s in 'ifst', that can be
	// characterized by a pair (s,w).  The vector 'pairs' maps each
	// state in 'ofst' to the corresponding pair maps states in OFST to
	// the corresponding pair (s,w).
	vector<Pair> pairs;
	// 'r[s + 1]', 's' state in 'fst', is the number of states in 'ofst'
	// which corresponding pair contains 's' ,i.e. , it is number of
	// paths computed so far to 's'. Valid for 's == -1' (superfinal).
	vector<int> r;
	vector<StateId> heap;
	ShortestPathCompare compare(pairs, best_cost_and_back, -1, 1e-4);

	StateId start = ofst->AddState();
	ofst->SetStart(start);
//	while (pairs.size() <= final)
//		pairs.push_back(Pair(kNoStateId,numeric_limits<float>::infinity()));
//	pairs[start] = Pair(ifst.Start(), 0);
	pairs.push_back(Pair(start, 0));
	heap.push_back(start);

	size_t num_final = 0;
	// here p.first is 'ifst' state, heap is 'ofst' state.
	while(!heap.empty())
	{
		pop_heap(heap.begin(), heap.end(), compare);
		StateId state = heap.back();
		Pair p = pairs[state];
		heap.pop_back();

		while (r.size() <= (unsigned)p.first) 
			r.push_back(0);
		++r[p.first];
		if(num_final == n)
			break;
		if((size_t)r[p.first] > n)
			continue;

		// traverse state arc.
		LatticeState * cur_state = ifst.GetState(p.first);
		for(unsigned i = 0; i < cur_state->GetArcSize(); ++i)
		{
			LOG_ASSERT(!cur_state->IsFinal());
			LatticeArc *arc = cur_state->GetArc(i);
#ifdef TEST
			if(hash_arc.find(arc) != hash_arc.end())
				hash_arc[arc] = true;
			else
			{
				LOG_WARN << "it's not a bug." ;
//				LOG_ASSERT(false);
			}
#endif
			Weight w = p.second + arc->_w.Value(); // it's forward score.
			StateId next = ofst->AddState();
			pairs.push_back(Pair(arc->_to, w));
			//Arc oarc(arc->_input, arc->_output, next, arc->_w);
			//ofst->AddArc(state ,oarc);
			LOG_ASSERT(arc->_to != p.first);
			ofst->AddArc(state ,LatticeArc(arc->_input, arc->_output, next, arc->_w));
			heap.push_back(next);
			push_heap(heap.begin(), heap.end(), compare);
		}

		if(cur_state->IsFinal())
		{
			ofst->SetFinal(state);
			num_final++;
		}
	}

	// delete can not arrive final state.
	Connect(ofst);

	// end ofst it's should be a tree structure.
}
 
void ConvertNbestToVector(Lattice &fst, vector<Lattice> *fsts_out)
{
	Lattice rfst;
	Reverse(fst,&rfst);
	fsts_out->clear();
	StateId start_stateid = rfst.Start();
	if(start_stateid == kNoStateId)
		return; // no output
	
	LatticeState * start_state = rfst.GetState(start_stateid);
	int num_best =  start_state->GetArcSize();
	fsts_out->resize(num_best);
	for(unsigned i = 0; i < start_state->GetArcSize(); ++i)
	{
//		fsts_out->resize(fsts_out->size() + 1);
		Lattice &ofst = (*fsts_out)[i];
		// add start state
		StateId start = ofst.AddState();
		ofst.SetStart(start);
		LatticeArc *arc = start_state->GetArc(i);
		StateId next_ostateid = ofst.AddState();
		ofst.AddArc(start, LatticeArc(arc->_input, arc->_output, next_ostateid , arc->_w));
		StateId cur_stateid = arc->_to;
		StateId cur_ostateid = next_ostateid;
		while(1)
		{
			LatticeState *cur_state = rfst.GetState(cur_stateid);
			unsigned this_n_arcs = cur_state->GetArcSize();

			LOG_ASSERT(this_n_arcs <= 1); // or it violates our assumptions about the input.
			if (this_n_arcs == 1)
			{
				LatticeArc *arc = cur_state->GetArc(0);
				next_ostateid = ofst.AddState();
				ofst.AddArc(cur_ostateid, LatticeArc(arc->_input, arc->_output, next_ostateid, arc->_w));
				cur_stateid = arc->_to;
				cur_ostateid = next_ostateid;
			}
			else // final
			{
				ofst.SetFinal(cur_ostateid);
				break;
			}
		}
	} // get nbest , but it's reserve, so I need reverse every fst

	for(unsigned i = 0 ; i < fsts_out->size(); ++i)
	{
		Lattice ofst ;
		Reverse((*fsts_out)[i], &ofst);
		(*fsts_out)[i].Swap(ofst);
	}
}
#include "src/util/namespace-end.h"

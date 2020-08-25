#ifndef __LATTICE_TO_NBEST_H__
#define __LATTICE_TO_NBEST_H__

#include <vector>
#include <utility>
#include <limits>
#include <algorithm>
#include "src/newfst/connect-fst.h"

using namespace std;

#include "src/util/namespace-start.h"
class ShortestPathCompare
{
public:
	typedef float Weight;
	typedef pair<StateId, Weight> Pair;

	ShortestPathCompare(const vector<Pair>& pairs,
			const vector<Weight>& distance,
			StateId sfinal, float d)
		: _pairs(pairs), _distance(distance), _superfinal(sfinal), _delta(d) { }

	bool operator()(const StateId x, const StateId y) const
	{
		const Pair &px = _pairs[x];
		const Pair &py = _pairs[y];
		LOG_ASSERT((unsigned)px.first < _distance.size() && (unsigned)py.first < _distance.size() && "distance is error.\n");
		Weight dx = (unsigned)px.first < _distance.size() ? _distance[px.first] : numeric_limits<float>::infinity();
		Weight dy = (unsigned)py.first < _distance.size() ? _distance[py.first] : numeric_limits<float>::infinity();

		Weight wx = dx + px.second;
		Weight wy = dy + py.second;

		return wx > wy;
	}
private:
	const vector<Pair> &_pairs;
	const vector<Weight> &_distance;
	StateId _superfinal;
	float _delta;
};

void ConvertNbestToVector(Lattice &fst, vector<Lattice> *fsts_out);

void NShortestPath(Lattice &ifst, Lattice *ofst, size_t n);
/*
{
	typedef float Weight;
	typedef pair<StateId, Weight> Pair;

	// here ifst must be topologically sort fst
	// don't check
	ofst->DeleteStates();
	if(ifst.Start() == KNoStateId)
		return ;
	LOG_ASSERT(ifst.Start() == 0);

	vector<float> best_cost_and_back(ifst.NumStates());

	StateId superfinal = -1;

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
			Arc *arc = cur_state->GetArc(i);
			float arc_cost = arc->_w;
			StateId nextstate = arc->_to;
			LOG_ASSERT(s < nextstate); // because it's topologically sort
			float next_cost = arc->_w + best_cost_and_back[nextstate];
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
	pairs[start] = Pair(ifst.Start(), 0);

	heap.push_back(start);

	int num_final = 0;
	// here p.first is 'ifst' state, heap is 'ofst' state.
	while(!heap.empty())
	{
		pop_heap(heap.begin(), heap.end(), compare);
		StateId state = heap.back();
		Pair p = pairs[state];
		heap.pop_back();

		while (r.size() <= p.first) 
			r.push_back(0);
		++r[p.first];
		if(num_final == n)
			break;
		if(r[p.first] > n)
			continue;

		// traverse state arc.
		LatticeState * cur_state = ilat->GetState(p.first);
		for(igned i = 0; i < cur_state->GetArcSize(); ++i)
		{
			Arc *arc = cur_state->GetArc(i);
			Weight w = p.second + arc->_w; // it's forward score.
			StateId next = ofst->AddState();
			pairs.push_back(Pair(arc->_to, w));
			ofst->AddArc(state ,Arc(arc->_input, arc->_output, next, arc->_w));
			heap.push_back(next);
			push_heap(heap.begin(), heap.end(), compare);
		}

		if(cur_state->IsFinal())
		{
			ofst->SetFinal(state);
		}
	}

	// delete can not arrive final state.
	Connect(ofst);

	// end ofst it's should be a tree structure.
}
*/
#include "src/util/namespace-end.h"
#endif

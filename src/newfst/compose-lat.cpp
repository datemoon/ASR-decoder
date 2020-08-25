
#include <queue>
#include <unordered_map>
#include <limits> 
#include "src/newfst/compose-lat.h"
#include "src/newfst/connect-fst.h"

#include "src/util/namespace-start.h"
//template<typename Int1, typename Int2 = Int1>
template<typename Int1, typename Int2>
struct PairHasher {  // hashing function for pair<int, long long int>
	size_t operator()(const std::pair<Int1, Int2> &x) const noexcept
	{ // 7853 was chosen at random from a list of primes.
		return (size_t)x.first * 7853 + x.second;
	}
	PairHasher() { }
};
template <typename I>
void ComposeLattice(Lattice *clat, LatticeComposeItf<I> *fst, Lattice *olat, float scale)
{
	typedef std::pair<StateId, I> StatePair;
	typedef std::unordered_map<StatePair, StateId, PairHasher<StateId, I> > MapType;
	typedef typename MapType::iterator IterType;

	LOG_ASSERT(olat != NULL);
	olat->DeleteStates();

	MapType state_map;
	std::queue<StatePair> state_queue;
	// Sets start state in <olat>.
	StateId start_state = olat->AddState();
	StatePair start_pair(clat->Start(), fst->Start());
	olat->SetStart(start_state);
	state_queue.push(start_pair);
	std::pair<IterType, bool> result =
		state_map.insert(std::make_pair(start_pair, start_state));
	LOG_ASSERT(result.second == true);

	// Starts composition here.
	while (!state_queue.empty())
	{ // Gets the first state in the queue.
		StatePair s = state_queue.front();
		StateId s1 = s.first;
		I s2 = s.second;
		state_queue.pop();
		

		// Loops over pair of edges at s1 and s2
		LatticeState *state1 = clat->GetState(s1);
		for(size_t i = 0; i<state1->GetArcSize(); i++)
		{
			LatticeArc *arc1 = state1->GetArc(i);
			//LatticeArc arc2;
			//I next_state2 = 0;
			LatticeWeight lweight;
			Label olabel2=0;
			StateId next_state1 = arc1->_to;
			I next_state2 = 0;
			bool matched = false;
			if(arc1->_output == 0)
			{
				// If the symbol on <arc1> is <epsilon>, we transit to the next state
				// for <clat>, but keep <fst> at the current state.
				matched = true;
				next_state2 = s2;
			}
			else
			{ // Otherwise try to find the matched arc in <det_fst>.
				//matched = fst->GetArc(s2, arc1->_output, &arc2);
				matched = fst->GetArc(s2, arc1->_output, &next_state2, &lweight, &olabel2);
				//if(matched)
				//	next_state2 = arc2._to;
			}

			// If matched arc is found in <fst>, then we have to add new arcs to
			// <olat>
			if(matched)
			{
				StatePair next_state_pair(next_state1, next_state2);
				IterType siter = state_map.find(next_state_pair);
				StateId next_state;

				// Adds composed state to <state_map>.
				if(siter == state_map.end())
				{ // If the composed state has not been created yet, create it.
					next_state = olat->AddState();
					std::pair<const StatePair, StateId> next_state_map(next_state_pair,
							next_state);
					std::pair<IterType, bool> result = state_map.insert(next_state_map);
					LOG_ASSERT(result.second);
					state_queue.push(next_state_pair);
				}
				else
				{
					// If the composed state is already in <state_map>, we can directly
					// use that.
					next_state = siter->second;
				}

				// Because state isn,t save final score,so add final score in arc.
				bool clat_final = clat->Final(next_state1);
				float final_score = 0.0;
				if (clat_final == true)
				{ // Test for whether the final of state s1 was true.
					final_score = fst->Final(next_state2);
					if(final_score == std::numeric_limits<float>::infinity())
						final_score = 0.0;
					else // set final
						olat->SetFinal(next_state,0.0);
				}

				// Adds arc to <olat>.
				if (arc1->_output == 0)
				{
					float am_score = arc1->_w.Value2();
					float lm_score = arc1->_w.Value1() + final_score * scale;
					olat->AddArc(state_map[s], LatticeArc(arc1->_input,
								0, next_state, LatticeWeight(lm_score, am_score)));
				}
				else
				{
					float am_score = arc1->_w.Value2() + lweight.Value2();
					float lm_score = arc1->_w.Value1() + (lweight.Value1() + final_score) * scale;
					olat->AddArc(state_map[s], LatticeArc(arc1->_input, olabel2,
							   	next_state, LatticeWeight(lm_score, am_score)));
				}
			}
		}
	}
	Connect(olat);
}

// instantiate templates
template void ComposeLattice(Lattice *clat,
		LatticeComposeItf<unsigned long long int> *fst, Lattice *olat, float scale);
template void ComposeLattice(Lattice *clat,
		LatticeComposeItf<long long int> *fst, Lattice *olat, float scale);
template void ComposeLattice(Lattice *clat,
		LatticeComposeItf<int> *fst, Lattice *olat, float scale);
template void ComposeLattice(Lattice *clat,
		LatticeComposeItf<unsigned int> *fst, Lattice *olat, float scale);
#include "src/util/namespace-end.h"

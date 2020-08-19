#ifndef __HMM_TRANSITION_MODEL_H__
#define __HMM_TRANSITION_MODEL_H__

#include <string>
#include <assert.h>
#include <vector>
#include "src/hmm/hmm-topology.h"

#include "src/util/namespace-start.h"

class TransitionModel 
{
public:
	TransitionModel() { }

	void Read(std::istream &is, bool binary, bool have_probs = true);  // note, no symbol table: topo object always read/written w/o symbols.


	// add pdfid + 1,for add block nn output
	void ExchangePdfId()
	{
		for(int i = 0 ; i < static_cast<int>(_triples.size()); ++i)
		{
			_triples[i]._pdf += 1;
		}
	}

	// write transitionmodel 
	void Write(std::ostream &os, bool binary, bool have_probs = true);

	const HmmTopology &GetTopo() const { return _topo; }

	// Returns the total number of transition-states (note, these are one-based).
	int NumTransitionStates() const { return _triples.size(); }

	// Transition-parameter-getting functions:
	float GetTransitionProb(int trans_id) const;
	float GetTransitionLogProb(int trans_id) const;

	int NumTransitionIndices(int trans_state) const;

	int TripleToTransitionState(int phone, int hmm_state, int pdf) const;
	int PairToTransitionId(int trans_state, int trans_index) const;
	int TransitionIdToTransitionState(int trans_id) const;
	int TransitionIdToTransitionIndex(int trans_id) const;
	int TransitionStateToPhone(int trans_state) const;
	int TransitionStateToHmmState(int trans_state) const;
	int TransitionStateToPdf(int trans_state) const;
	int SelfLoopOf(int trans_state) const;  // returns the self-loop transition-id, or zero if

	// this state doesn't have a self-loop.
	inline int TransitionIdToPdf(int trans_id) const
	{
		// If a lot of time is spent here we may create an extra array
		// to handle this.
	
		assert(static_cast<size_t>(trans_id) < _id2state.size() &&
				"Likely graph/model mismatch (graph built from wrong model?)");
		int trans_state = _id2state[trans_id];
		return _triples[trans_state-1]._pdf;
	}
	
	int TransitionIdToPhone(int trans_id) const;
	int TransitionIdToPdfClass(int trans_id) const;
	int TransitionIdToHmmState(int trans_id) const;

	// Returns the total number of transition-ids (note, these are one-based).
	inline int NumTransitionIds() const { return _id2state.size()-1; }
	

	// NumPdfs() actually returns the highest-numbered pdf we ever saw, plus one.
	// In normal cases this should equal the number of pdfs in the system, but if you
	// initialized this object with fewer than all the phones, and it happens that
	// an unseen phone has the highest-numbered pdf, this might be different.
	int NumPdfs() const { return _num_pdfs; }

	// This loops over the triples and finds the highest phone index present. If
	// the FST symbol table for the phones is created in the expected way, i.e.:
	// starting from 1 (<eps> is 0) and numbered contiguously till the last phone,
	// this will be the total number of phones.
	int NumPhones() const;
	
	void Check(bool have_prob = true) const;

private:

	void ComputeDerived();  // called from constructor and Read function: computes state2id_ and id2state_.

	void ComputeDerivedOfProbs();  // computes quantities derived from log-probs (currently just

	struct Triple 
	{
		int _phone;
		int _hmm_state;
		int _pdf;
		Triple() { }
		Triple(int phone, int hmm_state, int pdf):
			_phone(phone), _hmm_state(hmm_state), _pdf(pdf) { }
		bool operator < (const Triple &other) const {
			if (_phone < other._phone) return true;
			else if (_phone > other._phone) return false;
			else if (_hmm_state < other._hmm_state) return true;
			else if (_hmm_state > other._hmm_state) return false;
			else return _pdf < other._pdf;
		}
		bool operator == (const Triple &other) const 
		{
			return (_phone == other._phone && _hmm_state == other._hmm_state
					&& _pdf == other._pdf);
		}
	};

	HmmTopology _topo;

	// Triples indexed by transition state minus one;
	// the triples are in sorted order which allows us to do the reverse mapping from
	// triple to transition state
	std::vector<Triple> _triples;

	// Gives the first transition_id of each transition-state; indexed by
	// the transition-state.  Array indexed 1..num-transition-states+1 (the last one
	// is needed so we can know the num-transitions of the last transition-state.
	std::vector<int> _state2id;

	// For each transition-id, the corresponding transition
	// state (indexed by transition-id).
	std::vector<int> _id2state;

	// For each transition-id, the corresponding log-prob.  Indexed by transition-id.
	std::vector<float> _log_probs;

	// For each transition-state, the log of (1 - self-loop-prob).  Indexed by
	// transition-state.
	std::vector<float> _non_self_loop_log_probs;

	// This is actually one plus the highest-numbered pdf we ever got back from the
	// tree (but the tree numbers pdfs contiguously from zero so this is the number
	// of pdfs).
	int _num_pdfs;
};

#include "src/util/namespace-end.h"

#endif

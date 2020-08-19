
#include <iostream>
#include <assert.h>
#include <math.h>
#include <algorithm>
#include "src/hmm/transition-model.h"
#include "src/util/io-funcs.h"
#include "src/util/read-vector.h"

#include "src/util/namespace-start.h"

void TransitionModel::Read(std::istream &is, bool binary , bool have_probs) 
{
	ExpectToken(is, binary, "<TransitionModel>");
	_topo.Read(is, binary);
	ExpectToken(is, binary, "<Triples>");
	int size=0;
	is >> size;
	_triples.resize(size);

	for(int i=0;i<size;++i)
	{
		is >> _triples[i]._phone;
		is >> _triples[i]._hmm_state;
		is >> _triples[i]._pdf;
	}
	ExpectToken(is, binary, "</Triples>");
	ComputeDerived();
	if(have_probs == false)
	{
		ExpectToken(is, binary, "</TransitionModel>");
		Check(false);
		return ;
	}
	ExpectToken(is, binary, "<LogProbs>");
	ReadVector(is , _log_probs , binary);
	ExpectToken(is, binary, "</LogProbs>");
	ExpectToken(is, binary, "</TransitionModel>");
	ComputeDerivedOfProbs();
	Check();
}

void TransitionModel::Write(std::ostream &os, bool binary, bool have_probs)
{
	WriteToken(os, binary, "<TransitionModel>");
	if (!binary)
	   	os << "\n";
	_topo.Write(os, binary);
	WriteToken(os, binary, "<Triples>");
	WriteBasicType(os, binary, static_cast<int>(_triples.size()));
	if (!binary) 
		os << "\n";
	for (int i = 0; i < static_cast<int> (_triples.size()); i++)
	{
		WriteBasicType(os, binary, _triples[i]._phone);
		WriteBasicType(os, binary, _triples[i]._hmm_state);
		WriteBasicType(os, binary, _triples[i]._pdf);
		if (!binary)
			os << "\n";
	}
	WriteToken(os, binary, "</Triples>");
	if (!binary) 
		os << "\n";
	if(have_probs == true)
	{
		WriteToken(os, binary, "<LogProbs>");
		if (!binary) 
			os << "\n";
		os << "[ ";
		for(int i = 0; i  < static_cast<int>(_log_probs.size()); ++i)
			os << _log_probs[i] << " ";
		os << "]" << "\n";
		WriteToken(os, binary, "</LogProbs>");
		if (!binary)
		   	os << "\n";
	}
	WriteToken(os, binary, "</TransitionModel>");
	if (!binary) 
		os << "\n";
}

void TransitionModel::ComputeDerived()
{
	_state2id.resize(_triples.size()+2); // indexed by transition-state, which
	// is one based, but also an entry for one past end of list.
	
	int cur_transition_id = 1;
	_num_pdfs = 0;
	for (int tstate = 1;
			tstate <= static_cast<int>(_triples.size()+1);  // not a typo.
			tstate++)
	{
		_state2id[tstate] = cur_transition_id;
		if (static_cast<size_t>(tstate) <= _triples.size())
		{
			int phone = _triples[tstate-1]._phone,
				hmm_state = _triples[tstate-1]._hmm_state,
				pdf = _triples[tstate-1]._pdf;
			_num_pdfs = std::max(_num_pdfs, 1+pdf);
			const HmmTopology::HmmState &state = _topo.TopologyForPhone(phone)[hmm_state];
			int my_num_ids = static_cast<int>(state._transitions.size());
			cur_transition_id += my_num_ids; // # trans out of this state.
		}
	}

	_id2state.resize(cur_transition_id);   // cur_transition_id is #transition-ids+1.

	for (int tstate = 1; tstate <= static_cast<int>(_triples.size()); tstate++)
	{
		for (int tid = _state2id[tstate]; tid < _state2id[tstate+1]; tid++)
			_id2state[tid] = tstate;
	}
}

int TransitionModel::PairToTransitionId(int trans_state, int trans_index) const {
	assert(static_cast<size_t>(trans_state) <= _triples.size());
	assert(trans_index < _state2id[trans_state+1] - _state2id[trans_state]);
	return _state2id[trans_state] + trans_index;
}


int TransitionModel::SelfLoopOf(int trans_state) const
{
	assert(static_cast<size_t>(trans_state-1) < _triples.size());
	const Triple &triple = _triples[trans_state-1];
	// or zero if does not exist.
	int phone = triple._phone, hmm_state = triple._hmm_state;
	const HmmTopology::TopologyEntry &entry = _topo.TopologyForPhone(phone);
	assert(static_cast<size_t>(hmm_state) < entry.size());

	for (int trans_index = 0;
			trans_index < static_cast<int>(entry[hmm_state]._transitions.size());
			trans_index++)
	{
		if (entry[hmm_state]._transitions[trans_index].first == hmm_state)
			return PairToTransitionId(trans_state, trans_index);
	}
	return 0;  // invalid transition id.
}

void TransitionModel::ComputeDerivedOfProbs()
{
	_non_self_loop_log_probs.resize(NumTransitionStates()+1);  // this array indexed
	//  by transition-state with nothing in zeroth element.
	for (int tstate = 1; tstate <= NumTransitionStates(); tstate++)
	{
		int tid = SelfLoopOf(tstate);
		if (tid == 0)
	   	{  // no self-loop
			_non_self_loop_log_probs[tstate] = 0.0;  // log(1.0)
		}
		else
		{
			float self_loop_prob = expf(GetTransitionLogProb(tid)),
	  			  non_self_loop_prob = 1.0 - self_loop_prob;
			if (non_self_loop_prob <= 0.0)
			{
				std::cerr << "ComputeDerivedOfProbs(): non-self-loop prob is " << non_self_loop_prob << std::endl;
				non_self_loop_prob = 1.0e-10;  // just so we can continue...
			}
			_non_self_loop_log_probs[tstate] = log(non_self_loop_prob); // will be negative.
		}
	}
}

float TransitionModel::GetTransitionLogProb(int trans_id) const 
{
	return _log_probs[trans_id];
}

int TransitionModel::NumTransitionIndices(int trans_state) const 
{
	assert(static_cast<size_t>(trans_state) <= _triples.size());
	return static_cast<int>(_state2id[trans_state+1]-_state2id[trans_state]);
}

void TransitionModel::Check(bool have_probs) const 
{
	assert(NumTransitionIds() != 0 && NumTransitionStates() != 0);
	{
		int sum = 0;
		for (int ts = 1; ts <= NumTransitionStates(); ts++) 
			sum += NumTransitionIndices(ts);
		assert(sum == NumTransitionIds());
	}
	for (int tid = 1; tid <= NumTransitionIds(); tid++) 
	{
		int tstate = TransitionIdToTransitionState(tid),
			  index = TransitionIdToTransitionIndex(tid);
		assert(tstate > 0 && tstate <=NumTransitionStates() && index >= 0);
		assert(tid == PairToTransitionId(tstate, index));
		int phone = TransitionStateToPhone(tstate),
			  hmm_state = TransitionStateToHmmState(tstate),
			  pdf = TransitionStateToPdf(tstate);
		assert(tstate == TripleToTransitionState(phone, hmm_state, pdf));
		if(have_probs)
			assert(_log_probs[tid] <= 0.0 && _log_probs[tid] - _log_probs[tid] == 0.0);
		// checking finite and non-positive (and not out-of-bounds).
	}
}


int TransitionModel::TripleToTransitionState(int phone, int hmm_state, int pdf) const 
{
	Triple triple(phone, hmm_state, pdf);
	// Note: if this ever gets too expensive, which is unlikely, we can refactor
	// this code to sort first on pdf, and then index on pdf, so those
	// that have the same pdf are in a contiguous range.
	std::vector<Triple>::const_iterator iter =
		std::lower_bound(_triples.begin(), _triples.end(), triple);
	if (iter == _triples.end() || !(*iter == triple)) {
		std::cerr << "TransitionModel::TripleToTransitionState, triple not found."
			<< " (incompatible tree and model?)" << std::endl;
	}
	// triples_ is indexed by transition_state-1, so add one.
	return static_cast<int>((iter - _triples.begin())) + 1;
}
		
int TransitionModel::TransitionIdToTransitionState(int trans_id) const 
{
	assert(trans_id != 0 &&  static_cast<size_t>(trans_id) < _id2state.size());
	return _id2state[trans_id];
}

int TransitionModel::TransitionIdToTransitionIndex(int trans_id) const 
{
	assert(trans_id != 0 && static_cast<size_t>(trans_id) < _id2state.size());
	return trans_id - _state2id[_id2state[trans_id]];
}

int TransitionModel::TransitionStateToPhone(int trans_state) const 
{
	assert(static_cast<size_t>(trans_state) <= _triples.size());
	return _triples[trans_state-1]._phone;
}

int TransitionModel::TransitionStateToPdf(int trans_state) const 
{
	assert(static_cast<size_t>(trans_state) <= _triples.size());
	return _triples[trans_state-1]._pdf;
}

int TransitionModel::TransitionStateToHmmState(int trans_state) const 
{
	assert(static_cast<size_t>(trans_state) <= _triples.size());
	return _triples[trans_state-1]._hmm_state;
}

int TransitionModel::TransitionIdToPhone(int trans_id) const 
{
	assert(trans_id != 0 && static_cast<size_t>(trans_id) < _id2state.size());
	int trans_state = _id2state[trans_id];
	return _triples[trans_state-1]._phone;
}

int TransitionModel::TransitionIdToPdfClass(int trans_id) const 
{
	assert(trans_id != 0 && static_cast<size_t>(trans_id) < _id2state.size());
	int trans_state = _id2state[trans_id];

	const Triple &t = _triples[trans_state-1];
	const HmmTopology::TopologyEntry &entry = _topo.TopologyForPhone(t._phone);
	assert(static_cast<size_t>(t._hmm_state) < entry.size());
	return entry[t._hmm_state]._pdf_class;
}

#include "src/util/namespace-end.h"

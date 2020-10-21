#ifndef __LATTICE_DETERMINIZE_H__
#define __LATTICE_DETERMINIZE_H__

// this file process lattice determinize
// word input 
#include <vector>
#include <deque>
#include <climits>
#include <array>
#include <unordered_set>
#include <unordered_map>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include "src/newfst/optimize-fst.h"
#include "src/newfst/lattice-fst.h"
#include "src/newfst/invert.h"
#include "src/newfst/lattice-determinize-api.h"

using namespace std;
using std::unordered_map;

#include "src/util/namespace-start.h"
#ifndef FLOAT_INF
#define FLOAT_INF std::numeric_limits<float>::infinity()
#endif

// This class maps back and forth from/to integer id's to sequences of strings.
// used in determinization algorithm.  It is constructed in such a way that
// finding the string-id of the successor of (string, next-label) has constant time.

// Note: class IntType, typically int32, is the type of the element in the
// string (typically a template argument of the CompactLatticeWeightTpl).

template<class IntType> class LatticeStringRepository
{
public:
	struct Entry
	{
		const Entry *_parent; // NULL for empty string
		IntType _i;

		inline bool operator == (const Entry &other) const
		{
			return (_parent == other._parent && _i == other._i);
		}
		Entry() { }
		Entry(const Entry &e):_parent(e._parent), _i(e._i) { }
	};
	// Note: all Entry* pointers returned in function calls are
	// owned by the repository itself, not by the caller!
	
	// Interface guarantees empty string is NULL.
	inline const Entry *EmptyString() { return NULL; }

	// Returns string of "parent" with i appended.  Pointer
	// owned by repository
	const Entry *Successor(const Entry *parent, IntType i)
	{
		_new_entry->_parent = parent;
		_new_entry->_i = i;

		std::pair<typename SetType::iterator, bool> pr = _set.insert(_new_entry);
		if(pr.second)
		{
			// Was successfully inserted (was not there).  We need to
			// replace the element we inserted, which resides on the
			// stack, with one from the heap.
			const Entry *ans = _new_entry;
			_new_entry = new Entry();
			return ans;
		}
		else
		{
			// Was not inserted because an equivalent Entry already
			// existed.
			return *pr.first;
		}
	}

	const Entry *Concatenate(const Entry *a, const Entry *b)
	{
		if(a == NULL)
			return b;
		else if (b == NULL)
			return a;
		vector<IntType> v;
		ConvertToVector(b, &v);
		const Entry *ans = a;
		for(size_t i = 0; i < v.size(); ++i)
			ans = Successor(ans, v[i]);
		return ans;
	}

	const Entry *CommonPrefix(const Entry *a, const Entry *b)
	{
		vector<IntType> a_vec, b_vec;
		ConvertToVector(a, &a_vec);
		ConvertToVector(b, &b_vec);
		const Entry *ans = NULL;
		for(size_t i = 0; i < a_vec.size() && i < b_vec.size() &&
			   	a_vec[i] == b_vec[i]; ++i)
			ans = Successor(ans, a_vec[i]);
		return ans;
	}
	// removes any elements from b that are not part of
	// a common prefix with a.
	void ReduceToCommonPrefix(const Entry *a, vector<IntType> *b)
	{
		size_t a_size = Size(a), b_size = b->size();
		while(a_size > b_size)
		{
			a = a->_parent;
			a_size--;
		}
		if(b_size > a_size)
			b_size = a_size;
		typename vector<IntType>::iterator b_begin = b->begin();
		while (a_size != 0)
		{
			if(a->_i != *(b_begin +a_size - 1))
				b_size = a_size -1;
			a = a->_parent;
			a_size--;
		}
		if(b_size != b->size())
			b->resize(b_size);
	}

	// removes the first n elements of a.
	const Entry *RemovePrefix(const Entry *a, size_t n)
	{
		if( n == 0)
			return a;
		vector<IntType> a_vec;
		ConvertToVector(a, &a_vec);
		LOG_ASSERT(a_vec.size() >= n);
		const Entry *ans = NULL;
		for(size_t i=n; i < a_vec.size(); ++i)
			ans = Successor(ans, a_vec[i]);
		return ans;
	}

	// Returns true if a is a prefix of b.  If a is prefix of b,
	// time taken is |b| - |a|.  Else, time taken is |b|.
	bool IsPrefixOf(const Entry *a, const Entry *b) const
	{
		if(a== NULL)
			return true;
		if(a==b)
			return true;
		if(b == NULL)
			return false;
		return IsPrefixOf(a, b->parent);
	}

	inline size_t Size(const Entry *entry) const
	{
		size_t ans = 0;
		while(entry != NULL)
		{
			ans++;
			entry = entry->_parent;
		}
		return ans;
	}

	void ConvertToVector(const Entry *entry, vector<IntType> *out) const
	{
		size_t length = Size(entry);
		out->resize(length);
		if(entry != NULL)
		{
			typename vector<IntType>::reverse_iterator iter = out->rbegin();
			while(entry != NULL)
			{
				*iter = entry->_i;
				entry = entry->_parent;
				++iter;
			}
		}
	}

	const Entry *ConvertFromVector(vector<IntType> &vec)
	{
		const Entry *e = NULL;
		for(size_t i = 0; i < vec.size(); ++i)
		{
			e = Successor(e, vec[i]);
		}
		return e;
	}

	LatticeStringRepository() { _new_entry = new Entry; }

	void Destory()
	{
		for(typename SetType::iterator iter = _set.begin();
				iter != _set.end(); ++iter)
			delete *iter;
		SetType tmp;
		tmp.swap(_set);
		if(_new_entry)
		{
			delete _new_entry;
			_new_entry = NULL;
		}
	}

	// Rebuild will rebuild this object, guaranteeing only
	// to preserve the Entry values that are in the vector pointed
	// to (this list does not have to be unique).  The point of
	// this is to save memory.
	void Rebuild(const std::vector<const Entry*> &to_keep)
	{
		SetType tmp_set;
		for(typename std::vector<const Entry*>::const_iterator
				iter = to_keep.begin();
				iter != to_keep.end(); ++iter)
		{
			RebuildHelper(*iter, &tmp_set);
		}
		// Now delete all elems not in tmp_set
		for(typename SetType::iterator iter = _set.begin();
				iter != _set.end(); ++iter)
		{
			if(tmp_set.count(*iter) == 0)
				delete (*iter); // delete the Entry; not needed.
		}
		_set.swap(tmp_set);
	}

	~LatticeStringRepository() { Destroy(); }

	int MemSize() const
	{
		return _set.size() * sizeof(Entry) * 2; // this is a lower  bound
		// on the size this structure might take.
	}

	void Destroy()
	{
		for (typename SetType::iterator iter = _set.begin();
				iter != _set.end();
				++iter)
			delete *iter;
		SetType tmp;
		tmp.swap(_set);
		if (_new_entry) 
		{
			delete _new_entry;
			_new_entry = NULL;
		}
	}

private:
	class EntryKey
	{ // Hash function object
	public:
		inline size_t operator()(const Entry *entry) const
		{
			size_t prime = 49109;
			return static_cast<size_t>(entry->_i) + prime * reinterpret_cast<size_t>(entry->_parent);
		}
	};

	class EntryEqual
	{
	public:
		inline bool operator()(const Entry *e1, const Entry *e2) const
		{
			return (*e1 == *e2);
		}
	};

	typedef unordered_set<const Entry*, EntryKey, EntryEqual> SetType;

	void RebuildHelper(const Entry* to_add, SetType *tmp_set)
	{
		while(true)
		{
			if(to_add == NULL)
				return ;
			typename SetType::iterator iter = tmp_set->find(to_add);
			if(iter == tmp_set->end())
			{ // not in tmp_set
				tmp_set->insert(to_add);
				to_add = to_add->_parent; // and loop.
			}
			else
				return ;
		}
	}
	Entry *_new_entry; // We always have a pre-allocated Entry ready to use,
	                   // to avoid unnecessary news and deletes.

	SetType _set;
};

template<class Weight, class IntType> class LatticeDeterminizer
{
public:
	// Output to Gallic acceptor (so the strings go on weights, and there is a 1-1 correspondence
	// between our states and the states in ofst.  If destroy == true, release memory as we go
	// (but we cannot output again).
	
	void OutputNoolabel(Lattice *ofst, bool destory = true)
	{
		OutputStateId nStates = static_cast<OutputStateId>(_output_arcs.size());
		ofst->DeleteStates();
		ofst->SetStart(kNoStateId);
		if(nStates == 0)
		{
			return;
		}
		if(destory)
			FreeMostMemory();

		// Add basic states-- but we will add extra ones to account for strings on output
		for(OutputStateId s = 0; s<nStates; ++s)
		{
			OutputStateId news = ofst->AddState();
			LOG_ASSERT(news == s);
		}
		ofst->SetStart(0);
		for(OutputStateId this_state_id = 0; this_state_id < nStates; ++this_state_id)
		{
			vector<TempArc> &this_vec(_output_arcs[this_state_id]);
			typename vector<TempArc>::const_iterator iter = this_vec.begin(),end = this_vec.end();
			for(;iter != end; ++iter)
			{
				const TempArc &temp_arc(*iter);
				vector<Label> seq;
				_repository.ConvertToVector(temp_arc._string, &seq);
				if (temp_arc._nextstate == kNoStateId)
				{
					OutputStateId cur_state = this_state_id;
					{
						OutputStateId next_state = ofst->AddState();
						LatticeArc arc;
						arc._to = next_state;
						arc._w = temp_arc._weight;
						arc._input = 0;  // eps
						arc._output = 0;
						ofst->AddArc(cur_state, arc);
						cur_state = next_state;
					}
					ofst->SetFinal(cur_state);
				}
				else
				{ // Really an arc.
					OutputStateId cur_state = this_state_id;
					{
						LatticeArc arc;
						arc._to = temp_arc._nextstate;
						arc._w = temp_arc._weight;
						arc._input = temp_arc._ilabel;
						arc._output = 0;
						ofst->AddArc(cur_state,arc);
					}
				}
			}
			// Free up memory. Do this inside the loop as ofst is also allocating memory
			if(destory)
			{
				vector<TempArc> temp;
				temp.swap(this_vec);
			}
		}
		if(destory)
		{
			vector<vector<TempArc> > temp;
			temp.swap(_output_arcs);
			_repository.Destory();
		}

	}

	// Output to standard FST with Weight as its weight type.  We will create extra
	// states to handle sequences of symbols on the output.  If destroy == true,
	// release memory as we go (but we cannot output again).
	void Output(Lattice *ofst, bool destory = true)
	{ // Outputs to standard fst.
		OutputStateId nStates = static_cast<OutputStateId>(_output_arcs.size());
		ofst->DeleteStates();
		if(nStates == 0)
		{
			ofst->SetStart(kNoStateId);
			return ;
		}
		if(destory)
			FreeMostMemory();
		// Add basic states-- but we will add extra ones to account for strings on output
		for(OutputStateId s = 0; s<nStates; ++s)
		{
			OutputStateId news = ofst->AddState();
			LOG_ASSERT(news == s);
		}

		ofst->SetStart(0);
		for(OutputStateId this_state_id = 0; this_state_id < nStates; ++this_state_id)
		{
			vector<TempArc> &this_vec(_output_arcs[this_state_id]);
	//		OutputStateId &this_state = *(_output_states[this_state_id]);
//			vector<TempArc> &this_vec(this_state.arcs);

			typename vector<TempArc>::const_iterator iter = this_vec.begin(), end = this_vec.end();
			for(;iter != end; ++iter)
			{
				const TempArc &temp_arc(*iter);
				vector<Label> seq;
				_repository.ConvertToVector(temp_arc._string, &seq);

				if (temp_arc._nextstate == kNoStateId)
				{ // Really a final weight.
					// Make a sequence of states going to a final state, with the strings
					// as labels.  Put the weight on the first arc.
					OutputStateId cur_state = this_state_id;
					for (size_t i = 0; i < seq.size(); i++)
					{
						OutputStateId next_state = ofst->AddState();
						LatticeArc arc;
						arc._to = next_state;
						arc._w = (i == 0 ? temp_arc._weight : Weight::One());
						arc._input = 0;  // eps
						arc._output = seq[i];
						ofst->AddArc(cur_state, arc);
						cur_state = next_state;
					}

					if(seq.size() == 0)
					{
						OutputStateId next_state = ofst->AddState();
						LatticeArc arc;
						arc._to = next_state;
						arc._w = Weight::One();
						arc._input = 0;  // eps
						arc._output = 0;
						ofst->AddArc(cur_state, arc);
						cur_state = next_state;
					}
					ofst->SetFinal(cur_state);
				}
				else
				{ // Really an arc.
					OutputStateId cur_state = this_state_id;
					// Have to be careful with this integer comparison (i+1 < seq.size()) because unsigned.
					// i < seq.size()-1 could fail for zero-length sequences.
					for( size_t i = 0;i+1 < seq.size();++i)
					{ // for all but the last element of seq, create new state.
						OutputStateId next_state = ofst->AddState();
						LatticeArc arc;
						arc._to = next_state;
						arc._w = (i == 0 ? temp_arc._weight : Weight::One());
						arc._input = (i == 0 ? temp_arc._ilabel : 0); // put ilabel on first element of seq.
						arc._output = seq[i];
						ofst->AddArc(cur_state,arc);
						cur_state = next_state;
					}
					// Add the final arc in the sequence.
					LatticeArc arc;
					arc._to = temp_arc._nextstate;
					arc._w = (seq.size() <= 1 ? temp_arc._weight : Weight::One());
					arc._input = (seq.size() <= 1 ? temp_arc._ilabel : 0);
					arc._output = (seq.size() > 0 ? seq.back() : 0);
					ofst->AddArc(cur_state,arc);
				}
			}
			// Free up memory. Do this inside the loop as ofst is also allocating memory
			if(destory)
			{
				vector<TempArc> temp; 
				temp.swap(this_vec);
			}
		}
		if(destory)
		{
			vector<vector<TempArc> > temp;
			temp.swap(_output_arcs);
			_repository.Destory();
		}
	}
public:
	// Initializer.  After initializing the object you will typically
	// call Determinize() and then call one of the Output functions.
	// Note: ifst.Copy() will generally do a
	// shallow copy.  We do it like this for memory safety, rather than
	// keeping a reference or pointer to ifst_.
	LatticeDeterminizer(Lattice *ifst,
			DeterminizeLatticeOptions opts):
		_num_arcs(0), _num_elems(0), _ifst(ifst), _opts(opts),
		_equal(_opts._delta), _determinized(false),
		_minimal_hash(3, _hasher, _equal), _initial_hash(3, _hasher, _equal)
	{ }

	// frees all except output_arcs_, which contains the important info
	// we need to output the FST.
	void FreeMostMemory()
	{
		if(_ifst  && false)
		{
			delete _ifst;
			_ifst = NULL;
		}
		for(typename MinimalSubsetHash::iterator iter = _minimal_hash.begin();
				iter != _minimal_hash.end(); ++iter)
			delete iter->first;

		{
			MinimalSubsetHash tmp ;
			tmp.swap(_minimal_hash);
		}

		for(typename InitialSubsetHash::iterator iter = _initial_hash.begin();
				iter != _initial_hash.end(); ++iter)
			delete iter->first;
		
		{
			InitialSubsetHash tmp;
			tmp.swap(_initial_hash);
		}
		
		{
			vector<vector<Element>* >  output_states_tmp;
			output_states_tmp.swap(_output_states);
		}

		{
			vector<char> tmp; 
			tmp.swap(_isymbol_or_final);
		}
		
		{
			vector<OutputStateId> tmp;
			tmp.swap(_queue);
		}

		{
			vector<pair<Label, Element> > tmp;
			tmp.swap(_all_elems_tmp);
		}
	}

	~LatticeDeterminizer()
	{
		FreeMostMemory(); // rest is deleted by destructors.
	}

	// Returns true on success.  Can fail for out-of-memory
	// or max-states related reasons.
	bool Determinize(bool *debug_ptr)
	{
		LOG_ASSERT(!_determinized);
		// This determinizes the input fst but leaves it in the "special format"
		// in "output_arcs_".  Must be called after Initialize().  To get the
		// output, call one of the Output routines.
		try
		{
			InitializeDeterminization(); // some start-up tasks.
			while(!_queue.empty())
			{
				OutputStateId out_state = _queue.back();
				_queue.pop_back();
				ProcessState(out_state);
				if(debug_ptr && *debug_ptr) 
					Debug();  // will exit.
				if(!CheckMemoryUsage())
					return false;
			}
		}
		catch (std::bad_alloc)
		{
			int repo_size = _repository.MemSize(),
				arcs_size = _num_arcs * sizeof(TempArc),
				elems_size = _num_elems * sizeof(Element),
				total_size = repo_size + arcs_size + elems_size;
			LOG_WARN << "Memory allocation error doing lattice determinization; using "
				<< total_size << " bytes (max = " << _opts._max_mem
				<< " (repo,arcs,elems) = ("
				<< repo_size << "," << arcs_size << "," << elems_size << ")";
			return (_determinized = false);
		}
		catch(std::runtime_error)
		{
			LOG_WARN << "Caught exception doing lattice determinization" ;
			return (_determinized = false);
		}
		return true;
	}



private:

	typedef typename LatticeArc::StateId InputStateId;
	typedef typename LatticeArc::StateId OutputStateId;

	typedef LatticeStringRepository<IntType> StringRepositoryType;
	typedef const typename StringRepositoryType::Entry* StringId;

	// Element of a subset [of original states]
	struct Element
	{
		StateId _state; // use StateId as this is usually InputStateId but in one case OutputStateId.
		StringId _string;
		Weight _weight;

		bool operator != (const Element &other) const
		{
			return (_state != other._state || _string != other._string ||
					_weight != other._weight);
		}

		// This operator is only intended to support sorting in EpsilonClosure()
		bool operator < (const Element &other) const
		{
			return _state < other._state;
		}
	};

	// Arcs in the format we temporarily create in this class (a representation, essentially of a Gallic Fst).
	struct TempArc
	{
		Label _ilabel;
		StringId _string; // Look it up in the StringRepository, it's a sequence of Labels.
		OutputStateId _nextstate; // or kNoState for final weights.
		Weight _weight;
	};

	// Hashing function used in hash of subsets.
	// A subset is a pointer to vector<Element>.
	// The Elements are in sorted order on state id, and without repeated states.
	// Because the order of Elements is fixed, we can use a hashing function that is
	// order-dependent.  However the weights are not included in the hashing function--
	// we hash subsets that differ only in weight to the same key.  This is not optimal
	// in terms of the O(N) performance but typically if we have a lot of determinized
	// states that differ only in weight then the input probably was pathological in some way,
	// or even non-determinizable.
	//   We don't quantize the weights, in order to avoid inexactness in simple cases.
	// Instead we apply the delta when comparing subsets for equality, and allow a small
	// difference.
	
	class SubsetKey
	{
	public:
		size_t operator ()(const vector<Element> * subset) const
		{ // hashes only the state and string.
			size_t hash = 0, factor = 1;
			for(typename vector<Element>::const_iterator iter = subset->begin();
					iter != subset->end(); ++iter)
			{
				hash *= factor;
				hash += iter->_state + reinterpret_cast<size_t>(iter->_string);
				factor *= 23531;  // these numbers are primes.
			}
			return hash;
		}
	};

	// This is the equality operator on subsets.  It checks for exact match on state-id
	// and string, and approximate match on weights.
	class SubsetEqual
	{
	public:
		bool operator ()(const vector<Element> *s1, const vector<Element> *s2) const
		{
			size_t sz = s1->size();
			LOG_ASSERT(sz >=0);
			if(sz != s2->size())
				return false;
			typename vector<Element>::const_iterator iter1 = s1->begin(),
					 iter1_end = s1->end(), iter2 = s2->begin();
			for(; iter1 < iter1_end; ++iter1, ++iter2)
			{
				if(iter1->_state != iter2->_state ||
						iter1->_string != iter2->_string ||
						! ApproxEqual(iter1->_weight, iter2->_weight, _delta))
						//fabs(iter1->_weight - iter2->_weight) > _delta)
					return false;
			}
			return true;
		}
		float _delta;
		SubsetEqual(float delta): _delta(delta) {}
		SubsetEqual(): _delta(1e-5) {}
	};

	// Operator that says whether two Elements have the same states.
	// Used only for debug.
	class SubsetEqualStates
	{
	public:
		bool operator ()(const vector<Element> *s1, const vector<Element> *s2) const
		{
			size_t sz = s1->size();
			LOG_ASSERT(sz >=0);
			if(sz != s2->size())
				return false;
			typename vector<Element>::const_iterator iter1 = s1->begin(),
					 iter1_end = s1->end(), iter2=s2->begin();
			for(;iter1 < iter1_end; ++iter1, ++iter2)
			{
				if(iter1->_state != iter2->_state)
					return false;
			}
			return true;
		}
	};

	// Define the hash type we use to map subsets (in minimal
	// representation) to OutputStateId.
	typedef unordered_map<const vector<Element>*, OutputStateId,
			SubsetKey, SubsetEqual> MinimalSubsetHash;

	// Define the hash type we use to map subsets (in initial
	// representation) to OutputStateId, together with an
	// extra weight. [note: we interpret the Element.state in here
	// as an OutputStateId even though it's declared as InputStateId;
	// these types are the same anyway].
	typedef unordered_map<const vector<Element>*, Element,
			SubsetKey, SubsetEqual> InitialSubsetHash;



	// "less than" operator for pair<Label, Element>.   Used in ProcessTransitions.
	// Lexicographical order, which only compares the state when ordering the
	// "Element" member of the pair.
	class PairComparator
	{
	public:
		inline bool operator () (const pair<Label, Element> &p1,
				const pair<Label, Element> &p2)
		{
			if(p1.first < p2.first)
				return true;
			else if(p1.first > p2.first)
				return false;
			else
				return p1.second._state < p2.second._state;
		}
	};


	// maps from output state to minimal representation [normalized].
	// View pointers as owned in _minimal_hash
	vector<vector<Element>* > _output_states; 

	// essentially an FST in our format.
	vector<vector<TempArc> > _output_arcs;

	// keep track of memory usage: number of arcs in _output_arcs
	int _num_arcs;

	// keep track of memory usage: number of elems in _output_states
	int _num_elems;

//	const Fst<Arc> *_ifst;
	Lattice *_ifst;
	DeterminizeLatticeOptions _opts;

	SubsetKey _hasher; // object that computes keys-- has no data members.
	SubsetEqual _equal; // object that compares subsets-- only data member is _delta.
	bool _determinized; // set to true when user called Determinize(); used to make

	// sure this object is used correctly.
	// hash from Subset to OutputStateId.  Subset is "minimal epresentation" (only include final and states and states with
	// nonzero ilabel on arc out of them.  Owns the pointers in its keys.
	MinimalSubsetHash _minimal_hash; 

	// hash from Subset to Element, which represents the OutputStateId together
	// with an extra weight and string.  Subset is "initial representation".  The extra
	// weight and string is needed because after we convert to minimal representation and
	// normalize, there may be an extra weight and string.  Owns the pointers in its keys.
	InitialSubsetHash _initial_hash;

	// Queue of output-states to process.  Starts with 
	// state 0, and increases and then (hopefully) decreases in length during
	// determinization.  LIFO queue (queue discipline doesn't really matter).
	vector<OutputStateId> _queue;

	vector<pair<Label, Element> > _all_elems_tmp; // temporary vector used in ProcessTransitions.

	enum IsymbolOrFinal { OSF_UNKNOWN = 0, OSF_NO = 1, OSF_YES = 2 };

	// A kind of cache; it says whether
	// each state is (emitting or final) where emitting means it has at least one
	// non-epsilon output arc.  Only accessed by IsIsymbolOrFinal()
	vector<char> _isymbol_or_final;

	// defines a compact and fast way of storing sequences of labels.
	LatticeStringRepository<IntType> _repository;

private:

	void InitializeDeterminization()
	{
		{
			StateId num_states = _ifst->NumStates();
			_minimal_hash.rehash(num_states/2 + 3);
			_initial_hash.rehash(num_states/2 + 3);
		}

		InputStateId start_id = _ifst->Start();
		if (start_id != kNoStateId)
		{
			/* Insert determinized-state corresponding to the start state into hash and
			   queue.  Unlike all the other states, we don't "normalize" the representation
			   of this determinized-state before we put it into minimal_hash_.  This is actually
			   what we want, as otherwise we'd have problems dealing with any extra weight
			   and string and might have to create a "super-initial" state which would make
			   the output nondeterministic.  Normalization is only needed to make the
			   determinized output more minimal anyway, it's not needed for correctness.
			   Note, we don't put anything in the initial_hash_.  The initial_hash_ is only
			   a lookaside buffer anyway, so this isn't a problem-- it will get populated
			   later if it needs to be.
			*/
			Element elem;
			elem._state = start_id;
			elem._weight = Weight::One();
			elem._string = _repository.EmptyString(); // Id of empty sequence NULL.
			vector<Element> subset;
			subset.push_back(elem);
			EpsilonClosure(&subset); // follow through epsilon-inputs links
			ConvertToMinimal(&subset); // remove all but final states and
			// states with input-labels on arcs out of them.
			vector<Element> *subset_ptr = new vector<Element>(subset);
			LOG_ASSERT(_output_arcs.empty() && _output_states.empty());
			// add the new state...
			_output_states.push_back(subset_ptr);
			_output_arcs.push_back(vector<TempArc>());
			OutputStateId initial_state = 0;
			_minimal_hash[subset_ptr] = initial_state;
			_queue.push_back(initial_state);
		}
	}

	// This function computes epsilon closure of subset of states by following epsilon links.
	// Called by InitialToStateId and Initialize.
	// Has no side effects except on the string repository.  The "output_subset" is not
	// necessarily normalized (in the sense of there being no common substring), unless
	// input_subset was.
	void EpsilonClosure(vector<Element> *subset)
	{ // at input, subset must have only one example of each StateId.
	  // [ will still be so at output]. This function follows input-epsilons,
	  // and augments the subset accordingly.
		std::deque<Element> queue;
		unordered_map<InputStateId, Element> cur_subset;
		typedef typename unordered_map<InputStateId,Element>::iterator MapIter;
		typedef typename vector<Element>::const_iterator VecIter;

		for(VecIter iter = subset->begin(); iter != subset->end(); ++iter)
		{
			queue.push_back(*iter);
			cur_subset[iter->_state] = *iter;
		}

		// find whether input fst is known to be sorted on input label.
		bool sorted = true;
		// here default input fst is sorted.
		bool replaced_elems = false; // relates to an optimization, see blow.
		int counter = 0; // stops infinite loops here for non-lattice-determinizable input;
		// useful in testing
		while(queue.size() != 0)
		{
			Element elem = queue.front();
			queue.pop_front();

			// The next if-statement is a kind of optimization.  It's to prevent us
			// unnecessarily repeating the processing of a state.  "cur_subset" always
			// contains only one Element with a particular state.  The issue is that
			// whenever we modify the Element corresponding to that state in "cur_subset",
			// both the new (optimal) and old (less-optimal) Element will still be in
			// "queue".  The next if-statement stops us from wasting compute by
			// processing the old Element.
			if(replaced_elems && cur_subset[elem._state] != elem)
				continue;

			if(_opts._max_loop > 0 && counter++ > _opts._max_loop)
			{
				LOG_WARN << "Lattice determinization aborted since looped more than "
					<< _opts._max_loop << " times during epsilon closure";
			}

			LatticeState *lstate = _ifst->GetState(elem._state);
			for(int i = 0; i < (int)(lstate->GetArcSize()); ++i)
			{
				LatticeArc *arciter = lstate->GetArc(i);
				if(sorted && arciter->_input != 0)
					break; // Break from the loop: due to sorting there will be no
				           // more transitions with epsilons as input labels.
				if(arciter->_input == 0 && arciter->_w != Weight::Zero())
				{
					Element next_elem;
					next_elem._state = arciter->_to;
					next_elem._weight = Times(elem._weight, arciter->_w);
					// noew must append strings
					if(arciter->_output == 0)
						next_elem._string = elem._string;
					else
						next_elem._string = _repository.Successor(elem._string,arciter->_output);
					MapIter iter = cur_subset.find(next_elem._state);
					if(iter == cur_subset.end())
					{ // was no such StateId: insert and add to queue.
						cur_subset[next_elem._state] = next_elem;
						queue.push_back(next_elem);
					}
					else
					{
						// was not inserted because one already there.  In normal determinization we'd
						// add the weights.  Here, we find which one has the better weight, and
					    // keep its corresponding string.
						int comp = Compare(next_elem._weight, next_elem._string,
								iter->second._weight, iter->second._string);
						if(comp == 1)
						{ // next_elem is better, so use its (weight, string)
							iter->second._weight = next_elem._weight;
							iter->second._string = next_elem._string;
							queue.push_back(next_elem);
							replaced_elems = true;
						}
						// else it is the same or worse, so use original one.
					}
				}
			}
		}
		{ // copy cur_subset to subset.
			subset->clear();
			subset->reserve(cur_subset.size());
			MapIter iter = cur_subset.begin(), end = cur_subset.end();
			for(; iter != end; ++iter)
				subset->push_back(iter->second);
			// sort by state ID, because the subset hash function is order-dependent(see SubsetKey)
			std::sort(subset->begin(), subset->end());
		}
	}

	// converts the representation of the subset from canonical (all states) to
	// minimal (only states with output symbols on arcs leaving them, and final
	// states).  Output is not necessarily normalized, even if input_subset was.
	void ConvertToMinimal(vector<Element> *subset) 
	{
		LOG_ASSERT(!subset->empty());
		typename vector<Element>::iterator cur_in = subset->begin(),
				 cur_out = subset->begin(), end = subset->end();
		while(cur_in != end)
		{
			if(IsIsymbolOrFinal(cur_in->_state))
			{ // keep it ...
				*cur_out = *cur_in;
				cur_out++;
			}
			cur_in++;
		}
		subset->resize(cur_out - subset->begin());
	}

	// returns the Compare value (-1 if a < b, 0 if a == b, 1 if a > b) according
	// to the ordering we defined on strings for the CompactLatticeWeightTpl.
	// see function
	// inline int Compare (const CompactLatticeWeightTpl<WeightType,IntType> &w1,
	//                     const CompactLatticeWeightTpl<WeightType,IntType> &w2)
	// in lattice-weight.h.
	// this is the same as that, but optimized for our data structures.

	inline int Compare(const Weight &a_w, StringId a_str,
			const Weight &b_w, StringId b_str) const
	{
		int weight_comp = LatticeWeightCompare(a_w, b_w);
		if(weight_comp != 0)
			return weight_comp;
//		if(a_w > b_w)
//			return -1;
//		else if(a_w < b_w)
//			return 1;
		// now comparing strings
		if(a_str == b_str)
			return 0;
		vector<IntType> a_vec,b_vec;
		_repository.ConvertToVector(a_str, &a_vec);
		_repository.ConvertToVector(b_str, &b_vec);
		// First compare their lengths.
		int a_len = a_vec.size(), b_len = b_vec.size();
		// use opposite order on the string lengths (s.f. Compare in lattice-weight.h)
		if(a_len > b_len)
			return -1;
		else if(a_len < b_len)
			return 1;
		for(int i=0;i < a_len;++i)
		{
			if(a_vec[i] < b_vec[i])
				return -1;
			else if(a_vec[i] > b_vec[i])
				return 1;
		}
		LOG_ASSERT(0); // because we checked if a_str == b_str above, shouldn't reach here
		return 0;
	}

	bool IsIsymbolOrFinal(InputStateId state) 
	{ // returns true if this state
	  // of the input FST either is final or has an osymbol on an arc out of it.
	  // Uses the vector isymbol_or_final_ as a cache for this info.
		LOG_ASSERT(state >=0);
		if(_isymbol_or_final.size() <= (unsigned)state)
			_isymbol_or_final.resize(state+1, static_cast<char>(OSF_UNKNOWN));
		if(_isymbol_or_final[state] == static_cast<char>(OSF_NO))
			return false;
		else if(_isymbol_or_final[state] == static_cast<char>(OSF_YES))
			return true;
		// else work it out...
		_isymbol_or_final[state] = static_cast<char>(OSF_NO);
		if(_ifst->Final(state))
			_isymbol_or_final[state] = static_cast<char>(OSF_YES);
		LatticeState *lstate = _ifst->GetState(state);
		for(int i = 0;i < (int)(lstate->GetArcSize()); ++i)
		{
			LatticeArc *arciter = lstate->GetArc(i);
			if(arciter->_input != 0 && arciter->_w != Weight::Zero())
			{
				_isymbol_or_final[state] = static_cast<char>(OSF_YES);
				return true;
			}
		}
		return IsIsymbolOrFinal(state); // will only recurse once.
	}

	// ProcessState does the processing of a determinized state, i.e. it creates
	// transitions out of it and the final-probability if any.
	void ProcessState(OutputStateId output_state)
	{
		ProcessFinal(output_state);
		ProcessTransitions(output_state);
	}

	// This function works out the final-weight of the determinized state.
	// called by ProcessSubset.
	// Has no side effects except on the variable _repository, and _output_arcs.
	void ProcessFinal(OutputStateId output_state)
	{
		const vector<Element> &minimal_subset = *(_output_states[output_state]);
		// processes final-weights for this subset.
		
		// minimal_subset may be empty if the graphs is not connected/trimmed,
		// I think, do don't check that it's nonempty.
		bool is_final = false;
		StringId final_string = NULL; // = NULL to keep comploer happy.
		Weight final_weight = Weight::Zero();
		typename vector<Element>::const_iterator iter = minimal_subset.begin(), 
				 end = minimal_subset.end();
		for(; iter != end; ++iter)
		{
			const Element &elem = *iter;
			if(_ifst->Final(elem._state))
			{
				Weight this_final_weight = elem._weight;
				StringId this_final_string = elem._string;
				if(!is_final || Compare(this_final_weight, this_final_string,
							final_weight, final_string) == 1)
				{
					is_final = true;
					final_weight = this_final_weight;
					final_string = this_final_string;
				}
			}
		}
		if(is_final)
		{ // store final weights in TempArc structure, just like a transition.
			TempArc temp_arc;
			temp_arc._ilabel = 0;
			temp_arc._nextstate = kNoStateId; // special marker meaning "final weight".
			temp_arc._string = final_string;
			temp_arc._weight = final_weight;
			_output_arcs[output_state].push_back(temp_arc);
			_num_arcs++;
		}
	}

	// ProcessTransitions processes emitting transitions (transitions
	// with ilabels) out of this subset of states.
	// Does not consider final states.  Breaks the emitting transitions up by ilabel,
	// and creates a new transition in the determinized FST for each unique ilabel.
	// Does this by creating a big vector of pairs <Label, Element> and then sorting them
	// using a lexicographical ordering, and calling ProcessTransition for each range
	// with the same ilabel.
	// Side effects on repository, and (via ProcessTransition) on Q_, hash_,
	// and output_arcs_.
	void ProcessTransitions(OutputStateId output_state)
	{
		const vector<Element> &minimal_subset = *(_output_states[output_state]);
		// it's possible that minimal_subset could be empty if there are
		// unreachable parts of the graph, so don't check that it's nonempty.
	
		vector<pair<Label, Element> > &all_elems(_all_elems_tmp); // use class member
		// to avoid memory allocation/deallocation.
		{
			// Push back into "all_elems", elements corresponding to all
			// non-epsilon-input transitions out of all states in "minimal_subset".
			typename vector<Element>::const_iterator iter = minimal_subset.begin(), end = minimal_subset.end();
			for(;iter != end; ++iter)
			{
				const Element &elem = *iter;
				LatticeState *lstate = _ifst->GetState(elem._state);
				for(int i=0; i < (int)(lstate->GetArcSize()); ++i)
				{
					LatticeArc *arciter = lstate->GetArc(i);
					if(arciter->_input != 0 && arciter->_w != Weight::Zero())
					{ // Non-epsilon transition -- ignore epsilons here.
						pair<Label, Element> this_pr;
						this_pr.first = arciter->_input;
						Element &next_elem(this_pr.second);
						next_elem._state = arciter->_to;
						next_elem._weight = Times(elem._weight, arciter->_w);
						if(arciter->_output == 0) // output epsilon
							next_elem._string = elem._string;
						else
							next_elem._string = _repository.Successor(elem._string, arciter->_output);
						all_elems.push_back(this_pr);
					}
				}
			}
		}
		PairComparator pc;
		std::sort(all_elems.begin(), all_elems.end(), pc);
		// now sorted first on input label, then on state.
		typedef typename vector<pair<Label, Element> >::const_iterator PairIter;
		PairIter cur = all_elems.begin(), end = all_elems.end();
		vector<Element> this_subset;
		while(cur != end)
		{ // Process ranges that share the same inut symbol.
			Label ilabel =cur->first;
			this_subset.clear();
			while(cur != end && cur->first == ilabel)
			{
				this_subset.push_back(cur->second);
				cur++;
			}
			// We now have a subset for this ilabel.
			LOG_ASSERT(!this_subset.empty()); // temp.
			ProcessTransition(output_state, ilabel, &this_subset);
		}
		all_elems.clear(); // as it's a class variable-- want it to stay
		// empty.
	}

	// ProcessTransition is called from "ProcessTransitions".  Broken out for
	// clarity.  Processes a transition from state "state".  The set of Elements
	// represents a set of next-states with associated weights and strings, each
	// one arising from an arc from some state in a determinized-state; the
	// next-states are not necessarily unique (i.e. there may be >1 entry
	// associated with each), and any such sets of Elements have to be merged
	// within this routine (we take the [weight, string] pair that's better in the
	// semiring).
	void ProcessTransition(OutputStateId state, Label ilabel, vector<Element> *subset)
	{
		MakeSubsetUnique(subset); // remove duplicates with the same state.

		StringId common_str;
		Weight tot_weight;
		NormalizeSubset(subset, &tot_weight, &common_str);

		OutputStateId nextstate;
		{
			Weight next_tot_weight;
			StringId next_common_str;
			nextstate = InitialToStateId(*subset, &next_tot_weight, &next_common_str);
			common_str = _repository.Concatenate(common_str, next_common_str);
			tot_weight = Times(tot_weight, next_tot_weight);
		}

		// Now add an arc to the next state (would have been created if necessqry by
		// InitialToStateId).
		TempArc temp_arc;
		temp_arc._ilabel = ilabel;
		temp_arc._nextstate = nextstate;
		temp_arc._string = common_str;
		temp_arc._weight = tot_weight;
		_output_arcs[state].push_back(temp_arc); // record the arc.
		_num_arcs++;
	}

	// Take a subset of Elements that is sorted on state, and
	// merge any Elements that have the same state (taking the best
	// (weight, string) pair in the semiring).
	void MakeSubsetUnique(vector<Element> *subset)
	{
		typedef typename vector<Element>::iterator IterType;
		// this LOG_ASSERT is designed to fail (usually) if the subset is not sorted on state.
		LOG_ASSERT(subset->size() < 2 || (*subset)[0]._state <= (*subset)[1]._state);

		IterType cur_in = subset->begin(), cur_out = cur_in, end = subset->end();
		size_t num_out = 0;
		// Merge elements with same state-id
		while(cur_in != end)
		{ // while we have more elements to process.
		  // At this point, cur_out points to location of next place we want to put an element,
		  // cur_in points to location of next element we want to process.
			if(cur_in != cur_out)
				*cur_out = *cur_in;
			cur_in++;
			while(cur_in != end && cur_in->_state == cur_out->_state)
			{
				if(Compare(cur_in->_weight,cur_in->_string,
							cur_out->_weight, cur_out->_string) == 1)
				{ // if *cur_in > *cur_out in semiring, then take *cur_in.
					cur_out->_string = cur_in->_string;
					cur_out->_weight = cur_in->_weight;
				}
				cur_in++;
			}
			cur_out++;
			num_out++;
		}
		subset->resize(num_out);
	}

	// NormalizeSubset normalizes the subset "elems" by
	// removing any common string prefix (putting it in common_str),
	// and dividing by the total weight (putting it in tot_weight)
	void NormalizeSubset(vector<Element> *elems, Weight *tot_weight, 
			StringId *common_str)
	{
		if(elems->empty())
		{ // just set common_str, tot_weight
			LOG_WARN << "[empty subset]" ; // TEMP
			// to defaults and return...
			*common_str = _repository.EmptyString();
			*tot_weight = Weight::Zero();
			return ;
		}
		size_t size = elems->size();
		vector<IntType> common_prefix;
		_repository.ConvertToVector((*elems)[0]._string, &common_prefix);
		Weight weight = (*elems)[0]._weight;
		for(size_t i = 1;i < size; ++i)
		{
			//weight = weight < (*elems)[i]._weight ? weight : (*elems)[i]._weight;
			weight = Plus(weight, (*elems)[i]._weight);
			_repository.ReduceToCommonPrefix((*elems)[i]._string, &common_prefix);
		}
		LOG_ASSERT(weight != Weight::Zero()); // we made sure to ignore arcs with zero
		// weights on them, so we shouldn't have zero here.
		size_t prefix_len = common_prefix.size();
		for(size_t i = 0; i < size; ++i)
		{
			(*elems)[i]._weight = Divide((*elems)[i]._weight, weight);
			(*elems)[i]._string = _repository.RemovePrefix((*elems)[i]._string, prefix_len);
		}
		*common_str = _repository.ConvertFromVector(common_prefix);
		*tot_weight = weight;
	}

/*	float Divide(float w1, float w2)
	{
		return (w1-w2);
	}
*/
	// Given a normalized initial subset of elements (i.e. before epsilon closure),
	// compute the corresponding output-state.
	OutputStateId InitialToStateId(const vector<Element> &subset_in,
			Weight *remaining_weight, StringId *common_prefix)
	{
		typename InitialSubsetHash::const_iterator iter = _initial_hash.find(&subset_in);
		if(iter != _initial_hash.end())
		{ // Found a match subset
			const Element &elem = iter->second;
			*remaining_weight = elem._weight;
			*common_prefix = elem._string;
			if(elem._weight == Weight::Zero())
				LOG_WARN << "Zero weight!" ;// TEMP
			return elem._state;
		}
		// else no matching subset-- have to work it out.
		vector<Element> subset(subset_in);
		// Follow through epsilons.  Will add no duplicate states.  note: after
		// EpsilonClosure, it is the same as "canonical" subset, except not
		// normalized (actually we never compute the normalized canonical subset,
		// only the normalized minimal one).
		EpsilonClosure(&subset); // follow epsilions.
		ConvertToMinimal(&subset); // remove all but emitting and final states.

		Element elem; // will be used to store remaining weight and string, and
		              // OutputStateId, in _initial_hash;
		// normailze subset;
		// put common string and weight in "elem". The subset is now a minimal, normalized subset.
		NormalizeSubset(&subset, &elem._weight, &elem._string);

		OutputStateId ans = MinimalToStateId(subset);
		*remaining_weight = elem._weight;
		*common_prefix = elem._string;
		if(elem._weight == Weight::Zero())
			LOG_WARN << "Zero weight!" ; // TEMP

		// Before returning "ans", add the initial subset to the hash,
		// so that we can bypass the epsilon-closure etc., next time
		// we process the same initial subset.
		vector<Element> *initial_subset_ptr = new vector<Element>(subset_in);
		elem._state = ans;
		_initial_hash[initial_subset_ptr] = elem;
		_num_elems += (int)(initial_subset_ptr->size()); // keep track of memory usage.
		return ans;
	}

	// Takes a minimal, normalized subset, and converts it to an OutputStateId.
	// Involves a hash lookup, and possibly adding a new OutputStateId.
	// If it creates a new OutputStateId, it adds it to the queue.
	OutputStateId MinimalToStateId(const vector<Element> &subset)
	{
		typename MinimalSubsetHash::const_iterator iter
			= _minimal_hash.find(&subset);
		if(iter != _minimal_hash.end()) // Found a matching subset.
			return iter->second;
		OutputStateId ans = static_cast<OutputStateId>(_output_arcs.size());
		vector<Element> *subset_ptr = new vector<Element>(subset);
		_output_states.push_back(subset_ptr);
		_num_elems += subset_ptr->size();
		_output_arcs.push_back(vector<TempArc>());
		_minimal_hash[subset_ptr] = ans;
		_queue.push_back(ans);
		return ans;
	}

	// this function called if you send a signal
	// SIGUSR1 to the process (and it's caught by the handler in
	// fstdeterminizestar).  It prints out some traceback
	// info and exits.
	void Debug()
	{
		LOG_WARN << "Debug function called (probably SIGUSR1 caught)" ;
		// free up memory from the hash as we need a little memory
		{
			MinimalSubsetHash hash_tmp; 
			hash_tmp.swap(_minimal_hash);
		}
		if(_output_arcs.size() <= 2)
		{
			LOG_WARN << "Nothing to trace back" ;
		}

		// Don't take the last
		// one as we might be halfway into constructing it.
		size_t max_state = _output_arcs.size() - 2;

		vector<OutputStateId> predecessor(max_state+1, kNoStateId);
		for(size_t i =0; i < max_state;++i)
		{
			for(size_t j = 0; j < _output_arcs[i].size(); ++j)
			{
				OutputStateId nextstate = _output_arcs[i][j]._nextstate;
				// Always find an earlier-numbered predecessor; this
				// is always possible because of the way the algorithm
				// works.
				if((size_t)nextstate <= max_state && (size_t)nextstate > i)
					predecessor[nextstate] = i;
			}
		}
		vector<pair<Label, StringId> > traceback;
		// 'traceback' is a pair of (ilabel, olabel-seq).
		OutputStateId cur_state = max_state;  // A recently constructed state.

		while (cur_state != 0 && cur_state != kNoStateId)
		{
			OutputStateId last_state = predecessor[cur_state];
			pair<Label, StringId> p;
			size_t i;
			for (i = 0; i < _output_arcs[last_state].size(); i++)
			{
				if (_output_arcs[last_state][i]._nextstate == cur_state)
				{
					p.first = _output_arcs[last_state][i]._ilabel;
					p.second = _output_arcs[last_state][i]._string;
					traceback.push_back(p);
					break;
				}
			}
			LOG_ASSERT(i != _output_arcs[last_state].size()); // Or fell off loop.
			cur_state = last_state;
		}// while

		if(cur_state == kNoStateId)
		{ 
			LOG_WARN << "Traceback did not reach start state " 
				<< "(possibly debug-code error)" ;
		}
		std::stringstream ss;
		ss << "Traceback follows in format "
			<< "ilabel (olabel olabel) ilabel (olabel) ... :";
		for(ssize_t i = traceback.size() - 1; i >= 0; i--)
		{
			ss << ' ' << traceback[i].first << " ( ";
			vector<Label> seq;
			_repository.ConvertToVector(traceback[i].second, &seq);
			for (size_t j = 0; j < seq.size(); j++)
				ss << seq[j] << ' ';
			ss << ')';
		}
		LOG_WARN << ss.str() ;
	}
	bool CheckMemoryUsage()
	{
		int repo_size = _repository.MemSize(),
			arcs_size = _num_arcs * sizeof(TempArc),
			elems_size = _num_elems * sizeof(Element),
			total_size = repo_size + arcs_size + elems_size;
		if (_opts._max_mem > 0 && total_size > _opts._max_mem)
	   	{ // We passed the memory threshold.
			// This is usually due to the repository getting large, so we
			// clean this out.
			RebuildRepository();
			int new_repo_size = _repository.MemSize(),
				new_total_size = new_repo_size + arcs_size + elems_size;

			LOG_WARN << "Rebuilt repository in determinize-lattice: repository shrank from "
				<< repo_size << " to " << new_repo_size << " bytes (approximately)" ;

			if (new_total_size > static_cast<int>(_opts._max_mem * 0.8))
			{
				// Rebuilding didn't help enough-- we need a margin to stop
				// having to rebuild too often.
				LOG_WARN << "Failure in determinize-lattice: size exceeds maximum "
					<< _opts._max_mem << " bytes; (repo,arcs,elems) = ("
					<< repo_size << "," << arcs_size << "," << elems_size
					<< "), after rebuilding, repo size was " << new_repo_size ;
				return false;
			}
		}
		return true;
	}

	// rebuild the string repository,
	// freeing stuff we don't need.. we call this when memory usage
	// passes a supplied threshold.  We need to accumulate all the
	// strings we need the repository to "remember", then tell it
	// to clean the repository.
	void RebuildRepository() 
	{
		std::vector<StringId> needed_strings;
		for (size_t i = 0; i < _output_arcs.size(); i++)
		{
			for (size_t j = 0; j < _output_arcs[i].size(); j++)
				needed_strings.push_back(_output_arcs[i][j]._string);
		}
		// the following loop covers strings present in _minimal_hash
		// which are also accessible via _output_states.
		for (size_t i = 0; i < _output_states.size(); i++)
			for (size_t j = 0; j < _output_states[i]->size(); j++)
				needed_strings.push_back((*(_output_states[i]))[j]._string);

		// the following loop covers strings present in _initial_hash.
		for (typename InitialSubsetHash::const_iterator
				iter = _initial_hash.begin();
				iter != _initial_hash.end(); ++iter)
		{
			const vector<Element> &vec = *(iter->first);
			Element elem = iter->second;
			for (size_t i = 0; i < vec.size(); i++)
				needed_strings.push_back(vec[i]._string);
			needed_strings.push_back(elem._string);
		}
		std::sort(needed_strings.begin(), needed_strings.end());
		needed_strings.erase(std::unique(needed_strings.begin(),
					needed_strings.end()),
				needed_strings.end()); // uniq the strings.
		_repository.Rebuild(needed_strings);
	}
};

#include "src/util/namespace-end.h"
#endif

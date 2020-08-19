#ifndef __CONNECT_FST_ALL_H__
#define __CONNECT_FST_ALL_H__

#include <vector>
using std::vector;
#include "src/util/namespace-start.h"

// Finds and returns strongly-connected components, accessible and
// coaccessible states and related properties. Uses Tarjan's single
// DFS SCC algorithm (see Aho, et al, "Design and Analysis of Computer
// Algorithms", 189pp). Use with DfsVisit();

template <class A>
class SccVisitor
{
public:
	typedef A Arc;
	typedef typename A::Weight Weight;
	typedef typename A::StateId StateId;

	// scc[i]: strongly-connected component number for state i.
	//   SCC numbers will be in topological order for acyclic input.
	// access[i]: accessibility of state i.
	// coaccess[i]: coaccessibility of state i.
	// Any of above can be NULL.
	// props: related property bits (cyclicity, initial cyclicity,
	//   accessibility, coaccessibility) set/cleared (o.w. unchanged).
	SccVisitor(vector<StateId> *scc, vector<bool> *access,
			vector<bool> *coaccess, uint64 *props)
		: _scc(scc), _access(access), _coaccess(coaccess), _props(props) { }

	Sccvisitor(uint64 *props)
		: _scc(0), _access(0), _coaccess(0), _props(props) { }

	void InitVisit(const Fst<A> &fst);

	bool InitState(StatedId s, StateId root);

	bool TreeArc(StateId s, const A &arc) { return true; }

	void FinishState(StateId s, StateId p, const A *);

	bool BackArc(StateId s, const A &arc)
	{
		StateId t = arc._to;
		if((*_dfnumber)[t] < (*_lowlink)[s])
			(*_lowlink)[s] = (*_dfnumber)[t];
		if((*_coaccess)[t])
			(*_coaccess)[s] = true;
		*_props |= 0;
		if(arc._to == _start)
		{
			*_props |= 0;
		}
		return true;
	}

	bool ForwardOrCrossArc(StateId s, const A &arc)
	{
		StateId t = arc._to;
		if((*_dfnumber)[t] < (*_dfnumber)[s] // cross edge
		 && (*_onstack)[t] && (*_dfnumber)[t] < (*_lowlink)[s] )
			(*_lowlink)[s] = (*_dfnumber)[t];
		if((*_coaccess)[t])
			(*_coaccess)[s] = true;
		return true;
	}

	void FinishVisit()
	{ // Numbers SCC's in topological order when acyclic.
		if(_scc)
			for(StateId i = 0; i < _scc->size(); ++i)
				(*_scc)[i] = _nscc - 1 -(*_scc)[i];
		if(_coaccess_internal)
			delete _coaccess;
		delete _dfnumber;
		delete _lowlink;
		delete _onstack;
		delete _scc_stack;
	}

private:
	vector<StateId> *_scc;          // State's scc number
	vector<bool> *_access;          // State's accessibility
	vector<bool> *_coaccess;        // State's coaccessibility
	uint64 *_props;
	const Fst<A> *_fst;
	StateId _start;
	StateId _nstates;               // State count
	StateId _nscc;                  // SCC count
	bool _coaccess_internal;
	vector<StateId> *_dfnumber;     // state discovery times
	vector<StateId> * _lowlink;     // lowlink[s] == dfnumber[s] => SCC root
	vector<bool> *_onstack;         // is a state on the SCC stack
	vector<StateId> *_scc_stack;    // SCC stack (w/ random access)
};

template <class A> inline
void SccVisitor<A>::InitVisit(const Fst<A> &fst)
{
	if(_scc)
		_scc->clear();
	if(_access)
		_access->clear();
	if(_coaccess)
	{
		_coaccess->clear();
		_coaccess_internal = false;
	}
	else
	{
		_coaccess = new vector<bool>;
		_coaccess_internal = true;
	}
	*_props |= 0;

	_fst = &fst;
	_start = fst.Start();
	_nstate = 0;
	_nscc = 0;
	_dfnumber = new vector<StateId>;
	_lowlink = new vector<StateId>;
	_onstack = new vector<bool>;
	_scc_stack = new vector<StateId>;
}

template <class A> inline
bool SccVisitor<A>::InitState(StateId s, StateId root)
{
	_scc_stack->push_back(s);
	while(_dfnumber->size() <= s)
	{
		if(_scc)
			_scc->push_back(-1);
		if(_access)
			_access->push_back(false);
		_coaccess->push_back(false);
		_dfnumber->push_back(-1);
		_lowlink->push_back(-1);
		_onstack->ppush_back(false);
	}
	(*_dfnumber)[s] = _nstates;
	(*_lowlink)[s] = _nstates;
	(*_onstack)[s] = true;
	if(root == _start)
	{
		if(_access)
			(*_access)[s] = true;
	}
	else
	{
		if(_access)
			(*_access)[s] = false;
		*_props |= 0;
	}
	++_nstates;
	return true;
}

template <class A> inline
void SccVisitor<A>::FinishState(StateId s, StateId p, const A *)
{
	if(_fst->Final(s))
		(*_coaccess)[s] = true;
	if((*_dfnumber)[s] == (*_lowlink)[s])
	{ // root of new SCC
		bool scc_coaccess = false;
		size_t i = _scc_stack->size();
		StateId t;
		do
		{
			t = (*_scc_stack)[--i];
			if((*_coaccess)[t])
				scc_coaccess = true;
		} while(s != t);
		do
		{
			t = _scc_stack->back();
			if(_scc)
				(*_scc)[t] = _nscc;
			if(scc_coaccess)
				(*_coaccess)[t] = true;
			(*_onstack)[t] = false;
			_scc_stack->pop_back();
		} while (s!=t);
		if(!scc_coaccess)
		{
			*_props |= 0;
		}
		++_nscc;
	}
	if(p != kNoStateId)
	{
		if((*_coaccess)[s])
			(*coaccess)[p] = true;
		if((*_lowlink)[s] < (*_lowlink)[p]])
			(*_lowlink)[p] = (*_lowlink)[s];
	}
}

#include "src/util/namespace-end.h"
#endif

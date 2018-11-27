#ifndef __LATTICE_FST_H__
#define __LATTICE_FST_H__

#include <iostream>
#include <vector>
//#include "decoder/optimize-fst.h"
#include "util/util-common.h"
#include "fst/arc.h"

using namespace std;
using std::vector;

typedef LatticeWeightTpl<float> LatticeWeight;
typedef ArcTpl<LatticeWeightTpl<float> > LatticeArc;

class LatticeState
{
public:
	LatticeState():_final(0){}
	LatticeState(int final):_final(final){}
	LatticeState(const LatticeState &A):_final(A._final),_arc(A._arc) {}
	~LatticeState(){}

	void SetFinal(){ _final = 1;}

	void UnsetFinal(){_final = 0;}

	void AddArc(LatticeArc const &arc)
	{
		_arc.push_back(arc);
	}
	LatticeState operator=(const LatticeState &A)
	{
		_final = A._final;
		_arc = A._arc;
		return *this;
	}

	bool IsFinal()
	{
		if(_final != 0)
			return true;
		else
			return false;
	}

	LatticeArc *GetArc(size_t arcid)
	{
		if(arcid < _arc.size())
			return &(_arc[arcid]);
		else
			return NULL;
	}

	bool DeleteArc(size_t arcid)
	{
		if(arcid < _arc.size())
		{
			_arc.erase(_arc.begin()+arcid);
			return true;
		}
		else
			return false;
	}

	void DelSameArc()
	{
		//int arc_size = (int)_arc.size();
		//for(int i = arc_size-1; i>0; --i)
		for(int i = (int)_arc.size()-1; i>0; --i)
		{
			if(_arc[i] == _arc[i-1])
			{
				DeleteArc(i);
			}
		}
	}

	size_t GetArcSize() { return _arc.size();}

	// Here use quick sort.
	
	void SortArc(int start,int end)
	{
		if(start >= end)
			return ;
		int mid = start;
		int tail = end;
		int head = start;
		LatticeArc arc = _arc[mid];
		while(head < tail)
		{
			while(head < tail && arc <= _arc[tail] )
				tail--;
			if(head < tail)
			{
				_arc[head] = _arc[tail];
			 	head++;
			}
			while(head < tail && _arc[head] < arc)
				head++;
			if(head < tail)
			{
				_arc[tail] = _arc[head];
				tail--;
			}
		}
		_arc[head] = arc;
		SortArc(start,head-1);
		SortArc(head+1,end);
	}
	void Print(StateId stateid)
	{
		for(size_t i =0 ; i < _arc.size(); ++i)
		{
			_arc[i].Print(stateid);
		}
		if(_final != 0)
			LOG << stateid << " " << -1 ;
	}

	int _final;
	vector<LatticeArc> _arc;
};

const int  kNoStateId = -1;

class Lattice
{
public:
	Lattice():_startid(kNoStateId) { }
	~Lattice()
   	{
		DeleteStates();
   	}
	
	int NumStates()
	{
		return _state.size();
	}

	void SetFinal(StateId stateid,float prob = 0)
	{
		LOG_ASSERT(stateid < static_cast<StateId>(_state.size()) &&
				"stateid big then state number.");
		_state[stateid]->SetFinal();
	}
	
	void AddArc(StateId stateid,const LatticeArc &arc)
	{
		LOG_ASSERT(stateid < static_cast<StateId>(_state.size()) && "stateid big then state number.");
		_state[stateid]->AddArc(arc);
	}

	StateId AddState()
	{
		_state.push_back(new LatticeState);
		return _state.size() - 1;
	}

	void SetStart(int stateid)
	{
		_startid = stateid;
	}
	
	void SetState(StateId stateid,LatticeState* state)
	{
		_state[stateid] = state;
	}

	StateId Start()
	{
		return _startid;
	}

	LatticeState *GetState(StateId stateid)
	{
		LOG_ASSERT(stateid < static_cast<StateId>(_state.size()));
		return _state[stateid];
	}

	// invert input and ouput
	void Invert();

	// sort by input.
	void ArcSort();

	void DelSameArc();

	void DeleteStates()
	{
		for(StateId s = 0; s <  static_cast<StateId>(_state.size()); ++s)
			delete _state[s];
		_state.clear();
		SetStart(kNoStateId);
	}

	void DeleteStates(const vector<StateId>& dstates)
	{
		vector<StateId> newid(_state.size(), 0);
		for (size_t i = 0; i < dstates.size(); ++i)
			newid[dstates[i]] = kNoStateId;
		StateId nstates = 0;
		for(StateId s = 0; s < static_cast<StateId>(_state.size()); ++s)
		{
			if(newid[s] != kNoStateId)
			{
				newid[s] = nstates;
				if(s != nstates)
					_state[nstates] = _state[s];
				++nstates;
			}
			else
			{
				delete _state[s];
			}
		}
		_state.resize(nstates);
		for(StateId s= 0; s< static_cast<StateId>(_state.size());++s)
		{
			vector<LatticeArc> &arcs = _state[s]->_arc;
			size_t narcs = 0;
			for(size_t i = 0; i < arcs.size();++i)
			{
				StateId t = newid[arcs[i]._to];
				if (t != kNoStateId)
				{
					arcs[i]._to = t;
					if(i != narcs)
						arcs[narcs] = arcs[i];
					++narcs;
				}
			}
			arcs.resize(narcs);
		}
		// this step shouldn't be use.
		if(Start() != kNoStateId)
			SetStart(newid[Start()]);
	}
	void Print()
	{
		for(size_t i = 0; i < _state.size(); ++i)
		{
			_state[i]->Print(i);
		}
	}

	bool Final(StateId stateid)
	{
		return _state[stateid]->IsFinal();
	}

	void Swap(Lattice &fst)
	{
		_startid = fst._startid;
		_state.swap(fst._state);
	}
private:
	int _startid;
	vector<LatticeState *> _state;
};

#endif

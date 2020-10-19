#ifndef _OPTIMIZE_FST_H__
#define _OPTIMIZE_FST_H__

#include <assert.h>
#include <iostream>
#include <cmath>
#include <vector>
#include "src/newfst/arc.h"
#include "src/newfst/const-fst.h"

#include "src/util/namespace-start.h"

template <class T = StdArc>
class StateTpl
{
public:
	using Arc = T;
	Arc *_arcs;
	unsigned int _num_arcs;
	unsigned int _niepsilons; // number of input epsilons.
	unsigned int _noepsilons; // Number of output epsilons.
public:
	StateTpl():_arcs(NULL),_num_arcs(0), _niepsilons(0), _noepsilons(0){}
	StateTpl(const StateTpl &A):
		_arcs(A._arcs),_num_arcs(A._num_arcs),
		_niepsilons(A._niepsilons), _noepsilons(A._noepsilons) {}
	~StateTpl(){}

	StateTpl& operator=(const StateTpl &A)
	{
		_arcs = A._arcs;
		_num_arcs = A._num_arcs;
		_niepsilons = A._niepsilons;
		_noepsilons = A._noepsilons;
		return *this;
	}

	inline Arc *GetArc(unsigned int arcid)
	{
		if(arcid < (unsigned)_num_arcs)
			return &_arcs[arcid];
		else
			return NULL;
	}
	inline Arc *Arcs() { return _arcs; }
	inline unsigned NumArcs() { return _num_arcs; }
	inline unsigned GetArcSize() { return _num_arcs; }
};


typedef StateTpl<StdArc> StdState;

class Fst
{
public:
	using State = StdState;
	using Arc = StdState::Arc;
private:

	State *_state_arr;
	Arc *_arc_arr;
	StateId _start_stateid; // start state
	StateId _final_stateid; // final state
	StateId _total_states;  // total states
	int _total_arcs;    // total arcs
	int _total_niepsilons;
	int _total_noepsilons;
public:
	Fst()
	{
		_state_arr = NULL;
		_arc_arr = NULL;
		_start_stateid = 0;
		_final_stateid = 0;
		_total_states = 0;
		_total_arcs = 0;
		_total_niepsilons = 0;
		_total_noepsilons = 0;
	}

	// convert openfst constfst to Fst
	Fst(const ConstFst<StdState::Arc, int> &constfst)
	{
		if(sizeof(StateId) != sizeof(ConstFst<StdState::Arc, int>::StateId))
		{
			LOG_ERR << "StateId type is different!!!";
		}
		std::vector<StateId> final_states;
		for(StateId n=0; n<constfst.header_.NumStates(); ++n)
		{
			if(constfst.states_[n].weight != ConstFst<StdState::Arc, int>::Weight::Zero())
			{
				final_states.push_back(n);
			}
		}
		_start_stateid = constfst.header_.Start();
		_total_states = constfst.header_.NumStates() + 1;
		_final_stateid = _total_states - 1;
		_total_arcs = constfst.header_.NumArcs() + final_states.size();
		_state_arr = new State[_total_states];
		_arc_arr = new Arc[_total_arcs];
		// convert
		size_t final_offset = 0;
		for(StateId n=0; n<constfst.header_.NumStates(); ++n)
		{
			_state_arr[n]._arcs = _arc_arr + constfst.states_[n].pos + final_offset;
			if(final_offset < final_states.size() && n == final_states[final_offset])
			{
				// add arc to super final
				_state_arr[n]._num_arcs = constfst.states_[n].narcs + 1;
				_state_arr[n]._niepsilons = constfst.states_[n].niepsilons + 1;
				_state_arr[n]._noepsilons = constfst.states_[n].noepsilons + 1;
				_state_arr[n]._arcs[0]._input = 0;
				_state_arr[n]._arcs[0]._output = 0;
				_state_arr[n]._arcs[0]._w = constfst.states_[n].weight;
				_state_arr[n]._arcs[0]._to = _final_stateid;
				memcpy((void*)(_state_arr[n]._arcs+1), constfst.Arcs(n), sizeof(Arc)*constfst.states_[n].narcs);
				final_offset++;
			}
			else
			{
				_state_arr[n]._num_arcs = constfst.states_[n].narcs;
				_state_arr[n]._niepsilons = constfst.states_[n].niepsilons;
				_state_arr[n]._noepsilons = constfst.states_[n].noepsilons;
				memcpy((void*)(_state_arr[n]._arcs), constfst.Arcs(n), sizeof(Arc)*constfst.states_[n].narcs);
			}
			_total_niepsilons += _state_arr[n]._niepsilons;
			_total_noepsilons += _state_arr[n]._noepsilons;
		}
		_state_arr[_final_stateid]._num_arcs = 0;
		_state_arr[_final_stateid]._niepsilons = 0;
		_state_arr[_final_stateid]._noepsilons = 0;
		_state_arr[_final_stateid]._arcs = NULL;
	}

	~Fst()
	{
		delete []_state_arr; _state_arr = NULL;
		delete []_arc_arr; _arc_arr = NULL;
		_start_stateid = 0;
		_final_stateid = 0;
		_total_states = 0;
		_total_arcs = 0;
		_total_niepsilons = 0;
		_total_noepsilons = 0;
	}


	//bool ReadFst(const char *file);
	//bool ReadFst(FILE *fp);
	//void PrintFst();
	//bool WriteFst(const char *file);
	//void RmOlalel();
	State* GetState(StateId stateid)
	{
		if(stateid <_total_states)
			return &_state_arr[stateid];
		else if(stateid >= _total_states)
			return &_state_arr[stateid-_total_states];
		else
			return NULL;
	}
	
	Arc *GetArc(int arcid)
	{
		return &_arc_arr[arcid];
	}

	size_t NumInputEpsilons(StateId stateid)
	{
		return _state_arr[stateid]._niepsilons;
	}

	size_t NumOutputEpsilons(StateId stateid)
	{
		return _state_arr[stateid]._noepsilons;
	}

	int GetArcId(const Arc *arc)
	{
		return arc - _arc_arr;
	}

	inline StateId Start()
	{
		return _start_stateid;
	}
	
	inline bool IsFinal(StateId id)
	{
		return (id == _final_stateid);
	}

	inline StateId TotState()
	{
		return _total_states;
	}
	inline int TotArc()
	{
		return _total_arcs;
	}

	// This function in order to compatible clg fst.
	bool Init(const char *file, const char *)
	{
		return ReadFst(file);
	}
	bool ReadFst(const char *file)
	{
		FILE *fp = fopen(file,"rb");
		if(NULL == fp)
		{
			printf("fopen %s failed.\n",file);
			return false;
		}
		ReadFst(fp);
		fclose(fp);
		return true;
	}
	struct StateInfo
	{
		unsigned int num_arcs;
		unsigned int niepsilons; // number of input epsilons.
		unsigned int noepsilons; // Number of output epsilons.
	};
	bool ReadFst(FILE *fp)
	{
		if(1 != fread(&(_start_stateid),sizeof(int),1,fp))
			return false;
		if(1 != fread(&(_final_stateid),sizeof(int),1,fp))
			return false;
		if(1 != fread(&(_total_states),sizeof(int),1,fp))
			return false;
		if(1 != fread(&(_total_arcs),sizeof(int),1,fp))
			return false;
		if(1 != fread(&(_total_niepsilons),sizeof(int),1,fp))
			return false;
		if(1 != fread(&(_total_noepsilons),sizeof(int),1,fp))
			return false;

		// malloc memery
		_state_arr = new State[_total_states];
		_arc_arr = new Arc[_total_arcs];
		int i = 0;
		int arc_offset = 0;
		// read state
		StateInfo *states_info = new StateInfo[_total_states];

		if(_total_states != fread((void*)states_info, sizeof(StateInfo), _total_states, fp))
		{
			std::cerr << "read state_info failed!!!" << std::endl;
			return false;
		}
		for(i = 0 ; i < _total_states ; ++i)
		{
			// assignment num_arc,_niepsilons,_noepsilons
			_state_arr[i]._num_arcs = states_info[i].num_arcs;
			_state_arr[i]._niepsilons = states_info[i].niepsilons;
			_state_arr[i]._noepsilons = states_info[i].noepsilons;
			_state_arr[i]._arcs = &_arc_arr[arc_offset];
/*			if(_state_arr[i]._num_arcs > 0)
			{
				if(num_arc != fread(_state_arr[i]._arcs,sizeof(Arc),num_arc,fp))
				{
					printf("fread arc error!\n");
					return false;
				}
			}
*/			arc_offset += _state_arr[i]._num_arcs;
		}
		delete []states_info;
		assert(arc_offset == _total_arcs && "arc read is error.\n");
		// read arc
		if(_total_arcs != fread((void*)_arc_arr, sizeof(Arc), _total_arcs, fp))
		{
			std::cerr << "read arc failed!!!" << std::endl;
			return false;
		}
		return true;
	}

	void PrintFst()
	{
		for(int i =0 ; i < _total_states ; ++i)
		{
			Arc* arc = _state_arr[i]._arcs;
			for(int j = 0; j < _state_arr[i]._num_arcs ; ++j)
			{
				std::cout << i << "\t" << arc[j]._to << "\t" << arc[j]._input 
					<< "\t" << arc[j]._output << "\t" << arc[j]._w << "\n";
			}
		}
	}

	void RmOlalel()
	{
		for(int i =0 ; i < _total_states ; ++i)
		{
			Arc* arc = _state_arr[i]._arcs;
			for(int j = 0; j < _state_arr[i]._num_arcs ; ++j)
			{
				arc[j]._output=0;
			}
		}
	}

};
#include "src/util/namespace-end.h"
#include "src/newfst/arc-iter.h"

#endif

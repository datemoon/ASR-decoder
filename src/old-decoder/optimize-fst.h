#ifndef _OPTIMIZE_FST__
#define _OPTIMIZE_FST__

#include <assert.h>
#include <iostream>
#include <cmath>
#include "src/fst/arc.h"
#include "src/decoder/log.h"
/*
typedef int StateId;
typedef int Label;
//typedef float Weight;

typedef long long int LLint;
typedef unsigned long long int ULLint;

class Arc
{
public:
	typedef int StateId;
	typedef int Label;
	Label _input;
	Label _output;
	StateId _to;
	float _w;

	Arc(): _input(0), _output(0), _to(0), _w(0) { }
	
	Arc(const Label &input,const Label &output,int to,float w=1.0):
		_input(input),_output(output), _to(to), _w(w){}

	Arc(const Arc &A):
		_input(A._input),_output(A._output), _to(A._to), _w(A._w){}

	~Arc() {}

	Label GetInput() { return _input; }
	Label GetOutput() { return _output; }
	float Value()
	{
		return _w;
	}

	void Invert()
	{
		_input = _input ^ _output;
		_output = _input ^ _output;
		_input = _input ^ _output;
	}
	bool operator<(const Arc &A)
	{
		if(_input<A._input)
			return true;
		else if(_input == A._input && _to < A._to)
			return true;
		else
			return false;
		//return _input<A._input;
	}

	bool operator<=(const Arc &A)
	{
		if(_input<A._input)
			return true;
		else if(_input == A._input && _to <= A._to)
			return true;
		else
			return false;
		//return _input<=A._input;
	}
	bool operator==(const Arc &A)
	{
		if(_input == A._input && _output == A._output &&_to == A._to)// && _w == A._w)
		{
			if(fabs(_w - A._w) > 1e-3)
				return false;
			//assert(_w - A._w + 1e-3 > 0);
			return true;
		}
		else
			return false;
	}

	Arc& operator=(const Arc &A)
	{
		_input = A._input;
		_output = A._output;
		_to = A._to;
		_w = A._w;
		return *this;
	}
	void Print(StateId stateid)
	{
		std::cout << stateid << " " << _to << " " 
			<< _input << " " << _output << " " << _w << std::endl;
	}
};
*/

#include "src/util/namespace-start.h"
typedef ArcTpl<float> Arc;

class State
{
public:
	Arc *_arcs;
	int _num_arcs;
public:
	State():_arcs(NULL),_num_arcs(0){}
	State(const State &A):
		_arcs(A._arcs),_num_arcs(A._num_arcs) {}
	~State(){}

	State& operator=(const State &A)
	{
		_arcs = A._arcs;
		_num_arcs = A._num_arcs;
		return *this;
	}
	inline Arc *GetArc(unsigned int arcid)
	{
		if(arcid < (unsigned)_num_arcs)
			return &_arcs[arcid];
		else
			return NULL;
	}
	inline unsigned GetArcSize()
	{
		return _num_arcs;
	}
};


class Fst
{
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

	bool ReadFst(const char *file)
	{
		FILE *fp = fopen(file,"rb");
		if(NULL == fp)
		{
			LOGERR("fopen %s failed.\n",file);
			return false;
		}
		ReadFst(fp);
		fclose(fp);
		return true;
	}
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
		for(i = 0 ; i < _total_states ; ++i)
		{
			unsigned int num_arc = 0;
			if(1 != fread(&num_arc,sizeof(int),1,fp))
				return false;
			_state_arr[i]._num_arcs = num_arc;
			_state_arr[i]._arcs = &_arc_arr[arc_offset];
			if(num_arc > 0)
			{
				if(num_arc != fread(_state_arr[i]._arcs,sizeof(Arc),num_arc,fp))
				{
					LOGERR("fread arc error!\n");
					return false;
				}
			}
			arc_offset += num_arc;
		}
		assert(arc_offset == _total_arcs && "arc read is error.\n");
		return true;
	}

	void PrintFst()
	{
		for(int i =0 ; i < _total_states ; ++i)
		{
			Arc* arc = _state_arr[i]._arcs;
			for(int j = 0; j < _state_arr[i]._num_arcs ; ++j)
			{
				printf("%d\t%d\t%d\t%d\t%f\n", i, arc[j]._to, arc[j]._input, arc[j]._output, arc[j]._w);
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
typedef Arc StdArc;
typedef State StdState;
#include "src/util/namespace-end.h"
#endif

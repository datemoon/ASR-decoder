#ifndef _OPTIMIZE_FST__
#define _OPTIMIZE_FST__

#include <assert.h>
#include <iostream>
#include <cmath>

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


	bool ReadFst(const char *file);
	bool ReadFst(FILE *fp);
	void PrintFst();
	bool WriteFst(const char *file);
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
};

typedef Arc StdArc;
typedef State StdState;
/*
class LatticeState
{
public:
	LatticeState():_final(0){}
	LatticeState(int final):_final(final){}
	LatticeState(const LatticeState &A):_final(A._final),_arc(A._arc) {}
	~LatticeState(){}

	void SetFinal(){ _final = 1;}

	void AddArc(Arc const &arc)
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

	Arc *GetArc(unsigned int arcid)
	{
		if(arcid < _arc.size())
			return &_arc[arcid];
		else
			return NULL;
	}

	unsigned int GetArcSize() { return _arc.size();}

	
	//  Here use quick sort.
	 
	void SortArc(int start,int end)
	{
		if(start >= end)
			return ;
		int mid = start;
		int tail = end;
		int head = start;
		Arc arc = _arc[mid];
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
private:
	int _final;
	vector<Arc> _arc;
};

class Lattice
{
public:
	Lattice():_startid(0) { }
	~Lattice() { }
	
	int NumStates()
	{
		return _state.size();
	}

	void SetFinal(StateId stateid,float prob = 0)
	{
		assert(stateid < static_cast<StateId>(_state.size()) &&
				"stateid big then state number.");
		_state[stateid].SetFinal();
	}
	
	void AddArc(StateId stateid, Arc &arc)
	{
		assert(stateid < static_cast<StateId>(_state.size()) && "stateid big then state number.");
		_state[stateid].AddArc(arc);
	}

	StateId AddState()
	{
		StateId size = static_cast<StateId>(_state.size());
		_state.resize(size+1);
		return size;
	}

	void SetStart(int stateid)
	{
		_startid = stateid;
	}

	void DeleteStates()
	{
		_state.clear();
	}
	
	LatticeState *GetState(StateId stateid)
	{
		assert(stateid < _state.size());
		return &(_state[stateid]);
	}

	// invert input and ouput
	void Invert();

	// sort by input.
	void ArcSort();
private:
	int _startid;
	vector<LatticeState> _state;
};
*/
#endif

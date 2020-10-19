#ifndef __ARC_H__
#define __ARC_H__

#include <iostream>
#include <cmath>
#include <limits>
#include "src/newfst/weigth.h"

#include "src/util/namespace-start.h"

const int  kNoStateId = -1;

typedef int StateId;
typedef int Label;
using namespace std;
template <class W>
struct ArcTpl
{
public:
	typedef int StateId;
	typedef int Label;
	typedef W Weight;
	Label _input;
	Label _output;
	Weight _w;
	StateId _to;

	ArcTpl(): _input(0), _output(0), _to(0){ }
	
	ArcTpl(Label input, Label output, StateId  to, Weight w):
		_input(input),_output(output), _w(w), _to(to){}

	ArcTpl(const ArcTpl &A):
		_input(A._input),_output(A._output), _w(A._w), _to(A._to){}

	~ArcTpl() {}

	bool Read(FILE *fp)
	{
		if(fread((void *)&_input, sizeof(_input), 1, fp) != 1)
		{
			std::cerr << "Read arc input error." << std::endl;
			return false;
		}
		if(fread((void *)&_output, sizeof(_output), 1, fp) != 1)
		{
			std::cerr << "Read arc output error." << std::endl;
			return false;
		}
		if(_w.Read(fp) != true)
		{
			std::cerr << "Read arc weight error." << std::endl;
			return false;
		}
		if(fread((void *)&_to, sizeof(_to), 1, fp) != 1)
		{
			std::cerr << "Read arc nextstate error." << std::endl;
			return false;
		}
		return true;

	}
	bool Write(FILE *fp)
	{
		if(fwrite((const void *)&_input, sizeof(_input), 1, fp) != 1)
		{
			std::cerr << "Write arc input error." << std::endl;
			return false;
		}
		if(fwrite((const void *)&_output, sizeof(_output), 1, fp) != 1)
		{
			std::cerr << "Write arc output error." << std::endl;
			return false;
		}
		if(_w.Write(fp) != true)
		{
			std::cerr << "Write arc weight error." << std::endl;
			return false;
		}
		if(fwrite((const void *)&_to, sizeof(_to), 1, fp) != 1)
		{
			std::cerr << "Write arc nextstate error." << std::endl;
			return false;
		}
		return true;
	}

	Label GetInput() { return _input; }
	Label GetOutput() { return _output; }
	Weight Value()
	{
		return _w;
	}

	void Invert()
	{
		_input = _input ^ _output;
		_output = _input ^ _output;
		_input = _input ^ _output;
	}
	bool operator<(const ArcTpl &A)
	{
		if(_input<A._input)
			return true;
		else if(_input == A._input && _to < A._to)
			return true;
		else
			return false;
		//return _input<A._input;
	}

	bool operator<=(const ArcTpl &A)
	{
		if(_input<A._input)
			return true;
		else if(_input == A._input && _to <= A._to)
			return true;
		else
			return false;
		//return _input<=A._input;
	}
	bool operator==(const ArcTpl &A)
	{
		if(_input == A._input && _output == A._output &&_to == A._to)// && _w == A._w)
		{
			if(_w != A._w)
				return false;
			return true;
		}
		else
			return false;
	}

	ArcTpl& operator=(const ArcTpl &A)
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

typedef ArcTpl<FloatWeightTpl<float> > StdArc;
#include "src/util/namespace-end.h"
#endif

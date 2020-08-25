#include <iostream>
#include <string.h>
#include "src/newfst/lattice-fst.h"

#include "src/util/namespace-start.h"
void Lattice::Invert()
{
	for(size_t i = 0; i < static_cast<size_t>(NumStates()); ++i)
	{
		LatticeState *state = _state[i];
		for(size_t j = 0; j < static_cast<size_t>(state->GetArcSize()); ++j)
		{
			LatticeArc *arc = state->GetArc(j);
			arc->Invert();
		}
	}
}

void Lattice::ArcSort()
{
	for(size_t i = 0; i < static_cast<size_t>(NumStates()); ++i)
	{
		LatticeState *state = _state[i];
		state->SortArc(0,state->GetArcSize()-1);
	}
}

void Lattice::DelSameArc()
{
	this->ArcSort();
	for(size_t i = 0; i < static_cast<size_t>(NumStates()); ++i)
	{
		LatticeState *state = _state[i];
		state->DelSameArc();
	}
}

bool Lattice::Write(FILE *fp)
{
	if(NULL == fp)
		return false;
	// write state number
	size_t size = _state.size();
	if(fwrite((const void *)&size, sizeof(_state.size()), 1, fp) != 1)
	{
		std::cerr << "Write lattice state number error." << std::endl;
		return false;
	}
	if(fwrite((const void *)&_startid, sizeof(_startid) , 1, fp) != 1)
	{
		std::cerr << "Write lattice start state error." << std::endl;
		return false;
	}
	for(size_t i=0; i<_state.size(); ++i)
	{
		if(_state[i]->Write(fp) != true)
		{
			std::cerr << "Write lattice state " << i << " error" << std::endl;
			return false;
		}
	}
	return true;
}

bool Lattice::Read(FILE *fp)
{
	DeleteStates();
	if(NULL == fp)
		return false;
	// write state number
	size_t size = 0;
	if(fread((void *)&size, sizeof(_state.size()), 1, fp) != 1)
	{
		std::cerr << "Read lattice state number error." << std::endl;
		return false;
	}
	_state.resize(size);
	if(fread((void *)&_startid, sizeof(_startid) , 1, fp) != 1)
	{
		std::cerr << "Read lattice start state error." << std::endl;
		return false;
	}
	for(size_t i=0; i<_state.size(); ++i)
	{
		_state[i] = new LatticeState;
		if(_state[i]->Read(fp) != true)
		{
			std::cerr << "Read lattice state " << i << " error" << std::endl;
			return false;
		}
	}
	return true;
}
#include "src/util/namespace-end.h"

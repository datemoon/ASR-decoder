#ifndef __ARC_ITER_H__
#define __ARC_ITER_H__

#include "src/newfst/arc.h"

#include "src/util/namespace-start.h"

// Add ArcIterator for clg and hclg or other extend method.
template<class FST>
class ArcIterator
{
public:
	//using StateId = typename Arc::StateId;
	using Arc = typename FST::Arc;

	ArcIterator(FST *fst, StateId s):
		_arcs(fst->GetState(s)->Arcs()),
		_narcs(fst->GetState(s)->GetArcSize()), 
		_i(0) { }

	ArcIterator(StdState *state):
		_arcs(state->Arcs()),
		_narcs(state->GetArcSize()),
		_i(0) { }

	bool Done() const { return _i >= _narcs; }

	const Arc &Value() const 	{ return _arcs[_i];	}

	inline StateId NextState() {return _arcs[_i]._to;}

	void Next() { ++_i; }
	void Reset() { _i=0;}
	void Seek(size_t o) {_i = o;}
	size_t Position() const { return _i;}

	unsigned int Flags() const { return 0;}
	void SetFlags(unsigned , unsigned) { }
private:
	const Arc *_arcs;
	size_t _narcs;
	size_t _i;
};

#include "src/util/namespace-end.h"
#endif

#ifndef __COMPOSE_LAT_H__
#define __COMPOSE_LAT_H__

#include "src/newfst/lattice-fst.h"

#include "src/util/namespace-start.h"
template<class I>
class LatticeComposeItf
{
public:
	virtual ~LatticeComposeItf() { }

	virtual I Start() = 0;

	virtual float Final(I s) = 0;

	virtual bool GetArc(I s, Label ilabel, LatticeArc* oarc) = 0;

	virtual bool GetArc(I s, Label ilabel, I *nextstate, LatticeWeight *lweight, Label *olabel) = 0;
};

// fst w.Value2() * scale
template <typename I>
void ComposeLattice(Lattice *clat, LatticeComposeItf<I> *fst, Lattice *olat, float scale = 1.0);
#include "src/util/namespace-end.h"
#endif

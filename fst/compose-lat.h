#ifndef __COMPOSE_LAT_H__
#define __COMPOSE_LAT_H__

#include "fst/lattice-fst.h"

template<class I>
class LatticeComposeItf
{
public:
	virtual ~LatticeComposeItf() { }

	virtual I Start() = 0;

	virtual float Final(I s) = 0;

	virtual bool GetArc(I s, Label ilabel, LatticeArc* oarc) = 0;
};

// fst w.Value2() * scale
template <typename I>
void ComposeLattice(Lattice *clat, LatticeComposeItf<I> *fst, Lattice *olat, float scale = 1.0);

#endif

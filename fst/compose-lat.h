#ifndef __COMPOSE_LAT_H__
#define __COMPOSE_LAT_H__

#include "fst/lattice-fst.h"

template<class I>
class LatticeComposeItf
{
public:
	LatticeComposeItf();
	~LatticeComposeItf();

	virtual I Start() = 0;

	virtual float Final(I s) = 0;

	virtual bool GetArc(I s, Label ilabel, StdArc* oarc) = 0;
};

template <typename I>
void ComposeLattice(Lattice *clat, LatticeComposeItf<I> *fst, Lattice *olat);

#endif

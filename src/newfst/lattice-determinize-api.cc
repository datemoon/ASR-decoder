
#include "src/newfst/lattice-determinize.h"

#include "src/util/namespace-start.h"
bool DeterminizeLatticeWrapper(Lattice *ifst, Lattice *ofst,
		DeterminizeLatticeOptions opts, bool *debug_ptr)
{
	// first must invert ifst.
	Invert(*ifst);
	// second ifst input must be sort.
	ifst->ArcSort();
	// third determinize.
	LatticeDeterminizer<LatticeWeight ,int> det(ifst, opts);

	if(!det.Determinize(debug_ptr))
		return false;
	det.Output(ofst);
	// end invert ofst
	Invert(*ofst);
	return true;
}

#include "src/util/namespace-end.h"

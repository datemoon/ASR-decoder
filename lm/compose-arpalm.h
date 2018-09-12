#ifndef __COMPOSE_ARPALM_H__
#define __COMPOSE_ARPALM_H__
#include "lm/arpa2fsa.h"
#include "fst/compose-lat.h"


class ComposeArpaLm:public LatticeComposeItf<FsaStateId>
{
private:
	ArpaLm *_lm;
public:
	explicit ComposeArpaLm(ArpaLm *lm):_lm(lm) { }

	virtual FsaStateId Start(); 

	// We cannot use "const" because the pure virtual function in the interface is
	// not const.
	virtual float Final(FsaStateId s);

	virtual bool GetArc(FsaStateId s, Label ilabel, StdArc* oarc);

};

#endif

#ifndef __COMPOSE_ARPALM_H__
#define __COMPOSE_ARPALM_H__
#include "src/newlm/arpa2fsa.h"
#include "src/newfst/compose-lat.h"

#include "src/util/namespace-start.h"
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

	virtual bool GetArc(FsaStateId s, Label ilabel, LatticeArc* oarc);

	virtual bool GetArc(FsaStateId s, Label ilabel, FsaStateId *nextstate, LatticeWeight *lweight, Label *olabel);

};

class ArpaLmScore:public ArpaLm
{
private:
public:
	bool ComputerText(char *text);
	void ConvertText2Lat(char *text, Lattice *lat);
	void ComposeText();
};

void CutLine(char *line, std::vector<std::string> &cut_line);
#include "src/util/namespace-end.h"
#endif

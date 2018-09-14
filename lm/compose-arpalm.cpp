#include <cassert>
#include "lm/compose-arpalm.h"

FsaStateId ComposeArpaLm::Start()
{
	FsaStateId start = _lm->Start();
	FsaArc *arc = NULL;
	assert(_lm->GetArc(start, _lm->BosSymbol(), arc));
	return arc->tostateid;
}

float ComposeArpaLm::Final(FsaStateId s)
{
	FsaArc *arc = NULL;
	float weight = 0.0;
	while(_lm->GetArc(s, _lm->EosSymbol(), arc) == false)
	{
		assert(_lm->GetArc(s, 0, arc));
		s = arc->tostateid;
		weight += arc->weight;
	}
	return weight;
}

bool ComposeArpaLm::GetArc(FsaStateId s, Label ilabel, StdArc* oarc)
{
	FsaArc *arc = NULL;
	oarc->_w = 0;
	while(_lm->GetArc(s, ilabel, arc) == false)
	{
		assert(_lm->GetArc(s, 0, arc));
		s = arc->tostateid;
		oarc->_w += arc->weight;
	}
	oarc->_input = arc->wordid;
	oarc->_output = arc->wordid;
	oarc->_w += (-1.0 * arc->weight);
	oarc->_to = arc->tostateid;
	return true;
}

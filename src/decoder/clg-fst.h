#ifndef __CLG_FST_H__
#define __CLG_FST_H__
#include "src/newfst/optimize-fst.h"


#include "src/util/namespace-start.h"

typedef long long int ClgTokenStateId;

class ClgFst
{
public:
	ClgFst() { }
	
	~ClgFst() { Desotry(); }
	
	bool Init(string clgfst, string hmmfst)
	{
		_clg_fst = new Fst();
		if(_clg_fst->ReadFst(clgfst.c_str()) != true)
		{
			fprintf(stderr,"%s clg load error!\n",clgfst.c_str());
			return false;
		}
		_offset = _clg_fst->TotArc() + 1;
		assert(2147483648 > _offset*4);
		if(ReadHmm(hmmfst.c_str()) != true)
		{
			fprintf(stderr,"%s hmm load error!\n",hmmfst.c_str());
			return false;
		}
		return true;
	}

	void Desotry()
	{
		delete _clg_fst;
		_offset = 0;
		for(size_t i=0;i<_hmm_fst.size();++i)
		{
			if(_hmm_fst[i] != NULL)
			{
				delete _hmm_fst[i];
				_hmm_fst[i] = NULL;
			}
		}
	}

	bool ReadHmm(const char *hmmfst)
	{
		FILE *fp = fopen(hmmfst,"rb");
		if(NULL == fp)
		{
			fprintf(stderr,"fopen %s failed.\n",hmmfst);
			return false;
		}
		int numhmm=0;
		fread(&numhmm,sizeof(int),1,fp);
		_hmm_fst.resize(numhmm+1,NULL);
		for(int i=0;i<numhmm;++i)
		{
			Fst *fst = new Fst();
			bool ret = fst->ReadFst(fp);
			if(ret != true)
			{
				fprintf(stderr,"%s %d hmm error.\n",hmmfst,i);
				return false;
			}
			fst->RmOlalel();
			_hmm_fst[i+1] = fst;
		}
		fclose(fp);
		return true;
	}
	// get hmm state pointer
	inline StdState * GetHmmState(int hmmid, int hmmstateid)
	{
		return _hmm_fst[hmmid]->GetState(hmmstateid);
	}
	// build map relation
	// because _total_states <= _total_arcs + 1, 
	// so 0 - _total_arcs give clg state, 
	StdState *GetState(ClgTokenStateId stateid)
	{
		if (stateid >= _offset)
		{
			// first get arcid
			int arcid = stateid % _offset;
			// get hmmid
			Arc *clg_arc = _clg_fst->GetArc(arcid);
			int hmmid = clg_arc->_input;
			int hmmstateid = (stateid / _offset) - 1;
			return GetHmmState(hmmid, hmmstateid);
		}
		else
		{
			return _clg_fst->GetState(static_cast<StateId>(stateid));
		}
	}

	// judge state in clg or hmm
	bool StateIdInClg(ClgTokenStateId stateid)
	{
		if(stateid < _offset)
			return true;
		else
			return false;
	}
	// get fst start state index
	ClgTokenStateId Start()
	{
		return _clg_fst->Start();
	}
	inline bool IsFinal(ClgTokenStateId stateid)
	{
		if(stateid < _offset)
			return _clg_fst->IsFinal(stateid);
		else
			return false;
	}

	inline float GetGraphScore(ClgTokenStateId curstate, const Arc *arc)
	{
		if(curstate >= _offset)
		{
			if(arc->_input == 0)
			{// it's hmm end state
				int arcid = curstate % _offset;
				Arc *clg_arc = _clg_fst->GetArc(arcid);
				return arc->_w + clg_arc->_w;
			}
		}
		return arc->_w;
	}
	// return arrive state id
	ClgTokenStateId MapClgTokenStateId(ClgTokenStateId curstate, const Arc *arc)
	{
		if(curstate >= _offset )
		{ // curstate in clg arc
			if(arc->_input == 0)
			{ // it's hmm end state
				// first get arcid
				int arcid = curstate % _offset;
				Arc *clg_arc = _clg_fst->GetArc(arcid);
				return clg_arc->_to; // arrive at clg state
			}
			else
			{ // here need judge loop or not.
				int hmmstateid = (curstate / _offset) - 1;
				if(arc->_to == hmmstateid) // it's loop
					return curstate;
				else
					return curstate + _offset;
			}
		}
		else
		{ // curstate in clg state
			if(arc->_input == 0) // nonemit
			{ // arrive clg state
				return arc->_to;
			}
			else
			{ // arrive hmm state
				int arcid = _clg_fst->GetArcId(arc);
				return arcid + _offset;
			}
		}
	}
	inline StateId TotState()
	{
		return _clg_fst->TotState();
	}
	inline int TotArc()
	{
		return _clg_fst->TotArc();
	}

private:
	Fst *_clg_fst;
	vector<Fst *> _hmm_fst;
	ClgTokenStateId _offset; // _clg_fst->TotArc() + 1
};

#include "src/util/namespace-end.h"
#endif

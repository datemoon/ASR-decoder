#include "lm/compose-arpalm.h"
#include "util/log-message.h"

FsaStateId ComposeArpaLm::Start()
{
	FsaStateId start = _lm->Start();
	FsaArc *arc = NULL;
	LOG_ASSERT(_lm->GetArc(start, _lm->BosSymbol(), arc));
	return arc->tostateid;
}

float ComposeArpaLm::Final(FsaStateId s)
{
	FsaArc *arc = NULL;
	float weight = 0.0;
	while(_lm->GetArc(s, _lm->EosSymbol(), arc) == false)
	{
		LOG_ASSERT(_lm->GetArc(s, 0, arc));
		s = arc->tostateid;
		weight += arc->weight;
	}
	weight += arc->weight;
	return (-1.0 * weight);
}

bool ComposeArpaLm::GetArc(FsaStateId s, Label ilabel, LatticeArc* oarc)
{
	FsaArc *arc = NULL;
	float weight = 0.0;
	while(_lm->GetArc(s, ilabel, arc) == false)
	{
		LOG_ASSERT(_lm->GetArc(s, 0, arc));
		s = arc->tostateid;
		weight += arc->weight;
	}
	oarc->_input = arc->wordid;
	oarc->_output = arc->wordid;
	weight += arc->weight;
	oarc->_w.SetValue1(-1 * weight); // graph cost, lm.
	oarc->_w.SetValue2((float)0.0);
	oarc->_to = arc->tostateid;
	return true;
}

void CutLine(char *line, std::vector<std::string> &cut_line)
{
	char *curr_word=NULL;
	char *str_thread = NULL;
	curr_word = strtok_r( line , " \r\n\t" ,&str_thread) ;
	cut_line.push_back(std::string(curr_word));
	while((curr_word = strtok_r( NULL , " \n\r\t" ,&str_thread )) != NULL)
	{
		cut_line.push_back(std::string(curr_word));
	}
}

//
bool ArpaLmScore::ComputerText(char *text)
{
	std::vector<std::string> cut_text;
	std::vector<int> ids;
	CutLine(text,cut_text);
	//ids.push_back(InvertWord2Id("<s>"));
	for(size_t i=0;i<cut_text.size();++i)
		ids.push_back(InvertWord2Id(cut_text[i]));
	ids.push_back(InvertWord2Id("</s>"));
	FsaStateId state = _fsa.Start();
	FsaArc *arc=NULL;
	_fsa.GetArc(state, InvertWord2Id("<s>"), arc);
	state = arc->tostateid;
	int s_ngram = 1;
	int e_ngram = (int)_num_gram.size();
	float tot_score = 0.0;
	for(size_t i=0;i<ids.size();++i)
	{
		float backoff = 0.0,
			  logprob = 0.0;
		while(_fsa.GetArc(state, ids[i], arc) == false)
		{
			s_ngram--;
			LOG_ASSERT(s_ngram >= 0);
			// search back off
			if(_fsa.GetArc(state, 0, arc) == false)
			{
				LOG_WARN << "It shouldn't be happen." ;
				return false;
			}
			state = arc->tostateid;
			backoff += arc->weight;
		}
		logprob = arc->weight;
		state = arc->tostateid;
		s_ngram++;
		if(s_ngram > e_ngram) 
			s_ngram = e_ngram;
		VLOG(2) << logprob << " " << _map_syms[ids[i]] << " " 
			<< backoff << " " <<  logprob+backoff << " " << s_ngram ;
		tot_score += logprob+backoff;
	}
	VLOG(2) << tot_score;
	return true;
}

void ArpaLmScore::ConvertText2Lat(char *text,Lattice *lat)
{
	std::vector<std::string> cut_text;
	std::vector<int> ids;
	CutLine(text,cut_text);
	for(size_t i=0;i<cut_text.size();++i)
		ids.push_back(InvertWord2Id(cut_text[i]));
	StateId stateid = lat->AddState();
	lat->SetStart(stateid);
	for(size_t i=0;i<ids.size();++i)
	{
		StateId nextstate = lat->AddState();
		LatticeArc arc(ids[i],ids[i], nextstate, LatticeWeight::One());
		lat->AddArc(stateid, arc);
		stateid = nextstate;
	}
	lat->SetFinal(stateid, 0.0);
}


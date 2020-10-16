#include "src/newlm/compose-arpalm.h"
#include "src/util/log-message.h"

#include "src/util/namespace-start.h"
FsaStateId ComposeArpaLm::Start()
{
	FsaStateId start = _lm->Start();
	float w_arc = 0.0;
	FsaStateId tostateid = 0;
	if(false ==_lm->GetArc(start, _lm->BosSymbol(), &w_arc, &tostateid))
		LOG_ERR << "get <s> arc failed." ;
	return tostateid;
}

float ComposeArpaLm::Final(FsaStateId s)
{
	float weight = 0.0;
	float w_arc = 0.0;
	FsaStateId tostateid = 0;
	while(_lm->GetArc(s, _lm->EosSymbol(), &w_arc, &tostateid) == false)
	{
		if(_lm->GetArc(s, 0, &w_arc, &tostateid) == false)
			LOG_ERR << "get backoff failed." ;
		s = tostateid;
		weight += w_arc;
	}
	weight += w_arc;
	return (-1.0 * weight);
}

bool ComposeArpaLm::GetArc(FsaStateId s, Label ilabel, LatticeArc* oarc)
{
	float weight = 0.0;
	float w_arc = 0.0;
	FsaStateId tostateid = 0;
	while(_lm->GetArc(s, ilabel, &w_arc, &tostateid) == false)
	{
		if(_lm->GetArc(s, 0, &w_arc, &tostateid) == false)
			LOG_ERR << "get backoff failed." ;
		s = tostateid;
		weight += w_arc;
	}
	oarc->_input = ilabel;
	oarc->_output = ilabel;
	weight += w_arc;
	oarc->_w.SetValue1(-1 * weight); // graph cost, lm.
	oarc->_w.SetValue2((float)0.0);
	oarc->_to = tostateid;
	return true;
}

bool ComposeArpaLm::GetArc(FsaStateId s, Label ilabel, FsaStateId *nextstate, LatticeWeight *lweight, Label *olabel)
{
	float weight = 0.0;
	float w_arc = 0.0;
	FsaStateId tostateid = 0;
	while(_lm->GetArc(s, ilabel, &w_arc, &tostateid) == false)
	{
		if(_lm->GetArc(s, 0, &w_arc, &tostateid) == false)
			LOG_ERR << "get backoff failed." ;
		s = tostateid;
		weight += w_arc;
	}
	weight += w_arc;
	lweight->SetValue1(-1 * weight); // graph cost, lm.
	lweight->SetValue2((float)0.0);
	*olabel = ilabel;
	*nextstate = tostateid;
	return true;
}

void CutLine(char *line, std::vector<std::string> &cut_line)
{
	char *curr_word=NULL;
	char *str_thread = NULL;
	curr_word = strtok_r( line , " \r\n\t" ,&str_thread) ;
	if(curr_word == NULL)
		return ;
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
	float weight = 0;
	FsaStateId tostateid = 0;
	_fsa.GetArc(state, InvertWord2Id("<s>"), &weight, &tostateid);
	state = tostateid;
	int s_ngram = 1;
	int e_ngram = (int)_num_gram.size();
	float tot_score = 0.0;
	for(size_t i=0;i<ids.size();++i)
	{
		float backoff = 0.0,
			  logprob = 0.0;
		while(_fsa.GetArc(state, ids[i], &weight, &tostateid) == false)
		{
			s_ngram--;
			LOG_ASSERT(s_ngram >= 0);
			// search back off
			if(_fsa.GetArc(state, 0, &weight, &tostateid) == false)
			{
				LOG_WARN << "It shouldn't be happen." ;
				return false;
			}
			state = tostateid;
			backoff += weight;
		}
		logprob = weight;
		state = tostateid;
		s_ngram++;
		if(s_ngram > e_ngram) 
			s_ngram = e_ngram;
		VLOG_COM(2) << logprob << " " << _map_syms[ids[i]] << " " 
			<< backoff << " " <<  logprob+backoff/M_LN10 << " " << s_ngram ;
		std::cout << logprob << " " << _map_syms[ids[i]] << " " 
			<< backoff << " " <<  (logprob+backoff)/M_LN10 << " " << s_ngram << std::endl;
		tot_score += logprob+backoff;
	}
	VLOG_COM(2) << tot_score;
	std::cout << tot_score/M_LN10 << std::endl;
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
#include "src/util/namespace-end.h"

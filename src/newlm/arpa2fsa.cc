
#include <iostream>
#include <pthread.h>
#include "src/newlm/arpa2fsa.h"

#include "src/util/namespace-start.h"
static pthread_mutex_t add_fsa_node_mutex = PTHREAD_MUTEX_INITIALIZER;

bool Fsa::Write(FILE *fp)
{
	if(fp == NULL)
	{
		std::cerr << "FILE is NULL" << std::endl;
		return false;
	}
	// first total state number
	if(fwrite((void*)&states_num, sizeof(FsaStateId), 1, fp) != 1)
	{
		std::cerr << "Write total states number failed." << std::endl;
		return false;
	}
	FsaStateId tot_arcs_num = 0;
	// write state info
	for(FsaStateId i=0; i<states_num; ++i)
	{
		// write arc number
		if(fwrite((void*)&states[i].arc_num, sizeof(int),1 ,fp) != 1)
		{
			std::cerr << "Write arc number failed." << std::endl;
			return false;
		}
		// write backoff_prob
		if(fwrite((void*)&states[i].backoff_prob, sizeof(float),1 ,fp) != 1)
		{
			std::cerr << "Write backoff_prob failed." << std::endl;
			return false;
		}
		if(fwrite((void*)&states[i].backoff_id, sizeof(FsaStateId), 1, fp) != 1)
		{
			std::cerr << "Write backoff_id failed." << std::endl;
			return false;
		}
		// write backoff_id
		tot_arcs_num += static_cast<FsaStateId>(states[i].arc_num);
	}
	// write total arc number
	if(fwrite((void*)&tot_arcs_num, sizeof(FsaStateId), 1, fp) != 1)
	{
		std::cerr << "Write total arcs number failed." << std::endl;
		return false;
	}
	for(FsaStateId i=0; i<states_num; ++i)
	{
		if(fwrite((void*)states[i].arc, sizeof(FsaArc), states[i].arc_num ,fp)
			   	!= (size_t)states[i].arc_num)
		{
			std::cerr << "Write arc failed." << std::endl;
			return false;
		}
	}
	return true;
}

struct StateInfo
{
	int arc_num;
	float backoff_prob;
	FsaStateId backoff_id;
};
bool Fsa::Read(FILE *fp)
{
	if(fp == NULL)
	{
		std::cerr << "FILE is NULL" << std::endl;
		return false;
	}
	if(fread((void*)&states_num,sizeof(FsaStateId),1,fp) != 1)
	{
		std::cerr << "Read total states number failed." << std::endl;
		return false;
	}
	states = new FsaState[states_num];
	states_max_len = states_num;
	if(states == NULL)
	{
		std::cerr << "New states failed." << std::endl;
		return false;
	}
	FsaStateId tot_arcs_num = 0,
			   read_arcs_num = 0;
	// read state info
#define READALLSTATE
#ifdef READALLSTATE
	const size_t read_states_num = 10 * 1000 * 1000;
	StateInfo * state_info = new StateInfo[read_states_num];
	if(state_info == NULL)
	{
		std::cerr << "New state_info failed." << std::endl;
		return false;
	}
	size_t rest_states_nums = states_num;
	size_t offset = 0;
	while(rest_states_nums > 0)
	{
		size_t read_len = read_states_num < rest_states_nums ? read_states_num : rest_states_nums;

		if(fread((void*)state_info, sizeof(StateInfo), read_len, fp) != read_len)
		{
			std::cerr << "Read StateInfo failed." << std::endl;
			return false;
		}

		// assignment state info
		for(FsaStateId i=0; i<read_len; ++i)
		{
			states[offset].arc_num = state_info[i].arc_num;
			states[offset].backoff_prob = state_info[i].backoff_prob;
			states[offset].backoff_id = state_info[i].backoff_id;
			tot_arcs_num += static_cast<FsaStateId>(states[offset].arc_num);
			offset++;
		}
		rest_states_nums -= read_len;
		std::cout << "rest states number is " << rest_states_nums << std::endl;
	}
	delete [] state_info;
	state_info = NULL;
#else
	StateInfo state_info;
	for(FsaStateId i=0; i<states_num; ++i)
	{
		if(fread((void*)&state_info, sizeof(StateInfo), 1, fp) != 1)
		{
			std::cerr << "Read StateInfo failed." << std::endl;
			return false;
		}
		states[i].arc_num = state_info.arc_num;
		states[i].backoff_prob = state_info.backoff_prob;
		states[i].backoff_id = state_info.backoff_id;

		tot_arcs_num += static_cast<FsaStateId>(states[i].arc_num);
	}
#endif
	// read tot arcs
	if(fread((void*)&read_arcs_num, sizeof(FsaStateId), 1, fp) != 1)
	{
		std::cerr << "Read total arcs number failed." << std::endl;
		return false;
	}
	assert(read_arcs_num == tot_arcs_num && "Read arc number error.");
	// read all arc info
	arcs = new FsaArc[tot_arcs_num];
	arcs_num = tot_arcs_num;
	std::cout << "Total arc number " << tot_arcs_num << std::endl ;
	if(arcs == NULL)
	{
		std::cerr << "New arcs failed." << std::endl;
		return false;
	}
	else
	{
		std::cout << "New arc ok." << std::endl;
	}
	if(fread((void*)arcs, sizeof(FsaArc), tot_arcs_num, fp) != static_cast<size_t>(tot_arcs_num))
	{
		std::cerr << "Read arc failed." << std::endl;
		return false;
	}
	// assignment arc
	FsaStateId offset_arc_point = 0;
	for(FsaStateId i=0; i<states_num; ++i)
	{
		states[i].arc = arcs+offset_arc_point;
		offset_arc_point += states[i].arc_num;
	}
	return true;
}
/*
bool Fsa::ReadInfo(FILE *fp)
{
	if(fp == NULL)
	{
		std::cerr << "FILE is NULL" << std::endl;
		return false;
	}
	if(fread((void*)&states_num,sizeof(FsaStateId),1,fp) != 1)
	{
		std::cerr << "Read total states number failed." << std::endl;
		return false;
	}
	states = new FsaState[states_num];
	states_max_len = states_num;
	if(states == NULL)
	{
		std::cerr << "New states failed." << std::endl;
		return false;
	}
	FsaStateId tot_arcs_num = 0,
			   read_arcs_num = 0;
	// read state info
	StateRead *stateinf = new StateRead[states_num];
	if(fread((void*)stateinf,sizeof(StateRead),states_num,fp) != states_num)
	{
		std::cerr << "Read state info failed." << std::endl;
		return false;
	}
	for(FsaStateId i=0; i<states_num; ++i)
	{
		states[i].arc_num = stateinf[i].arc_num;
		states[i].backoff_prob = stateinf[i].backoff_prob;
		states[i].backoff_id = stateinf[i].backoff_id;
		tot_arcs_num += static_cast<FsaStateId>(states[i].arc_num);
	}
	delete []stateinf;
	// read tot arcs
	if(fread((void*)&read_arcs_num, sizeof(FsaStateId), 1, fp) != 1)
	{
		std::cerr << "Read total arcs number failed." << std::endl;
		return false;
	}
	assert(read_arcs_num == tot_arcs_num && "Read arc number error.");
	// read all arc info
	arcs = new FsaArc[tot_arcs_num];
	arcs_num = tot_arcs_num;
	if(arcs == NULL)
	{
		std::cerr << "New arcs failed." << std::endl;
		return false;
	}
	if(fread((void*)arcs, sizeof(FsaArc), tot_arcs_num, fp) != static_cast<size_t>(tot_arcs_num))
	{
		std::cerr << "Read arc failed." << std::endl;
		return false;
	}
	// assignment arc
	FsaStateId offset_arc_point = 0;
	for(FsaStateId i=0; i<states_num; ++i)
	{
		states[i].arc = arcs+offset_arc_point;
		offset_arc_point += states[i].arc_num;
	}
	return true;
}
*/
bool Fsa::GetArc(FsaStateId id, int wordid, float *weight, FsaStateId *tostateid)
{
	if(wordid == 0)
	{
		*weight = states[id].backoff_prob;
		*tostateid = states[id].backoff_id;
		return true;
	}
	FsaArc *arc = NULL;
	if(id == start)
		arc = states[id].SearchStartArc(wordid);
	else
		arc = states[id].SearchArc(wordid);
	if(arc == NULL)
		return false;
	*weight = arc->weight;
	*tostateid = arc->tostateid;
	return true;
}

void Fsa::Rescale(float scale)
{
	if(scale == 1)
		return;
	for(FsaStateId i = 0; i < arcs_num; ++i)
	{
		arcs[i].weight *= scale;
	}
	for(FsaStateId i = 0; i < states_num; ++i)
	{
		states[i].backoff_prob *= scale;
	}
	return ;
	std::unordered_set<FsaStateId> state_set;
	std::queue<FsaStateId> state_queue;
	FsaStateId start = Start();
	state_set.insert(start);
	state_queue.push(start);
	while(!state_queue.empty())
	{
		FsaStateId s = state_queue.front();
		state_queue.pop();
		FsaState *state = GetState(s);
		int n = 0;
		FsaArc *arc = NULL;
		state->backoff_prob *= scale;
		while((arc = state->GetArc(n)) != NULL)
		{
			arc->weight *= scale;
			if(state_set.find(arc->tostateid) == state_set.end())
			{
				state_set.insert(arc->tostateid);
				state_queue.push(arc->tostateid);
			}
			n++;
		}
	}
}

void strtoupper( char *str )
{
	if(str == NULL)
		return ;
	for(int i=0 ; i<(int)strlen(str); i++)
		str[i] = toupper(str[i]);
}

FsaStateId Arpa2Fsa::AnasyArpa()
{
	FILE *fp = fopen(_arpafile.c_str(),"r");
	if(fp == NULL)
	{
		std::cerr << "Open " << _arpafile << " error." << std::endl;
		exit(-1);
	}
	char line[1024];
	memset(line,0x00,sizeof(line)*sizeof(char));
	int read_state = NON;
	int cur_gram = 0;
	int cur_thread_n = 0;
	FsaStateId cur_line=0;//record in gram line number.
	FsaStateId cut_line = 0;//record cut line number.
	FsaStateId tot_gram_line = 0;
	while(fgets(line,sizeof(line),fp) != NULL)
	{
		if ( (line[0]==' ') || (line[0]=='\r') || (line[0]=='\n') || (line[0]=='\t') || (line[0]=='#') )
		{
			// if line is start with # \r \n \t '' ,i don't want to read it
			continue;
		}
		if ( line[0] == '\\' )
		{
			strtoupper(line);
			if(strstr(line,"\\DATA\\" ) != NULL)
			{
				 read_state = DATA;
			}
			else if(strstr( line , "-GRAMS:" ) != NULL)
			{
				if(cur_gram != 0)
					_file_offset[cur_gram-1][cur_thread_n-1].line = cut_line;
				cur_thread_n = 0;
				read_state = GRAM;
				++cur_gram;
				cur_line = 0;
				cut_line = 0;
				_file_offset[cur_gram-1][cur_thread_n].offset = ftell(fp);
				++cur_thread_n;
			}
			else if(strstr(line,"\\END\\" ) != NULL)
			{
				if(cur_gram != 0)
					_file_offset[cur_gram-1][cur_thread_n-1].line = cut_line;
			}
			memset(line,0x00,sizeof(line)*sizeof(char));
			continue;
		}
		if(read_state == DATA)
		{
			strtoupper(line);
			if ( strstr( line , "NGRAM" ) != NULL )
			{
				int tmp1 = 0;
			   	FsaStateId tmp2 = 0;
				if ( sscanf( line , "%*s %d=%d" , &tmp1 , &tmp2 ) != 2 )
				//if ( sscanf( line , "%*s %d=%u" , &tmp1 , &tmp2 ) != 2 )
				{
					std::cerr << "arpa file format error when reading ngram n=count" 
						<< std::endl;
					exit(-1);
				}
				_num_gram.push_back(tmp2);
				std::vector<Offset> off;
				off.resize(_nthread);
				_file_offset.push_back(off);
			}
		}
		else if(read_state == GRAM)
		{
			++cut_line;
			++cur_line;
			++tot_gram_line;
			size_t cur_ngram_line = _num_gram[cur_gram-1];
			if(cur_line%((cur_ngram_line + _nthread)/_nthread )== 0)
			{
				// prev thread read end line
				_file_offset[cur_gram-1][cur_thread_n-1].line = cut_line; 
				// cur thread read start offset
				_file_offset[cur_gram-1][cur_thread_n].offset = ftell(fp);
				cut_line = 0;
				++cur_thread_n;
			}
		}
		memset(line,0x00,sizeof(line)*sizeof(char));
	} // read arpa ok
	fclose(fp);
	// statistics every gram total
	FsaStateId tot_line =0;
	for(size_t i =0;i<_file_offset.size();++i)
		for(size_t j=0;j<_file_offset[i].size();++j)
		{
			tot_line += _file_offset[i][j].line;
			//std::cout << _file_offset[i][j].line << std::endl;
		}
	assert(tot_line == tot_gram_line);
	return tot_gram_line;
}

int ArpaLm::ReadSymbols(const char *wordlist)
{
	FILE *fp = fopen(wordlist, "r");
	if(NULL == fp)
	{
		std::cerr << "Open " << wordlist << " failed." << std::endl;
		return -1;
	}
	char line[256];
	memset(line,0x00,sizeof(line));
	int wordid = 0;
	char word[128];
	int word_num = 0;
	memset(word,0x00,sizeof(word));
	int max_wordid = 0;
	while(NULL != fgets(line,sizeof(line),fp))
	{
		line[strlen(line)-1] = '\0';
		if(2 != sscanf(line,"%s %d",word,&wordid))
		{
			std::cerr << "Read line \"" << line << "\" error." << std::endl;
			return -1;
		}
		_symbols[word] = wordid;
		word_num++;
		max_wordid = max_wordid > wordid ? max_wordid:wordid;
		//_symbols.insert(std::make_pair<std::string,int>(std::string(word),wordid));
	}
	_map_syms.resize(max_wordid+1);
	for(auto it = _symbols.begin(); it != _symbols.end(); ++it)
	{
		_map_syms[it->second] = it->first;
	}
	fclose(fp);
	return word_num;
}


bool Arpa2Fsa::AnalyLine(char *line, ArpaLine *arpaline,int gram)
{
	if ( sscanf( line , "%f" , &arpaline->logprob ) != 1 )
	{
		std::cerr << "Read logprob failed,in line: " << line << std::endl;
		return false;
	}
	arpaline->logprob *= M_LN10;
	char *curr_word=NULL;
	//int word_arr[12];
	char *str_thread = NULL;
	strtok_r( line , " \r\n\t" ,&str_thread) ;
	// read words ,words count is curr_n
	for(int i=0 ; i<gram ; ++i)
	{
		if ( (curr_word = strtok_r( NULL , " \n\r\t" ,&str_thread )) == NULL )
		{
			std::cerr << "Read words error:" << line << std::endl;
			return false;
		}
		int wordid = InvertWord2Id(curr_word);
		if(wordid == UnkSymbol())
		{
			std::cerr << "No word " << curr_word << " in " << line << std::endl;
			return false;
		}
		if(wordid == BosSymbol() && i != 0) 
		{
			return false;
		}
		if(wordid == EosSymbol() && i != gram-1) 
			return false;
		arpaline->words.push_back(wordid);
	}
	if(static_cast<size_t>(gram) < _num_gram.size())
	{
		if ( (curr_word = strtok_r( NULL , " \n\r\t" , &str_thread)) == NULL )
			arpaline->backoff_logprob = 0;
		else
		{
			if(sscanf( curr_word , "%f" , &arpaline->backoff_logprob ) != 1)
			{
				std::cerr << "Read back off weight error,line: " << curr_word << std::endl;
				return false;
			}
		}
	}
	else
		arpaline->backoff_logprob = 0;
	arpaline->backoff_logprob *= M_LN10;
	if(arpaline->logprob/M_LN10 < -98.0)
	{
		if(gram == 1 && arpaline->words[0] == BosSymbol())
			return true;
		std::cerr << "logprob is " << arpaline->logprob << std::endl;
		return false;
	}
	return true;
}

bool Arpa2Fsa::AddLineToFsa(ArpaLine *arpaline, 
		FsaStateId &prev_gram_stateid, int gram)
{
	FsaStateId startid = _fsa.Start();
	FsaState *start = _fsa.GetState(startid);
	// first words node
	if(gram == 1)
	{ // add 1 grammer
		while(true)
		{
			pthread_mutex_lock(&add_fsa_node_mutex);
			int arcnum = start->GetArcNum();
			if(arcnum-1 < arpaline->words[0])
			{	
				FsaStateId id = _fsa.AddState();
				assert(startid < id);
				start->AddArc(id, arcnum, 0.0, arc_pool);
				pthread_mutex_unlock(&add_fsa_node_mutex);
			}
			else if (arcnum-1 >= arpaline->words[0])
			{
				FsaArc *arc = start->SearchStartArc(arpaline->words[0]);
				assert(arc->wordid == arpaline->words[0] && "this start arc wordid != arpaline->words[0]");
				arc->weight = arpaline->logprob;
				// add backoff arc
				// if backoff_logprob==0.0 ,add arc for search
				FsaState* tostate = _fsa.GetState(arc->tostateid);
				tostate->backoff_id = startid; tostate->backoff_prob = arpaline->backoff_logprob;
				//tostate->AddArc(startid, 0, arpaline->backoff_logprob, arc_pool);
				pthread_mutex_unlock(&add_fsa_node_mutex);
				break;
			}
		}
	}
	else
	{ // add 2 to n grammer
		FsaState *state = _fsa.GetState(prev_gram_stateid);
		FsaState *tmpstate = state;
		size_t num_words = arpaline->words.size();
		if(prev_gram_stateid == startid)// prev line it's different current line.
		{
			for(size_t i=0;i<num_words-1;++i)
			{
				FsaArc *arc = NULL;
				if(i==0)
					arc = tmpstate->SearchStartArc(arpaline->words[i]);
				else
					arc = tmpstate->SearchArc(arpaline->words[i]);
				if(arc == NULL)
				{	
					// no A B, but have A B C, so I don't add this gram.
					std::cout <<  "Arc shouldn't NULL, so I don't add this gram. -->" ;
					PrintArpaLine(*arpaline);
					//assert(arc != NULL && "arc shouldn't NULL");
					return false;
				}
				FsaStateId tostateid = arc->tostateid;
				prev_gram_stateid = tostateid;
				tmpstate = _fsa.GetState(tostateid);
			}
		}
		// add line
		pthread_mutex_lock(&add_fsa_node_mutex);
		FsaStateId toid = _fsa.AddState();
		assert(prev_gram_stateid < toid);
		tmpstate->AddArc(toid, arpaline->words[num_words-1], arpaline->logprob, arc_pool);
		pthread_mutex_unlock(&add_fsa_node_mutex);

		// add back off
		{
			FsaState *laststate = _fsa.GetState(toid); // line arrive last state ,add backoff in laststate
			FsaStateId tostateid = 0;
			// search backoff stateid
			size_t back_start = 1;
			while(true)
			{
				tmpstate = start;
				size_t i=0;
				for(i=back_start;i<num_words;++i)
				{
					FsaArc *arc = NULL;
					if(i==back_start)
						arc = tmpstate->SearchStartArc(arpaline->words[i]);
					else
						arc = tmpstate->SearchArc(arpaline->words[i]);
					if(arc == NULL )
					{
						if(arpaline->backoff_logprob != 0)
						{
							// have A B C, but no B C.	
							std::cerr << "No search arc. "  << i << " start.-->";
							PrintArpaLine(*arpaline);
						}
						break;
					}
					tostateid = arc->tostateid;
					tmpstate = _fsa.GetState(tostateid);
				}
				if(i == num_words)
					break;
				back_start++;
				assert(back_start<num_words);
			}
			// add backoff arc
			pthread_mutex_lock(&add_fsa_node_mutex);
			laststate->backoff_id = tostateid; laststate->backoff_prob = arpaline->backoff_logprob;
			//laststate->AddArc(tostateid, 0 , arpaline->backoff_logprob, arc_pool);
			pthread_mutex_unlock(&add_fsa_node_mutex);
		}
	}
	return true;
}

bool Arpa2Fsa::NgramToFsa(int gram,int nt)
{
	FsaStateId line_count = 0;
	ArpaLine *prev_arpaline = new ArpaLine(),
			 *cur_arpaline = new ArpaLine(),
			 *tmp_line = NULL;

	FsaStateId prev_stateid = 0;

	FILE *fp = fopen(_arpafile.c_str(),"rb");
	if(fp == NULL)
	{
		std::cerr << "Open " << _arpafile << " failed." << std::endl;
		return false;
	}
	if(fseek(fp, _file_offset[gram-1][nt-1].offset, SEEK_SET) != 0)
	{
		std::cerr << "Fseek " << _arpafile << " failed." << std::endl;
		return false;
	}
	char line[1024];
	memset(line,0x00,sizeof(line));
	while(fgets(line,1024,fp) != NULL)
	{
		cur_arpaline->Reset();
		// if line is start with # \r \n \t '' ,i don't want to read it
		if ( (line[0]==' ') || (line[0]=='\r') || (line[0]=='\n') || 
				(line[0]=='\t') || (line[0]=='#') )
			continue;
		++line_count;
		if(AnalyLine(line, cur_arpaline, gram) != true)
		{
			std::cerr << "AnalyLine failed,the line it's not legal." << gram << std::endl;
			PrintArpaLine(*cur_arpaline);
			memset(line,0x00,sizeof(line));
			continue;
		}
		if(!(*prev_arpaline == *cur_arpaline))
			prev_stateid = _fsa.Start();
		if(AddLineToFsa(cur_arpaline, prev_stateid, gram) != true)
		{
			std::cerr << "AddLineToFsa " << line << " failed." << std::endl;
			prev_stateid = _fsa.Start(); // it's must be reset.
			continue;
			return false;
		}

		// record cur_arpaline
		tmp_line = prev_arpaline;
		prev_arpaline = cur_arpaline;
		cur_arpaline = tmp_line;

		if(line_count >= _file_offset[gram-1][nt-1].line)
			break;
		memset(line,0x00,sizeof(line));
	}
	fclose(fp);
	delete prev_arpaline;
	delete cur_arpaline;
	return true;
}

bool Arpa2Fsa::ConvertArpa2Fsa()
{
	//int numwords = 
	ReadSymbols(_wordlist.c_str());
	FsaStateId tot_grammer_line = AnasyArpa();
	std::cout << "AnasyArpa OK" << std::endl;
	_fsa.InitMem(tot_grammer_line+1024, 0);

	// beacuse arc pool now it's realloc ,
	// so it start must be alloc enough memory
	NewArcPool(tot_grammer_line+1024);
	FsaStateId startstate = _fsa.AddState();
	_fsa.SetStart(startstate);

	_bos_symbol = _symbols["<s>"];
	_eos_symbol = _symbols["</s>"];
	_unk_symbol = _symbols["<unk>"];

	std::vector<pthread_t> thread;
	thread.resize(_nthread, 0);
	std::vector<ArgsT> args;
	args.resize(_nthread);

	// ngram
	for(size_t i=0; i<_file_offset.size(); ++i)
	{
		std::cout << i+1 << " gram start." << std::endl;
		// nthread
		for(size_t j=0; j<_file_offset[i].size(); ++j)
		{
			args[j].gram = i+1;
			args[j].nthread = j+1;
			args[j].arp2fsa = this;
			if(0 != pthread_create(&thread[j], NULL, Arpa2FsaThread, (void *)&args[j]))
			{
				std::cerr << i << " " << j << " pthread_create failed!" << std::endl;
				return false;
			}
		}
		int flags = 0;
		for(size_t j=0; j<_file_offset[i].size(); ++j)
		{
			void *rt = 0;
			if(pthread_join(thread[j],&rt) != 0)
			{
				std::cerr << i+1 << " gram: " << j << " thread error."<< std::endl;
				++flags;
			}
		}
		if(flags != 0)
			return false;
	}
	return true;
}
#include "src/util/namespace-end.h"

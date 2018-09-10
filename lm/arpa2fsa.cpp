
#include <iostream>
#include <cassert>
#include <pthread.h>
#include "lm/arpa2fsa.h"

static pthread_mutex_t add_fsa_node_mutex = PTHREAD_MUTEX_INITIALIZER;

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
				if ( sscanf( line , "%*s %d=%lld" , &tmp1 , &tmp2 ) != 2 )
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
			assert((size_t)cur_gram == _num_gram.size());
			size_t cur_ngram_line = _num_gram[cur_gram-1];
			if(cur_line%(cur_ngram_line + _nthread)/_nthread == 0)
			{
				// prev thread read end line
				_file_offset[cur_gram-1][cur_thread_n-1].line = cur_line; 
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
			tot_line += _file_offset[i][j].line;
	assert(tot_line == tot_gram_line);
	return tot_gram_line;
}

int Arpa2Fsa::ReadSymbols()
{
	FILE *fp = fopen(_wordlist.c_str(), "r");
	if(NULL == fp)
	{
		std::cerr << "Open " << _wordlist << " failed." << std::endl;
		return -1;
	}
	char line[256];
	memset(line,0x00,sizeof(line));
	int wordid = 0;
	char word[128];
	int word_num = 0;
	memset(word,0x00,sizeof(word));
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
		//_symbols.insert(std::make_pair<std::string,int>(std::string(word),wordid));
	}
	fclose(fp);
	return word_num;
}
/*
inline int Arpa2Fsa::InvertWord2Id(std::string wordstr)
{
	if(_symbols[wordstr] != _symbols.end())
		return _symbols[wordstr];
	else
		return -1;
}
*/
bool Arpa2Fsa::AnalyLine(char *line, ArpaLine *arpaline,int gram)
{
	if ( sscanf( line , "%f" , &arpaline->logprob ) != 1 )
	{
		std::cerr << "Read logprob failed,in line: " << line << std::endl;
		return false;
	}
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
		if(wordid < 0)
		{
			std::cerr << "No word " << curr_word << " in " << line << std::endl;
			return false;
		}
		if(wordid == _fsa.BosSymbol() && i != 0) 
		{
			return false;
		}
		if(wordid == _fsa.EosSymbol() && i != gram-1) 
			return false;
		else
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
				std::cerr << "Read back off weight error,line: " << line << std::endl;
				return false;
			}
		}
	}
	else
		arpaline->backoff_logprob = 0;
	return true;
}

bool Arpa2Fsa::AddLineToFsa(ArpaLine *arpaline, 
		FsaStateId &prev_gram_stateid, int gram)
{
	FsaStateId startid = _fsa.Start();
	// first words node
	if(gram == 1)
	{
		FsaState *start = _fsa.GetState(startid);
		while(true)
		{
			int arcnum = start->GetArcNum();
			if(arcnum-1 < arpaline->words[0])
			{	
				pthread_mutex_lock(&add_fsa_node_mutex);
				FsaStateId id = _fsa.AddState();
				start->AddArc(id, arcnum, 0.0);
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
				pthread_mutex_lock(&add_fsa_node_mutex);
				tostate->AddArc(startid, 0, arpaline->backoff_logprob);
				pthread_mutex_unlock(&add_fsa_node_mutex);
				break;
			}
		}
	}
	else
	{
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
				assert(arc == NULL && "arc shouldn't NULL");
				FsaStateId tostateid = arc->tostateid;
				prev_gram_stateid = tostateid;
				tmpstate = _fsa.GetState(tostateid);
			}
		}
		// add line
		pthread_mutex_lock(&add_fsa_node_mutex);
		FsaStateId toid = _fsa.AddState();
		tmpstate->AddArc(toid, arpaline->words[num_words-1], arpaline->logprob);
		pthread_mutex_unlock(&add_fsa_node_mutex);

		// add back off
		{
			FsaState *laststate = _fsa.GetState(toid); // line arrive last state ,add backoff in laststate
			FsaStateId tostateid = 0;
			// search backoff stateid
			size_t back_start = 1;
			while(true)
			{
				tmpstate = state;
				for(size_t i=back_start;i<num_words;++i)
				{
					FsaArc *arc = NULL;
					if(i==back_start)
						arc = tmpstate->SearchStartArc(arpaline->words[i]);
					else
						arc = tmpstate->SearchArc(arpaline->words[i]);
					if(arc == NULL)
					{
						std::cerr << "No search arc,shouldn't happen." << std::endl;
						break;
					}
					tostateid = arc->tostateid;
				}
				if(tostateid != 0)
					break;
				back_start++;
				assert(back_start<num_words);
			}
			// add backoff arc
			pthread_mutex_lock(&add_fsa_node_mutex);
			laststate->AddArc(tostateid, 0 , arpaline->backoff_logprob);
			pthread_mutex_unlock(&add_fsa_node_mutex);
		}
	}
	return true;
}

bool Arpa2Fsa::NgramToFsa(int gram,int nt)
{
	FsaStateId line_count = 0;
	ArpaLine *prev_arpaline = new ArpaLine(),
			 *cur_arpaline = new ArpaLine();

	FsaStateId prev_stateid = 0;

	FILE *fp = fopen(_arpafile.c_str(),"r");
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
		if(AnalyLine(line, cur_arpaline, gram) != true)
		{
			std::cerr << "AnalyLine " << line << " failed." << std::endl;
			return false;
		}
		if(!(*prev_arpaline == *cur_arpaline))
			prev_stateid = _fsa.Start();
		if(AddLineToFsa(cur_arpaline, prev_stateid, gram) != true)
		{
			std::cerr << "AddLineToFsa " << line << " failed." << std::endl;
			return false;
		}

		// record cur_arpaline
		prev_arpaline = cur_arpaline;
		++line_count;
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
	ReadSymbols();
	FsaStateId tot_grammer_line = AnasyArpa();
	_fsa.InitMem(tot_grammer_line+1000, 0);
	FsaStateId startstate = _fsa.AddState();
	_fsa.SetStart(startstate);

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

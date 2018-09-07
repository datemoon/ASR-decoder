
#include <pthread.h>
#include "lm/arpa2fsa.h"

static pthread_mutex_t add_fsa_node_mutex = PTHREAD_MUTEX_INITIALIZER;

FsaStateId Arap2Fsa::AnasyArpa(const char *file)
{
	FILE *fp = fopen(file,"r");
	if(fp == NULL)
	{
		std::cerr << "Open " << file << " error." << std::endl;
		exit(-1);
	}
	char line[1024];
	memset(line,0x00,sizeof(line)*sizeof(char));
	int read_state = NON;
	int cur_gram = 0;
	int cur_thread_n = 0;
	int64 cur_line=0;//record in gram line number.
	int64 cut_line = 0;//record cut line number.
	int64 tot_gram_line = 0;
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
					file_offset[cur_gram-1][cur_thread_n-1].line = cut_line;
				cur_thread_n = 0;
				read_state = GRAM;
				++cur_gram;
				cur_line = 0;
				cut_line = 0;
				file_offset[cur_gram-1][cur_thread_n].offset = ftell(fp);
				++cur_thread_n;
			}
			else if(strstr(line,"\\END\\" ) != NULL)
			{
				if(cur_gram != 0)
					file_offset[cur_gram-1][cur_thread_n-1].line = cut_line;
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
			   	int64 tmp2 = 0;
				if ( sscanf( line , "%*s %d=%lld" , &tmp1 , &tmp2 ) != 2 )
				{
					std::cerr << "arpa file format error when reading ngram n=count" 
						<< std::endl;
					exit(-1);
				}
				_num_gram.push_back(tmp2);
				Offset off;
				off.resize(_nthread);
				_file_offset.push_back(off);
			}
		}
		else if(read_state == GRAM)
		{
			++cut_line;
			++cur_line;
			++tot_gram_line;
			assert(cur_gram == _num_gram.size());
			size_t cur_ngram_line = _num_gram[cur_gram-1];
			if(cur_line%(cur_gram_line+nthread)/nthread == 0)
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
	int64 tot_line =0;
	for(size_t i =0;i<_file_offset.size();++i)
		for(size_t j=0;j<_file_offset[i].size();++j)
			tot_line += _file_offset[i][j].line;
	assert(tot_line == tot_gram_line);
	return tot_gram_line;
}

int Arap2Fsa::ReadSymbols(const char *file)
{
	FILE *fp = fopen(file, "r");
	if(NULL == fp)
	{
		std::cerr << "Open " << file << " failed." << std::endl;
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

int Arpa2Fsa::InvertWord2Id(std::string wordstr)
{
	if(_symbols[wordstr] != _symbols.end())
		return _symbols[wordstr];
	else
		return -1;
}

bool Arpa2Fsa::AnalyLine(char *line, ArpaLine *arpaline,int gram)
{
	if ( sscanf( line , "%f" , &arpaline->logprob ) != 1 )
	{
		std::cerr << "Read logprob failed,in line: " << line << std::endl;
		return false;
	}
	char *curr_word=NULL;
	int word_arr[12];
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
		if(word_id < 0)
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
	if(gram < _num_gram.size())
	{
		if ( (curr_word = strtok_r( NULL , " \n\r\t" , &str_thread)) == NULL )
			arpaline->backoff_logprob = 0;
		else
		{
			sscanf( curr_word , "%f" , &arpaline->backoff_logprob ) != 1
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

FsaStateId Arpa2Fsa::AddLineToFsa(ArpaLine *arpaline, FsaStateId prev_gram_stateid)
{
	// first words node
	FsaStateId to_state;
	FsaState* state = states[prev_gram_stateid];
	for(size_t i=0;i<arpaline->words.size();++i)
	{
		FasArc *arc = _fst.SearchArc(arpaline->words[i]);

	}
}

bool Arpa2Fsa::ConvertToFsa(const char *arpa_file,int gram,int nt)
{
	int64 line_count = 0;
	ArpaLine *prev_arpaline = new ArpaLine(),
			 *cur_arpaline = new ArpaLine();

	FsaStateId prev_stateid = 0;

	FILE *fp = fopen(arpa_file,"r");
	if(fp == NULL)
	{
		std::cerr << "Open " << arpa_file << " failed." << std::endl;
		return false;
	}
	if(fseek(fp, _file_offset[gram-1][nt-1].offset, SEEK_SET) != 0)
	{
		std::cerr << "Fseek " << arpa_file << " failed." << std::endl;
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
		if(*prev_arpaline != *cur_arpaline)
			prev_stateid = 0;
		if(AddLineToFsa(cur_arpaline) != true)
		{
			std::cerr << "AddLineToFsa " << line << " failed." << std::endl;
			return false;
		}

		// record cur_arpaline
		prev_arpaline = cur_arpaline
		
	}
	delete prev_arpaline;
	delete cur_arpaline;
}

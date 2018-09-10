#ifndef __ARPA2FSA_H__
#define __ARPA2FSA_H__

#include <unordered_map>
#include <cstring>
#include <vector>
/*
 * This class change arpa lm to fsa.
 * */
struct FsaState;
typedef long long int FsaStateId;
struct FsaArc
{
	int wordid;      // word id
	float weight;    // weight in arc , if word id is 0 ,weight is backoff 
	FsaStateId tostateid;
	//struct FsaState *state; // arrive state point
	FsaArc():wordid(0),weight(0),tostateid(0) { }
	~FsaArc()
	{
		wordid = 0;
		weight = 0.0;
		tostateid = 0;
	}
};

struct FsaState
{
	FsaArc *arc;
	int arc_num; // record arc real number
	int max_len; // record arc max number
	FsaState():arc(NULL),arc_num(0),max_len(0) { } 
	FsaState(int n):arc_num(0), max_len(n)
	{
		arc = new FsaArc[n];
	}
	~FsaState()
	{
		if(arc != NULL)
		{
			Delete();
		}
	}
	int GetArcNum() { return arc_num; }
	void Delete()
	{
		if(arc != NULL)
		{
			delete [] arc;
			arc=NULL;
			arc_num=0;
			max_len=0;
		}
	}

	void Reset()
	{
		arc = NULL;
		arc_num = 0;
		max_len = 0;
	}

	// add arc and sort
	void AddArc(FsaStateId tostateid,int wordid,float weight, int alloc_size=10)
	{
		// realloc arc
		if(arc_num >= max_len)
		{
			FsaArc *tmp = new FsaArc[max_len + alloc_size];
			max_len += alloc_size;
			if(arc != NULL)
			{
				memcpy(tmp, arc, sizeof(FsaArc)*arc_num);
				delete[] arc;
			}
			arc = tmp;
		}
		// sort arc
		int i = arc_num;
		while(i>0)
		{
			if(arc[i-1].wordid > wordid)
			{
				arc[i].wordid = arc[i-1].wordid;
				arc[i].weight = arc[i-1].weight;
				arc[i].tostateid = arc[i-1].tostateid;
				--i;
			}
			else
				break;
		}
		// valuation
		arc[i].wordid = wordid;
		arc[i].weight = weight;
		arc[i].tostateid = tostateid;
		arc_num++;
	}

	// arc is sort, so binary search
	FsaArc *SearchArc(int wordid)
	{
		int start = 0,
			end = arc_num-1,
			mid = (start + end)/2;
		while(start <= end)
		{
			if(arc[mid].wordid > wordid)
				end = mid - 1;
			else if(arc[mid].wordid < wordid)
				start = mid + 1;
			else
				return &(arc[mid]);
			mid = (start + end)/2;
		}
		return NULL;
	}
	FsaArc *SearchStartArc(int wordid)
	{
		return &(arc[wordid]);
	}
};

class Fsa
{
private:
	FsaState *states; // Fsa state save memory
	FsaArc *arcs;
	FsaStateId start;
	FsaStateId arcs_num;
	FsaStateId states_num;    // state real number
	FsaStateId states_max_len;      // state max number
	FsaStateId add_num;
	int bos_symbol; // begin symbol
	int eos_symbol; // end symbol
	int unk_symbol; // <unk>
	int ngram_order; //
	int num_words;  // word list number, no use now.
	bool continuous_storage;
public:
	Fsa(bool con_storage=false):
		states(NULL), arcs(NULL), start(0), arcs_num(0), states_num(0), 
		states_max_len(0), add_num(10000), bos_symbol(-1), eos_symbol(-1), 
		unk_symbol(-1), num_words(0), continuous_storage(con_storage) { }

	Fsa(FsaStateId states_max_len, bool con_storage=true):
		states(NULL),arcs(NULL), start(0), arcs_num(0), states_num(0),
		states_max_len(states_max_len), add_num(10000), bos_symbol(-1), 
		eos_symbol(-1), unk_symbol(-1), num_words(0), continuous_storage(con_storage) 
	{
	   states = new FsaState[states_max_len];	
	}
	

	~Fsa()
	{
		if(arcs != NULL)
		{
			for(FsaStateId i=0; i<states_num ;++i)
			{
				states[i].Reset();
			}
			delete[] arcs;
			arcs_num = 0;
		}
		delete[] states;
		states_num=0;
		states_max_len=0;
	}

	// alloc memory
	void InitMem(FsaStateId states_n, FsaStateId arcs_n=0)
	{
		if(states == NULL && states_n != 0)
		{
			states = new FsaState[states_n];
			states_num = 0;
			states_max_len = states_n;
		}
		if(arcs == NULL && arcs_n != 0)
		{
			arcs = new FsaArc[arcs_n];
			arcs_num = arcs_n;
		}
	}

	FsaStateId Start()
	{
		return start;
	}

	void SetStart(FsaStateId s)
	{
		start = s;
	}
	FsaState* GetState(FsaStateId id)
	{
		return &(states[id]);
	}

	FsaStateId AddState()
	{
		// realloc
		if(states_num >= states_max_len)
		{
			FsaState *tmp = new FsaState[states_max_len + add_num];
			states_max_len += add_num;
			if(states != NULL)
			{
				memcpy(tmp, states, sizeof(FsaState)*states_max_len);
				delete [] states;
			}
			states = tmp;
		}
		states_num++;
		return (states_num-1);
	}

	int BosSymbol() const { return bos_symbol; }
	int EosSymbol() const { return eos_symbol; }
	int UnkSymbol() const { return unk_symbol; }
	int NgramOrder() const { return ngram_order; }
};

class Arpa2Fsa;
struct ArgsT
{
	int gram;
	int nthread;
	Arpa2Fsa *arp2fsa;
//	ArgsT(int gram, int nthread, Arpa2Fsa *arp2fsa):
//		gram(gram), nthread(nthread), arp2fsa(arp2fsa) { }
};

class Arpa2Fsa
{
	struct ArpaLine;
public:
	Arpa2Fsa(int nthread, std::string arpafile, std::string wordlist=""):
		_nthread(nthread), _arpafile(arpafile), _wordlist(wordlist) { }

	// anasy Arpa file for thread
	// know every threads read start position
	// return state approximate number.
	FsaStateId AnasyArpa();

	// Read  word list.
	int ReadSymbols();

	// Invert word to wordid
	inline int InvertWord2Id(std::string wordstr)
	{
		std::unordered_map<std::string, int>::const_iterator iter = _symbols.find(wordstr);
		if(iter != _symbols.end())
			return iter->second;
		else
			return -1;
	}



	// convert one line to arpaline
	bool AnalyLine(char *line, ArpaLine *arpaline,int gram);

	// add one line grammer to fsa.
	bool AddLineToFsa(ArpaLine *arpaline,
			FsaStateId &prev_gram_stateid, int gram);

	bool NgramToFsa(int gram,int nt);

	bool ConvertArpa2Fsa();
private:
	struct ArpaLine 
	{
		std::vector<int> words; // Sequence of words to be printed.
		float logprob;          // Logprob corresponds to word sequence.
		float backoff_logprob;  // Backoff_logprob corresponds to word sequence.
		ArpaLine(): logprob(0), backoff_logprob(0) { }
		void Reset()
		{
			words.clear();
			logprob = 0;
			backoff_logprob = 0;
		}
		inline bool operator==(const ArpaLine &other) const
		{
			for(size_t i=0;i<words.size()-1;++i)
			{
				if(words[i] != other.words[i])
					return false;
			}	
			return true;
		}
	};
private:
	static void *Arpa2FsaThread(void *arg)
	{
		ArgsT *args = (ArgsT *)arg;
		if(args->arp2fsa->NgramToFsa(args->gram, args->nthread) != true)
		{
			int rt = -1 * args->nthread;
			pthread_exit((void*)&rt);
		}
		return NULL;
	}
private:
	int _nthread;
	std::string _arpafile;
	std::string _wordlist;
	std::vector<size_t> _num_gram; // record ngram line number.
	struct Offset
	{
		FsaStateId line;//read how many line, it's end;
		FsaStateId offset;//file offset,it's start
		Offset():line(0), offset(0) { }
	};

	enum {NON,DATA,GRAM};
	
	std::vector<std::vector<Offset> > _file_offset;
	std::unordered_map<std::string, int> _symbols;

	Fsa _fsa;
};


#endif

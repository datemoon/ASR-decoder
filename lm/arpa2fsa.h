#ifndef __ARPA2FSA_H__
#define __ARPA2FSA_H__

#include <unordered_map>
#include <cstring>
#include <vector>
/*
 * This class change arpa lm to fsa.
 * */
struct FsaState;
typedef size_t FsaStateId;
struct FsaArc
{
	int wordid;      // word id
	float weight;    // weight in arc , if word id is 0 ,weight is backoff 
	FsaStateId tostateid;
	//struct FsaState *state; // arrive state point
	FsaArc():wordid(0),weight(0),state(NULL) { }
	~FsaArc()
	{
		wordid = 0;
		weight = 0.0;
		state = NULL;
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
			delete [] arc;
			arc = NULL;
		}
		arc_num = 0;
		max_len = 0;
	}
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
	void AddArc(int tostateid,int wordid,float weight, int alloc_size=10)
	{
		// realloc arc
		if(arc_num >= max_len)
		{
			FasArc *tmp = new FsaArc[max_len + alloc_size];
			max_len += alloc_size;
			if(arc != NULL)
			{
				memcpy(tmp, arc, sizeof(FasArc)*arc_num);
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
	FasArc *SearchArc(int wordid)
	{
		int start = 0,
			end = num_arc-1,
			mid = (start + end)/2;
		while(start <= end)
		{
			if(arc[mid].wordid > wordid)
				end = mid - 1;
				min = (start + end)/2;
			if(arc[mid].wordid < wordid)
				start = mid + 1;
			else
				return &(arc[mid]);
			min = (start + end)/2;
		}
		return NULL;
	}
};

class Fsa
{
private:
	FsaState *states; // Fsa state save memory
	FsaArc *arcs;
	FsaStateId arcs_num;
	FsaStateId states_num;    // state real number
	FsaStateId states_max_len;      // state max number
	FsaStateId add_num;
	int bos_symbol; // begin symbol
	int eos_symbol; // end symbol
	int unk_symbol; // <unk>
	int ngram_order; //
	int num_words;  // word list number
	bool contious_storage;
public:
	Fsa(bool contious_storage=false):
		states(NULL),arcs(NULL), arcs_num(0), states_num(0), 
		states_max_len(0), add_num(100000), bos_symbol(-1), eos_symbol(-1), 
		unk_symbol(-1), num_words(0), con_storage(con_storage) { }

	Fsa(FsaStateId states_max_len, bool contious_storage=true):
		states(NULL),arcs(NULL), arcs_num(0), states_num(0),
		states_max_len(states_max_len), add_num(100000), bos_symbol(-1), 
		eos_symbol(-1), unk_symbol(-1), num_words(0), con_storage(con_storage) 
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

	FsaStateId AddState()
	{
		// realloc
		if(states_num >= states_max_len)
		{
			FsaState *tmp = new FsaState[states_max_len + add_num];
			states_max_len += add_num;
			if(states != NULL)
			{
				delete [] states;
				memcpy(tmp, states, sizeof(FasState)*states_max_len);
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

class Arap2Fsa
{
public:
	Arap2Fsa(int nthread):_nthread(nthread) { }
	
	// anasy Arpa file for thread
	// know every threads read start position
	// return state approximate number.
	FsaStateId AnasyArpa(const char *file);

	// Read  word list.
	int ReadSymbols(const char *file);

	// Invert word to wordid
	inline int InvertWord2Id(std::string wordstr);

	bool AddLineToFsa(ArpaLine &arpaline);
private:
	int _nthread;
	std::vector<size_t> _num_gram; // record ngram line number.
	struct Offset
	{
		int64 line;//read how many line, it's end;
		int64 offset;//file offset,it's start
		Offset():line(0), offset(0) { }
	};

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

	bool AnalyLine(char *line, ArpaLine *arpaline, int gram);
	
	std::vector<std::vector<Offset> > _file_offset;
	std::unordered_map<std::string, int> _symbols;

	Fsa _fsa;
};


#endif

#ifndef __ARPA2FSA_H__
#define __ARPA2FSA_H__

#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <cstring>
#include <vector>
#include <iostream>
#include "src/util/util-common.h"
#include "src/util/hash-key.h"
/*
 * This class change arpa lm to fsa.
 * */
#include "src/util/namespace-start.h"
#ifndef M_LN10
#define M_LN10 2.302585092994045684017991454684
#endif
struct FsaState;
//typedef unsigned long long int FsaStateId;
//typedef unsigned int FsaStateId;
typedef int FsaStateId;
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

// add arc pool
struct ArcPool
{
	FsaArc * arcs_pool;
	FsaStateId arcs_num;
	FsaStateId total_num;
	ArcPool(FsaStateId total_num=0):
		arcs_pool(NULL),
		arcs_num(0), total_num(total_num)
	{
		if(total_num !=0)
			arcs_pool = new FsaArc[total_num];
	}
	~ArcPool()
	{
		if(arcs_pool != NULL)
			delete[] arcs_pool;
		arcs_num = 0;
		total_num = 0;
	}
	FsaArc *New()
	{
		if(arcs_num >= total_num)
		{
			std::cout << "realloc fsaarc " << total_num << std::endl;
			total_num += 1024*8;
			FsaArc *tmp = new FsaArc[total_num];
			if(arcs_pool != NULL)
			{
				memcpy(tmp, arcs_pool, sizeof(FsaArc)*arcs_num);
				delete []arcs_pool;
			}
			arcs_pool = tmp;
		}
		arcs_num++;
		return &(arcs_pool[arcs_num-1]);
	}
};

struct FsaState
{
	FsaArc *arc;
	int arc_num; // record arc real number
	int max_len; // record arc max number
	float backoff_prob;
	FsaStateId backoff_id;
	FsaState():arc(NULL),arc_num(0),max_len(0),backoff_prob(0.0),backoff_id(0) { } 
	/*
	FsaState(int n):arc_num(0), max_len(n)
	{
		arc = new FsaArc[n];
	}*/
	~FsaState()
	{
		if(arc != NULL && max_len != 0)
		{
			Delete();
		}
	}
	int GetArcNum() { return arc_num; }
	FsaArc* GetArc(int n)
	{
		if(n < arc_num)
			return &arc[n];
		return NULL;
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
	void AddArc(FsaStateId tostateid,int wordid,float weight, int alloc_size=5)
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

	void AddArc(FsaStateId tostateid,int wordid,float weight, ArcPool *arc_pool)
	{
		// new arc
		if(arc_num == 0)
		{
			arc = arc_pool->New();
		}
		else
		{
			FsaArc *arc_tmp = arc_pool->New();
			if(arc_tmp - arc != arc_num)
				std::cout << "arc offset " << arc_tmp - arc << " " << arc_num << std::endl;
			LOG_ASSERT(arc_tmp - arc == arc_num);
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
	FsaArc tmp_arc;
public:
	Fsa():
		states(NULL), arcs(NULL), start(0), arcs_num(0), states_num(0), 
		states_max_len(0), add_num(10000) { }

	Fsa(FsaStateId states_max_len):
		states(NULL),arcs(NULL), start(0), arcs_num(0), states_num(0),
		states_max_len(states_max_len), add_num(10000)
	{
	   states = new FsaState[states_max_len];	
	}
	
	void Info(int n=0)
	{
		VLOG_COM(n) << "Fsa info : start -> " << start 
			<< " arcs_num -> " << arcs_num
			<< " states_num -> " << states_num;
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

	bool GetArc(FsaStateId id, int wordid, float *weight, FsaStateId *tostateid);

	bool Write(FILE *fp);
	bool Write(const char *file)
	{
		FILE *fp = fopen(file, "wb");
		if(fp == NULL)
		{
			std::cerr << "Open " << file << " failed." << std::endl;
			return false;
		}
		bool ret = Write(fp);
		fclose(fp);
		return ret;
	}
	bool Read(FILE *fp);
//	bool ReadInfo(FILE *fp);

	bool Read(const char *file)
	{
		FILE *fp = fopen(file, "rb");
		if(fp == NULL)
		{
			std::cerr << "Open " << file << " failed." << std::endl;
			return false;
		}
		bool ret = Read(fp);
		fclose(fp);
		return ret;
	}

	// rescale arc weight*scale
	void Rescale(float scale);
};

class ArpaLm
{
protected:
	int _bos_symbol; // begin symbol
	int _eos_symbol; // end symbol
	int _unk_symbol; // <unk>
	std::vector<int> _num_gram; // record ngram line number.
	Fsa _fsa;
	//typedef std::unordered_map<std::string, int, StringKey, StringEqual> WordHash;
	typedef std::unordered_map<std::string, int> WordHash;
	WordHash _symbols;
	std::vector<std::string> _map_syms;
	ArcPool *arc_pool;
	void NewArcPool(FsaStateId size)
	{
		arc_pool = new ArcPool(size);
	}
public:
	ArpaLm():_bos_symbol(-1), _eos_symbol(-1), _unk_symbol(-1), arc_pool(NULL) { }

	~ArpaLm()
	{
		if(arc_pool != NULL)
			delete arc_pool;
	}

	void Info(int n)
	{
		_fsa.Info(0);
	}
	// Read  word list.
	int ReadSymbols(const char* file);
	
	// Invert word to wordid
	inline int InvertWord2Id(std::string wordstr)
	{
		WordHash::const_iterator iter = _symbols.find(wordstr);
		if(iter != _symbols.end())
			return iter->second;
		else
			return _unk_symbol;
	}

	void Rescale(float scale) { _fsa.Rescale(scale); }
	FsaStateId Start() { return _fsa.Start(); }

	bool GetArc(FsaStateId id, int wordid, float *weight, FsaStateId * tostateid)
	{
		return _fsa.GetArc(id, wordid, weight, tostateid);
	}
	int BosSymbol() const { return _bos_symbol; }
	int EosSymbol() const { return _eos_symbol; }
	int UnkSymbol() const { return _unk_symbol; }
	int NgramOrder() const { return static_cast<int>(_num_gram.size()); }
	bool Read(const char *file)
	{
		FILE *fp = fopen(file, "rb");
		if(fp == NULL)
		{
			std::cerr << "Open " << file << " failed." << std::endl;
			return false;
		}
		if(fread((void*)&_bos_symbol,sizeof(int),1,fp) != 1)
		{
			std::cerr << "Read _bos_symbol failed" << std::endl;
			return false;
		}
		if(fread((void*)&_eos_symbol,sizeof(int),1,fp) != 1)
		{
			std::cerr << "Read _eos_symbol failed" << std::endl;
			return false;
		}
		if(fread((void*)&_unk_symbol,sizeof(int),1,fp) != 1)
		{
			std::cerr << "Read _unk_symbol failed" << std::endl;
			return false;
		}
		size_t ngram = 0;
		if(fread((void*)&ngram,sizeof(ngram),1,fp) != 1)
		{
			std::cerr << "Read ngram failed" << std::endl;
			return false;
		}
		_num_gram.resize(ngram);
		if(fread((void*)_num_gram.data(),sizeof(int),_num_gram.size(),fp)
				!= _num_gram.size())
		{
			std::cerr << "Read num gram info failed." << std::endl;

			return false;
		}
		bool ret = _fsa.Read(fp);
		fclose(fp);
		return ret;
	}

	bool Write(const char *file)
	{
		FILE *fp = fopen(file, "wb");
		if(fp == NULL)
		{
			std::cerr << "Open " << file << " failed." << std::endl;
			return false;
		}
		if(fwrite((void*)&_bos_symbol,sizeof(int),1,fp) != 1)
		{
			std::cerr << "Write _bos_symbol failed" << std::endl;
			return false;
		}
		if(fwrite((void*)&_eos_symbol,sizeof(int),1,fp) != 1)
		{
			std::cerr << "Write _eos_symbol failed" << std::endl;
			return false;
		}
		if(fwrite((void*)&_unk_symbol,sizeof(int),1,fp) != 1)
		{
			std::cerr << "Write _unk_symbol failed" << std::endl;
			return false;
		}
		size_t ngram = _num_gram.size();
		if(fwrite((void*)&ngram,sizeof(ngram),1,fp) != 1)
		{
			std::cerr << "Write ngram failed" << std::endl;
			return false;
		}
		if(fwrite((void*)_num_gram.data(),sizeof(int),_num_gram.size(),fp)
				!= _num_gram.size())
		{
			std::cerr << "Write num gram info failed" << std::endl;
			return false;
		}
		bool ret = _fsa.Write(fp);
		fclose(fp);
		return ret;
	}
};


class Arpa2Fsa;
struct ArgsT
{
	int gram;
	int nthread;
	Arpa2Fsa *arp2fsa;
};

class Arpa2Fsa:public ArpaLm
{
	struct ArpaLine;
public:
	Arpa2Fsa(int nthread=1, std::string arpafile="", std::string wordlist=""):
		_nthread(nthread), _arpafile(arpafile), _wordlist(wordlist)
		{
		   //LOG_ASSERT(nthread == 1);	
		}

	// anasy Arpa file for thread
	// know every threads read start position
	// return state approximate number.
	FsaStateId AnasyArpa();
	
	// convert one line to arpaline
	bool AnalyLine(char *line, ArpaLine *arpaline,int gram);

	// add one line grammer to fsa.
	bool AddLineToFsa(ArpaLine *arpaline,
			FsaStateId &prev_gram_stateid, int gram);

	bool NgramToFsa(int gram,int nt);

	bool ConvertArpa2Fsa();
private:
	// Arpa line save in this struct.
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
			if(other.words.size() != words.size())
					return false;
			for(size_t i=0;i<words.size()-1;++i)
			{
				if(words[i] != other.words[i])
					return false;
			}	
			return true;
		}
	};

	// Print one line arpa.
	void PrintArpaLine(ArpaLine &arpaline)
	{
		std::cout << arpaline.logprob << " ";
		for(size_t i=0;i<arpaline.words.size();++i)
			std::cout << _map_syms[arpaline.words[i]] << " ";
		std::cout << arpaline.backoff_logprob << " " << std::endl;
	}
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
	struct Offset
	{
		FsaStateId line;//read how many line, it's end;
		long long int offset;//file offset,it's start
		Offset():line(0), offset(0) { }
	};

	enum {NON,DATA,GRAM};
	
	std::vector<std::vector<Offset> > _file_offset;
};

#include "src/util/namespace-end.h"
#endif

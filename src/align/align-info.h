#ifndef __HB_ALIGN_INFO_H__
#define __HB_ALIGN_INFO_H__

#include <unordered_map>
#include <string>
#include <vector>
#include <set>
#include <iostream>
using namespace std;
using std::unordered_map;

#include "src/util/util-common.h"

#include "src/util/namespace-start.h"

template<typename T=int>
class StringToInt
{
public:
	bool Read(std::string &file, bool binary=false)
	{
		bool ret = ReadWordMap(file, _str_to_val, binary);
		if(ret != true)
		{
			LOG_WARN << "Read " << file << " failed!!!";
			return false;
		}
		return true;
	}

	T Find(const std::string &key)
	{
		auto find = _str_to_val.find(key);
		if(find == _str_to_val.end())
		{
			LOG_WARN << "No this key (" << key << ")";
			return -1;
		}
		return find->second;
	}
private:
	std::unordered_map<std::string, T> _str_to_val;
};

template<typename T>
bool PrefixVector(std::vector<T> &v1, std::vector<T> &v2)
{
	size_t v1_size = v1.size();
	for(size_t i = 0 ; i < v1_size; ++i)
	{
		if(i >= v2.size())
			return true;
		if(v1[i] == v2[i])
			continue;
		else
			return false;
	}
	return true;
}

template<typename T>
void PrintVector(const std::vector<T> &v)
{
	for(auto iter = v.begin(); iter != v.end(); ++iter)
		std::cout << *iter << " ";
	std::cout << std::endl;
}

class WordToToken
{
public:

	WordToToken() {}

	~WordToToken() {}
	// load wordfile and tokenfile
	bool ReadWordAndTokenText(std::string &wordfile, std::string &tokenfile, bool binary=false)
	{
		bool ret = _words_map_id.Read(wordfile, binary);
		if(ret != true)
		{
			LOG_WARN << "Read wordfile failed!!!";
			return false;
		}
		ret = _tokens_map_id.Read(tokenfile, binary);
		if(ret != true)
		{
			LOG_WARN << "Read tokenfile failed!!!";
			return false;
		}
		return true;
	}

	// load lexicon , format:
	//   word phone1 ... phonen
	//   ...
	bool ReadLexicon(std::string &lexiconfile, bool check = false,  bool binary=false)
	{
		std::ifstream ifs;
		ifs.open(lexiconfile.c_str(),
				binary ? std::ios_base::in | std::ios_base::binary
				: std::ios_base::in);
		if(!ifs.is_open())
		{
			return false;
		}
		std::string line;
		std::vector<std::string> token_list;
		typedef unordered_map<std::string, int>::iterator IterType;
		while(getline(ifs, line))
		{
			token_list.clear();
			CutLineToStr(line, token_list);
			if(token_list.size() < 2)
			{
				LOG_WARN << "pronounce dict format error!!!";
				return false;
			}
			std::string word = token_list.front();
			token_list.erase(token_list.begin());
			auto find = _words_map_tokens.find(word);
#ifdef DEBUG
			PrintVector(token_list);
#endif
			if(find ==  _words_map_tokens.end())
			{ // not find
				std::vector<std::vector<std::string> > tokens_list;
				tokens_list.push_back(std::move(token_list));
				_words_map_tokens.insert(std::make_pair(word, std::move(tokens_list)));
#ifdef DEBUG
				std::cout << " size is "  << _words_map_tokens.size() << std::endl;
#endif
			}
			else
			{
				// check same prefix
				if(check)
				{
					for(std::vector<std::vector<std::string> >::iterator token_iter = find->second.begin();
							token_iter != find->second.end(); ++token_iter)
					{
						LOG_ASSERT(!PrefixVector<std::string>(*token_iter, token_list) || "same prefix tokens!");
					}
				}
				find->second.push_back(std::move(token_list));
			}
		}
		return true;
	}
	
	// convert string to int.
	void ConvertStrToInt(bool number = false)
	{
		typedef unordered_map<std::string, int>::iterator IterType;
		for(auto iter = _words_map_tokens.begin(); iter != _words_map_tokens.end(); ++iter)
		{
			int wordid = -1;
			// convert word to wordid
			if(number == true)
			{
				wordid = std::atoi(iter->first.c_str());
			}
			else
			{
				wordid = FindWordId(iter->first);
				if(wordid < 0)
				{
					continue;
				}
			}
			// convert phone to phoneid
			
			std::vector<std::vector<int> > tokens_list;
			std::vector<int> token_list;
			for(std::vector<std::vector<std::string> >::iterator tokenslistiter = iter->second.begin(); 
					tokenslistiter != iter->second.end();
					++tokenslistiter)
			{
				for(std::vector<std::string>::iterator tokenlistiter = tokenslistiter->begin();
						tokenlistiter != tokenslistiter->end();
						++tokenlistiter)
				{

					if(number == true)
						token_list.push_back(std::atoi(tokenlistiter->c_str()));
					else
					{
						int tokenid = FindTokenId(*tokenlistiter);
						if(tokenid < 0)
						{
							continue;
						}
						token_list.push_back(tokenid);
					}
				}
				tokens_list.push_back(std::move(token_list));
			}
			_wordsid_map_tokensid.insert(std::make_pair(wordid, std::move(tokens_list)));
		} // for
	}//
	
	int FindWordId(const std::string &word)
	{
		int wordid = _words_map_id.Find(word);
		if(wordid == -1)
		{
			LOG_WARN << "No this word in word map id (" << word << ")";
			return -1;
		}
		return wordid;
	}

	int FindTokenId(const std::string &token)
	{
		int tokenid = _tokens_map_id.Find(token);
		if(tokenid == -1)
		{
			LOG_WARN << "No this token in tokens map id (" << token << ")";
			return -1;
		}
		return tokenid;
	}

	const std::vector<std::vector<int> > *FindWordMapTokens(int wordid)
	{
		auto ret = _wordsid_map_tokensid.find(wordid);
		if(ret == _wordsid_map_tokensid.end())
		{
			LOG_WARN << "No this word in lexicon (" << wordid << ")";
			return NULL;
		}
		return &ret->second;
	}

	const std::vector<std::vector<int> > * FindWordMapTokens(std::string &words)
	{
		int wordid = FindWordId(words);
		if(wordid < 0)
		{
			LOG_WARN << "No word in _words_map_id (" << words << ")";
			return NULL;
		}
		return FindWordMapTokens(wordid);
	}

	const std::unordered_map<int, std::vector<std::vector<int> > > &GetWordToToken() const
	{
		return _wordsid_map_tokensid;
	}

	void PrintWordMapToken()
	{
		LOG_COM << "WordMapToken size " << _words_map_tokens.size();
		for(auto iter = _words_map_tokens.begin(); iter != _words_map_tokens.end(); ++iter)
		{
			const std::string &word = iter->first;
			for(std::vector<std::vector<std::string> >::iterator tokenslistiter = iter->second.begin(); 
					tokenslistiter != iter->second.end();
					++tokenslistiter)
			{
				std::string token_str;
				for(std::vector<std::string>::iterator tokenlistiter = tokenslistiter->begin();
						tokenlistiter != tokenslistiter->end();
						++tokenlistiter)
				{
					token_str += *tokenlistiter + " ";
				}
				LOG_COM << word << " " << token_str;
			}
		}
	}
private:
	StringToInt<int> _words_map_id;
	StringToInt<int> _tokens_map_id;
	
	// because polyphone, so use std::vector<std::vector<std::string> > save token(phone) list.
	// lexicon don't have same prefix.
	// for example:
	//    error:
	//    word p1 p2
	//    word p1 p2 p3 p4
	//    right:
	//    word p1 p2 p3
	//    word p1 p2 p5
	std::unordered_map<std::string, std::vector<std::vector<std::string> > > _words_map_tokens;
	std::unordered_map<int, std::vector<std::vector<int> > > _wordsid_map_tokensid;
}; // class WordToToken


class TokenToWordTree;

/////////////////////////////////////////////////
class TokenToWordTree
{
public:
	typedef int TokenInt;
	typedef int WordInt;

	TokenToWordTree(TokenInt loop_token=-1):_loop_token(loop_token) {}

	~TokenToWordTree()
	{
		for(auto iter = _arcs.begin(); iter != _arcs.end(); ++iter)
		{
			TokenToWordTree *tmp = iter->second;
			delete tmp;
		}
		_arcs.clear();
	}

	bool IsFinal() const
	{
		return _arcs.size() == 0;
	}

	bool IsMiddleFinal() const
	{
		return _words_info.size() != 0;
	}

	bool LoopToken(TokenInt tokenid) const
	{
		return tokenid == _loop_token;
	}
	//////
	TokenToWordTree* FindToken(TokenInt tokenid) const
	{
		auto find = _arcs.find(tokenid);
		if(find == _arcs.end())
			return NULL;
		return find->second;
	}

	bool FindWord(WordInt wordid) const
	{
		auto find = _words_info.find(wordid);
		return find != _words_info.end();
	}

   	TokenToWordTree *InsertToken(TokenInt tokenid)
	{
		TokenToWordTree *arc = FindToken(tokenid);
		if(arc == NULL)
		{ // no find insert unordered_map
			TokenToWordTree *new_arc = new TokenToWordTree(tokenid);
			_arcs.insert(std::make_pair(tokenid, new_arc));
			if(FindToken(tokenid) == NULL)
			{
				LOG_ERR << "InsertToken Error, it should not be happen!!!" ;
			}
			return new_arc;
		}
		else
			return arc;
	}

	bool InsertWord(WordInt wordid)
	{
		auto result = _words_info.insert(wordid);
		if(*result.first != wordid)
		{
			LOG_WARN << "insert wordid error (" << wordid << ")";
			return false;
		}
		if(result.second != true)
		{
			LOG_WARN << "Have the same lexicon line!!!";
		}
		return true;
	}

	bool InsertTokenAndWord(TokenInt tokenid, WordInt wordid)
	{
		// first find token
		TokenToWordTree *arc = InsertToken(tokenid);
		return arc->InsertWord(wordid);
	}

	void Print(int n=1)
	{
		//std::cout << _arcs.size() << " " << _words_info.size() << " " 
		//	<< _loop_token << std::endl;
		for(auto iter = _arcs.begin(); iter != _arcs.end(); iter++)
		{
			std::cout << "[" << n << "]" << iter->first << " " ;
			iter->second->Print(n+1);
		}
		if(_words_info.size() > 0)
			std::cout << " wrod ( " ;
		for(auto word_iter = _words_info.begin(); 
				word_iter != _words_info.end(); ++word_iter)
		{
			std::cout << *word_iter << " ";
		}
		if(_words_info.size() > 0)
		{
			std::cout << " ) ";
			std::cout << std::endl;
		}
	}
public:
	std::unordered_map<TokenInt, TokenToWordTree*> _arcs;
	std::set<WordInt> _words_info;
	TokenInt _loop_token;

};

//
class TokenToWordTreeInstance
{
public:
	typedef TokenToWordTree::TokenInt TokenInt;
	typedef TokenToWordTree::WordInt WordInt;

	TokenToWordTreeInstance(const WordToToken &word_to_token)
   	{
		CreateTree(word_to_token.GetWordToToken());
	}
	TokenToWordTreeInstance() { }

	virtual ~TokenToWordTreeInstance() {}

	// create token prefix tree.
	void CreateTree(const std::unordered_map<int, std::vector<std::vector<int> > > &wordtotoken)
	{
		//const std::unordered_map<int, std::vector<std::vector<int> > > &wordtotoken = _word_to_token.GetWordToToken();
		// traversal lexicon
		for(auto iter = wordtotoken.begin(); iter != wordtotoken.end(); ++iter)
		{
			WordInt wordid = iter->first;
			//for(std::vector<TokenInt>::const_iterator token_iter = iter->second.begin(); 
			for(auto token_iter = iter->second.begin(); 
					token_iter != iter->second.end(); 
					++token_iter)
			{
				if(AddWord(wordid, *token_iter) != true)
				{
					LOG_WARN << "Wordid add error!!! (" << wordid << ")";
				}
				//_tree.Print();
			}
		}
	}

	// add word to tree.
	bool AddWord(WordInt wordid, const std::vector<TokenInt> &token_list)
	{
		//PrintVector(token_list);
		TokenToWordTree *cur_node = &_tree;
		for(auto iter = token_list.begin(); iter != token_list.end(); ++iter)
		{
			TokenToWordTree *next_node = cur_node->InsertToken(*iter);
			//next_node->Print();
			cur_node = next_node;
		}
		return cur_node->InsertWord(wordid);
	}

	// get start node.
	const TokenToWordTree *Start() const
	{
		return &_tree;
	}

	// search current node loop,node not change
	// input:
	//   tokenid : token index
	//   node    : search node
	// return :
	// search return true,else return false
	bool SearchTokenLoop(TokenInt tokenid, const TokenToWordTree *node) const
	{
		return node->LoopToken(tokenid);
	}

	// search arc.
	// input :
	//   tokenid : token index
	//   node    : search node
	// return :
	//   search return next node,else return NULL
	TokenToWordTree* SearchTokenNext(TokenInt tokenid,
		   	const TokenToWordTree *node) const
	{
		return node->FindToken(tokenid);
	}
	
	bool IsFinal(const TokenToWordTree *node) const
	{
		return node->IsFinal();
	}

	bool IsMiddleFinal(const TokenToWordTree *node) const
	{
		return node->IsMiddleFinal();
	}

	bool SearchWord(WordInt wordid,
			const TokenToWordTree *node) const
	{
		return node->FindWord(wordid);
	}

	void Print()
	{
		_tree.Print();
	}
private:

	TokenToWordTree _tree;
};

// calculate align info.
class AlignInfo
{
public:
	typedef TokenToWordTree::TokenInt TokenInt;
	typedef TokenToWordTree::WordInt WordInt;

	AlignInfo(const TokenToWordTreeInstance &token_to_word_tree,
			float frames_pre_second=100):
		_frames_pre_second(frames_pre_second),
		_token_to_word_tree(token_to_word_tree) {}
	
	std::vector<std::pair<TokenInt, int> > ConvertToken(
			std::vector<TokenInt> &token_list, TokenInt ignore_token = -1)
	{
		size_t time = 0;
		std::vector<std::pair<TokenInt, int> > token_time;
		for(auto iter = token_list.begin(); iter != token_list.end(); ++iter)
		{
			if(*iter == ignore_token)
			{
				time++;
				continue;
			}
			else
			{
				token_time.push_back(std::make_pair(*iter, time));
				time++;
			}
		}
		return std::move(token_time);
	}
	
	struct AlignToken
	{
		TokenInt _tokenid;
		int _word_offset;
		int _time;
		TokenToWordTree *_arc;
		//AlignToken *_prev;
		int _pre_offset;

		AlignToken(TokenInt tokenid, int word_offset, 
				int time, TokenToWordTree *arc, int pre_offset):// AlignToken *prev):
			_tokenid(tokenid), _word_offset(word_offset), 
			_time(time), _arc(arc), _pre_offset(pre_offset) { }


		void Print(int i = 0)
		{
			std::cout << "AlignToken " << i <<  "\t: " << _tokenid << "\t" << _word_offset << "\t" 
				<< _time << "\t" << _pre_offset << std::endl;
		}
	};
	
	std::vector<std::pair<float, float> > GetAlignInfo(std::vector<WordInt> &word_list, 
		std::vector<TokenInt> &token_list, float offset = 0.0,
	   	TokenInt ignore_token = -1, bool invert=true, float frame_shift=1.0)
	{
		std::vector<std::pair<int, int> > frame_align = GetAlignInfo(word_list, token_list, ignore_token, invert);
		return std::move(ConvertFrameToSecond(frame_align, offset, frame_shift));
	}
	// input:
	//   word_list: word1 word2 ... wordn
	//   token_list: token1 token2 ... tokenn
	//   ignore_token: blank(or not -1)
	// return:
	//   {[start1,end1], [start2,end2]..., [startn, endn]}
	std::vector<std::pair<int, int> > GetAlignInfo(std::vector<WordInt> &word_list, 
		std::vector<TokenInt> &token_list, TokenInt ignore_token = -1, bool invert=true)
	{
		internal::PrintContainer(std::cout, word_list);
		internal::PrintContainer(std::cout,token_list);
		_toks.clear();
		size_t token_start=0, token_end = 0; // record current token start index and end index.
		int time = -1;
		size_t numword = word_list.size();
		bool last_word = false;

		for(auto token_iter = token_list.begin(); 
				token_iter != token_list.end() ; ++token_iter)
		{
			time++;
			if(*token_iter == ignore_token)
				continue;

			last_word = false;

			TokenInt tokenid = *token_iter;
			TokenInt prefix_tokenid = -1;
			if(token_iter != token_list.begin())
			{
				prefix_tokenid = *(token_iter-1);
			}
			bool need_loop = prefix_tokenid == tokenid;
			if(_toks.size() == 0)
			{ // init
				const TokenToWordTree *arc = _token_to_word_tree.Start();
				//AlignToken token(tokenid, 0, time, arc, NULL);
				token_start = 0;
				token_end = 1;
				_toks.push_back(AlignToken(-1, 0, time, (TokenToWordTree*)arc, -1));
			}
			std::set<std::pair<int, TokenToWordTree *> > have_word_and_time;

			for(size_t i = token_start; i<token_end;++i)
			{
				int prev_token_offset = i;
				AlignToken *token = &_toks[i];
				//int prev_time = token->_time;
				//TokenInt prev_tokenid = token->_tokenid;
				int cur_word_offset = token->_word_offset;
				TokenToWordTree *cur_arc = token->_arc;

				if(token->_word_offset >= word_list.size())
					continue;
				WordInt wordid = word_list[token->_word_offset];
				// first search loop
				bool ret = false;
				bool add_start = false;
				if(need_loop)
				{
					ret = _token_to_word_tree.SearchTokenLoop(tokenid, cur_arc);
					if(ret == true)
					{ // add token to _toks
						std::pair<int, TokenToWordTree *> cur_pair = {cur_word_offset, cur_arc};
						if(have_word_and_time.find(cur_pair) == have_word_and_time.end())
						{
							_toks.push_back(AlignToken(tokenid, cur_word_offset,
										time, cur_arc, prev_token_offset));
							have_word_and_time.insert(std::move(cur_pair));
						}
						// find word 
						ret = _token_to_word_tree.SearchWord(wordid, cur_arc);
						if(ret == true && add_start == false)
						{
							//for end flag so add token.
							cur_pair = {cur_word_offset+1, (TokenToWordTree*)(_token_to_word_tree.Start())};
							if(have_word_and_time.find(cur_pair) == have_word_and_time.end())
							{
								_toks.push_back(AlignToken(tokenid, cur_word_offset+1,	
											time, (TokenToWordTree*)(_token_to_word_tree.Start()), prev_token_offset));
								have_word_and_time.insert(std::move(cur_pair));
							}
							last_word = true;
							add_start = true;
						}
					}
				}
				// second search next
				TokenToWordTree *next_arc = _token_to_word_tree.SearchTokenNext(tokenid, cur_arc);
				if(next_arc != NULL)
				{// find arc
					std::pair<int, TokenToWordTree*> cur_pair = {cur_word_offset, next_arc};
					if(have_word_and_time.find(cur_pair) == have_word_and_time.end())
					{
						_toks.push_back(AlignToken(tokenid, cur_word_offset,
									time, next_arc, prev_token_offset));
						have_word_and_time.insert(std::move(cur_pair));
					}
					// find word 
					// last judge current prossble end.
					ret = _token_to_word_tree.SearchWord(wordid, next_arc);
					if(ret == true && add_start == false)
					{
						cur_pair = {cur_word_offset+1, (TokenToWordTree*)(_token_to_word_tree.Start())};
						if(have_word_and_time.find(cur_pair) == have_word_and_time.end())
						{
							_toks.push_back(AlignToken(tokenid, cur_word_offset+1,	
										time, (TokenToWordTree*)(_token_to_word_tree.Start()), prev_token_offset));
							have_word_and_time.insert(std::move(cur_pair));
						}
						last_word = true;
						add_start = true;
					}
				} // if(next_arc != NULL)
			}
#ifdef DEBUG
			std::cout << "start: " << token_start << " end: " << token_end << std::endl;
#endif
			token_start = token_end;
			token_end = _toks.size();
		} // token_list
#ifdef DEBUG
		Print();
#endif
		// check
		if(last_word == false)
		{ // not align
			LOG_WARN << "Not align!!!";
			std::vector<std::pair<int, int> > noali;
			return std::move(noali);
		}
		return std::move(GetAlign(numword, invert));
	}

	std::vector<std::pair<int, int> > GetAlign(size_t numword, bool invert=true)
	{
		int best_ali_id = -1;
		//AlignToken *best_ali = NULL;
		if(invert == true)
		{
			for(int i = _toks.size()-1; i>=0 ; --i)
			{
				if(_toks[i]._word_offset == numword)
				{
					best_ali_id = i;
					break;
				}
			}
		}
		else
		{
			for(int i = 0; i< _toks.size(); ++i)
			{
				if(_toks[i]._word_offset == numword)
				{
					best_ali_id = i;
					break;
				}
			}
		}
		// {[0, 0],[0,1], ..., [numword-1, timen-1], [numword, timen]}
		std::vector<std::pair<int,int> > word_time;
		while(best_ali_id >= 0)
		{
			AlignToken &best_ali = _toks[best_ali_id];
#ifdef DEBUG
			std::cout << best_ali._word_offset << " " << best_ali._tokenid << " " 
				<< best_ali._time << std::endl;
#endif
			word_time.push_back(std::make_pair(best_ali._word_offset, 
						best_ali._time));
			best_ali_id = best_ali._pre_offset;
		}

		// word time is reverse
		std::vector<std::pair<int, int> >word_ali;
		int start = -1, end = -1;
		int pre_word= -1;
		for(int i = word_time.size()-1; i > 0 ;--i)
		{
			int wordoffset = word_time[i].first;
			//int time = word_time[i].second;
			int nextwordoffset = word_time[i-1].first;
			int next_time = word_time[i-1].second;
			if(pre_word != wordoffset)
			{
				start = next_time;
				pre_word = wordoffset;
			}
			if(wordoffset != nextwordoffset)
			{
				end = next_time;
				word_ali.push_back(std::make_pair(start, end));
			}
		}
		return std::move(word_ali);
	}
	void Print()
	{
		int i=0;
		for(auto iter = _toks.begin(); iter != _toks.end(); ++iter)
		{
			iter->Print(i);
			i++;
		}
	}

	std::vector<std::pair<float, float> > ConvertFrameToSecond(
			const std::vector<std::pair<int, int> > &align_frame, 
			float offset=0.0, float frame_shift=1.0)
	{
		std::vector<std::pair<float, float> > align_time; 
		for(ssize_t i=0;i<align_frame.size();++i)
		{
			float start_time = align_frame[i].first * frame_shift / _frames_pre_second + offset;
			float end_time = align_frame[i].second * frame_shift / _frames_pre_second + offset;
			align_time.push_back(std::make_pair(start_time, end_time));
		}
		return std::move(align_time);
	}

private:
	float _frames_pre_second;
	std::vector<AlignToken> _toks;

	const TokenToWordTreeInstance &_token_to_word_tree;
};
#include "src/util/namespace-end.h"
#endif

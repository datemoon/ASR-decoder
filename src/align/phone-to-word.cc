#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "src/util/text-util.h"
#include "src/align/phone-to-word.h"

#include "src/util/namespace-start.h"

int PinyinNode::AddWordToNode(int wordid)
{
	if(wordid != -1 )
	{
		// realloc wordid
		if( _wordnum == _totwordnum)
		{
			_totwordnum+=2;
			int *tmp = new int[_totwordnum];
			if(_wordid != NULL)
			{
				memcpy(tmp, _wordid , sizeof(int)*_wordnum);
				delete []_wordid;
			}
			_wordid = tmp;
		}
		_wordid[_wordnum] = wordid;
		_wordnum++;
	}
	return 0;
}
/*
 * return node id and tonode+1,
 * */
int PinyinNode::AddPhoneToNode(int phoneid,int &tonode, int &totarc)
{
	int node = OrderSearchPhoneInNode(phoneid);
	if(-1 != node)
	{
		return node;
	}
	// realloc arcs
	if(_arcnum == _totarcnum)
	{
		_totarcnum+=5;
		PinyinArc *tmp = new PinyinArc[_totarcnum];
		if(_arcs != NULL)
		{
			memcpy(tmp, _arcs , sizeof(PinyinArc)*_arcnum);
			delete []_arcs;
		}
		_arcs = tmp;
	}
	_arcs[_arcnum]._phoneid = phoneid;
	_arcs[_arcnum]._tonode = tonode;
	_arcnum++;
	tonode++;
	totarc++;

	return (tonode-1);
}

int PinyinNode::OrderSearchPhoneInNode(int phoneid)
{
	for(int i=0;i<_arcnum;++i)
	{
		if(_arcs[i]._phoneid == phoneid)
			return _arcs[i]._tonode;
	}
	return -1;
}

bool PinyinNode::OrderSearchWordInNode(const int wordid)
{
	for(int i=0;i<_wordnum;++i)
	{
		if(wordid == _wordid[i])
			return true;
	}
	return false;
}

void PinyinNode::PrintNode(int n)
{
	for(int i=0;i<_wordnum;++i)
	{
		std::cout << n << "\t" << _wordid[i] << std::endl;
	}
	for(int i=0;i < _arcnum;++i)
	{
		std::cout << n << "\t" << _arcs[i]._phoneid << " "
			<< _arcs[i]._tonode << std::endl;
	}
}

void PinyinTree::PrintTree()
{
	for(int i=0;i<_realnum;++i)
	{
		_node[i].PrintNode(i);
	}
}

int PinyinTree::AddWord(int *pinyin,int n,int wordid)
{
	int node = 0; // start is 0
	for(int i=0;i<n;++i)
	{
		// there need realloc _node,
		// if the memory is not enough.
		if(_realnum == _totnum)
		{
			// it's necessary realloc _node
			_totnum += 1e4;
			PinyinNode *tmp = new PinyinNode[_totnum];
			if(NULL != _node)
			{
				for(int i = 0;i<_realnum;++i)
					tmp[i].Copy(_node[i]);
//				memcpy(tmp,_node,sizeof(PinyinNode) * _realnum);
				delete []_node;
			}
			else
				_realnum += 1;
			_node = tmp;
		}
		node = _node[node].AddPhoneToNode(pinyin[i],_realnum,_totarc);
		if(i == n-1)
		{
			_node[node].AddWordToNode(wordid);
		}
	}
	return 0;
}

int PinyinTree::Init()
{
	_totnum = 1e5;
	_node = new PinyinNode[_totnum];
	_realnum = 0;
	return 0;
}

int PinyinTree::SearchWord(const int *pinyin, const int n,const int wordid)
{
	int node = _startnode;
	for(int i=0;i<n;++i)
	{
		node = _node[node].OrderSearchPhoneInNode(pinyin[i]);
		if(node == -1)
		{
			std::cerr << "search this wordid failed " << wordid << std::endl;
			return -1;
		}
		bool ret = _node[node].OrderSearchWordInNode(wordid);
		if(ret == true)
			return i+1;
	}
	return -1;
}


int PhoneToWord::ReadText(const char *worddict,const char *phonedict)
{
	std::ifstream fin(worddict);
	std::string line;
	std::vector<std::string> str;
	while(getline(fin, line))
	{
		CutLineToStr(line,str);
		if(_worddict.find(str[0]) != _worddict.end())
		{
			std::cerr << str[0] << "have this word in worddict" << std::endl; 
			continue;
		}
		if(2 != str.size() )
		{
			std::cerr << str[0] << "phonedict format error." << std::endl;
			continue;
		}
		_worddict[str[0]] = atoi(str[1].c_str());
	}
	fin.clear();
	fin.close();

	std::ifstream pfin(phonedict);
	while(getline(pfin, line))
	{
		CutLineToStr(line,str);
		if(_phonedict.find(str[0]) != _phonedict.end())
		{
			std::cerr << str[0] << "have this phone in phonedict" << std::endl;
			continue;
		}
		if(2 != str.size() )
		{
			std::cerr << str[0] << "phonedict format error." << std::endl;
			continue;
		}
		_phonedict[str[0]] = atoi(str[1].c_str());
	}
	pfin.clear();
	pfin.close();

	return 0;
}

int PhoneToWord::ProcessPronDict(const char *prondict)
{
	std::ifstream fin(prondict);
	std::string line;
	std::vector<std::string> str;
#define PINYINLEN 30
	typedef unordered_map<std::string, int>::iterator IterType;
	int pinyins[PINYINLEN];
	int pinyin_num = 0;
	while(getline(fin, line))
	{
		pinyin_num = 0;
		CutLineToStr(line,str);
		if(str.size() < 2)
		{
			std::cerr << str[0] << "pronounce dict format error." << std::endl;
			continue;
		}
		IterType worditer = _worddict.find(str[0]);
		if(worditer == _worddict.end())
		{
			std::cerr << str[0] << "word dict no this word." << std::endl;
			continue;
		}
		int wordid = worditer->second;
		IterType phoneiter ;
		for(size_t i=1;i<str.size();++i)
		{
			phoneiter = _phonedict.find(str[i]);
			if(phoneiter == _phonedict.end())
			{
				std::cerr << str[i] << "phone dict no this phone." << std::endl;
				exit(-1);
			}
			pinyins[i-1] = phoneiter->second;
			pinyin_num++;
		}
		if(0 != _phonetoword->AddWord(pinyins, pinyin_num, wordid))
		{
			std::cerr << line << " add failed." << std::endl;
		}
		assert((size_t)pinyin_num == str.size()-1);
#ifdef HBDEBUG
		std::cout << wordid ;
		for(size_t i=1;i<str.size();++i)
		{
			std::cout << " " << _phonedict[str[i]];
		}
		std::cout << std::endl;
#endif
	}

	fin.clear();
	fin.close();
	return 0;
}

#include "src/util/namespace-end.h"


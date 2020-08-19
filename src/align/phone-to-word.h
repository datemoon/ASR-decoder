#ifndef __PHONE_TO_WORD_H__
#define __PHONE_TO_WORD_H__

#include <unordered_map>
#include <string>
#include <vector>
using namespace std;
using std::unordered_map;

#include "src/util/namespace-start.h"

struct PinyinArc
{
	int _phoneid;
	int _tonode;
	PinyinArc():_phoneid(-1),_tonode(-1) { }
	~PinyinArc() { }
};

struct PinyinNode
{
	PinyinArc *_arcs;
	int *_wordid;
	int _arcnum;
	int _totarcnum;
	int _wordnum;
	int _totwordnum;
	PinyinNode()
		:_arcs(NULL), _wordid(NULL), _arcnum(0), _totarcnum(0),
		_wordnum(0), _totwordnum(0) { }
	PinyinNode(PinyinNode &pinyin)
		:_arcs(pinyin._arcs) , _wordid(pinyin._wordid),
	   	_arcnum(pinyin._arcnum), _totarcnum(pinyin._totarcnum), 
		_wordnum(pinyin._wordnum), _totwordnum(pinyin._totwordnum) { }
	void Copy(PinyinNode &pinyin)
	{
		_arcs = pinyin._arcs ;
	   	_wordid = pinyin._wordid;
		_arcnum = pinyin._arcnum;
		_totarcnum = pinyin._totarcnum;
		_wordnum = pinyin._wordnum;
	   	_totwordnum = pinyin._totwordnum;
	}
	~PinyinNode()
	{
//		if(NULL != _arcs)
//			delete []_arcs;
//		_arcs = NULL;
//		_arcnum=0;
//		if(NULL != _wordid)
//			delete []_wordid;
//		_wordid=NULL;
//		_wordnum=0;
	}
	void Destory()
	{
		if(NULL != _arcs)
			delete []_arcs;
		_arcs = NULL;
		_arcnum=0;
		if(NULL != _wordid)
			delete []_wordid;
		_wordid=NULL;
		_wordnum=0;
	}

	void PrintNode(int n);
	/*
	 * search node use order way.if have phoneid return tonodeid,
	 * else return -1.
	 * */
	int OrderSearchPhoneInNode(int phoneid);

	/*
	 * search node use order way,if have wordid return true,else false
	 * */
	bool OrderSearchWordInNode(const int wordid);

	/*
	 * add phone to this node and return tonodeid,tonode is total node number.
	 * */
	int AddPhoneToNode(int phoneid, int &tonode, int &totarc);

	/*
	 * if add the last phone , add word to the final node.
	 * */
	int AddWordToNode(int wordid);
};

struct PinyinTree
{
	PinyinNode *_node;
	int _startnode;
	int _realnum;
	int _totnum;
	int _totarc;
	PinyinTree():_node(NULL), _startnode(0), _realnum(0), _totnum(0), _totarc(0) { }
	~PinyinTree()
	{
		for(int i=0;i<_realnum;++i)
			_node[i].Destory();
		if(NULL != _node)
			delete []_node;
	}

	int Init();
	
	/*
	 * add word to this tree.
	 * */
	int AddWord(int *pinyin,int n,int wordid);

	/*
	 * input pinyin sequence and if find wordid ,return use pinyin number
	 * else return -1.
	 * */
	int SearchWord(const int *pinyin,const int n,const int wordid);

	void PrintTree();
};

class PhoneToWord
{
public:
	PhoneToWord() 
	{
	   _phonetoword = new PinyinTree;	
	}

	~PhoneToWord() 
	{
	   delete _phonetoword;
	}

	int ReadText(const char *worddict,const char *phonedict);
	
	int ProcessPronDict(const char *prondict);

	/**/
	int SearchWord(const int *pinyin, const int n,const int wordid)
	{
		return _phonetoword->SearchWord(pinyin,n,wordid);
	}

	void PrintStruct()
	{
		_phonetoword->PrintTree();
	}
private:
	std::unordered_map<std::string, int> _worddict;
	std::unordered_map<std::string, int> _phonedict;
	
	PinyinTree *_phonetoword;

};

#include "src/util/namespace-end.h"

#endif

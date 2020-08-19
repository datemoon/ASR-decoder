#ifndef __GRAPH_H__
#define __GRAPH_H__ 1


#include <vector>
#include <string>

using namespace std;
using std::vector;

#include "src/util/namespace-start.h"
#define NODE_ID int
#define WEIGHT   float


template <class T1,class T2>
struct Arc
{
	T1 _input;
	T2 _output;
	int _to;
	float _w;

	Arc():_to(0),_w(1.0){}
	Arc(const T1 &input,const T2 &output,int to,float w=1.0):
		_input(input),_output(output),_to(to),_w(w){}
	
	Arc(const Arc &A):_input(A._input),_output(A._output),_to(A._to),_w(A._w){}
	~Arc() {}
	bool operator<(const Arc &A)
	{
		return _input<A._input;
	}
	bool operator<=(const Arc &A)
	{
		return _input<=A._input;
	}
	bool operator==(const Arc &A)
	{
		if(_input == A._input && _output == A._output &&_to == A._to )//&& _w == A._w)
		{
			assert(_w - A._w + 1e-3 > 0);
			return true;
		}
		else
			return false;
	}

	Arc& operator=(const Arc &A)
	{
		_input = A._input;
		_output = A._output;
		_to = A._to;
		_w = A._w;
		return *this;
	}
};
//default node 0 is start.
template <class T1,class T2>
class Node
{
public:
	Node():_final(0){}
	Node(int final):_final(final){}
	Node(const Node &A):_final(A._final),_arc(A._arc) {}
	~Node(){}

	void SetNodeFinal(){ _final = 1;}
	void AddArc(Arc<T1,T2> const &arc)
	{
		//int size = _arc.size();
		//_arc.resize(size+1);
		//_arc[size+1] = arc;
		_arc.push_back(arc);
	}

	bool HaveSameArc(Arc<T1,T2> const &arc)
	{
		for(unsigned i=0;i<_arc.size() ; ++i)
		{
			if(_arc[i] == arc)
				return true;
		}
		return false;
	}
	Node& operator=(const Node &A)
	{
		_final = A._final;
		_arc = A._arc;
		return *this;
	}

	bool IsFinal() 
	{
		if(_final == 1)
			return true;
		else
			return false;
	}
	Arc<T1,T2> *GetArc(unsigned int arcid)
	{
		if(arcid < _arc.size())
			return &_arc[arcid];
		else
			return NULL;
	}
	unsigned int GetArcSize() { return _arc.size();};

	/*
	 * Here use quick sort.
	 * */
	void SortArc(int start,int end)
	{
		if(start >= end)
			return ;
		int mid = start;
		int tail = end;
		int head = start;
		Arc<T1,T2> arc = _arc[mid];
		while(head < tail)
		{
			while(head < tail && arc <= _arc[tail] )
			{
				tail--;
			}
			if(head < tail)
			{
				_arc[head] = _arc[tail];
				head++;
			}
			while(head < tail && _arc[head] < arc)
			{
				head++;
			}
			if(head < tail)
			{
				_arc[tail] = _arc[head];
				tail--;
			}
		}
		_arc[head] = arc;
		SortArc(start,head-1);
		SortArc(head+1,end);
	}
private:
	int _final;
	vector<Arc<T1,T2> > _arc;
};

template <class T1,class T2>
class Net
{
public:
	Net(){};
	~Net(){};
	int AddNode(int final = 0)
	{
		Node<T1,T2> node(final);
		_node.push_back(node);
		return _node.size()-1;
	}
	bool SetNodeFinal(int nodeid) 
	{
		if(nodeid >= _node.size())
			return false;
		_node[nodeid]._final = 1;
		return true;
	}
	Node<T1,T2>* GetNode(unsigned int nodeid)
	{
		if(nodeid < _node.size())
			return &_node[nodeid];
		else
			return NULL;
	}
	unsigned int GetNetSize() { return _node.size();}
	void Clear() { _node.clear();}
private:
	vector<Node<T1,T2> > _node;
};

struct ArcInput
{
	int _word;
	int _net;
	ArcInput():_word(0),_net(0){}
	ArcInput(int word,int net):_word(word),_net(net){}
	
	ArcInput(const ArcInput& A):_word(A._word),_net( A._net){}

	~ArcInput(){}
	ArcInput& operator=(const ArcInput &A)
	{
		_word = A._word;
		_net = A._net;
		return *this;
	}
	bool operator==(const ArcInput &A)
	{
		if(_word == A._word && _net == A._net)
			return true;
		else
			return false;
	}
	bool operator<(const ArcInput &A)
	{
		return _word < A._word;
	}
	bool operator<=(const ArcInput &A)
	{
		return _word <= A._word;
	}
	bool IsSpace()
	{
		return (_word == 0) ? true : false;
	}
};
typedef Arc<ArcInput,int> ParseArc;
typedef Node<ArcInput,int> ParseNode;
typedef Net<ArcInput,int> ParseNet;
//typedef Net<string,string> ParseStringNet;
#include "src/util/namespace-end.h"
#endif

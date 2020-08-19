#ifndef __DECODE_GRAPH_H__
#define __DECODE_GRAPH_H__


#include "src/decoder/graph.h"

#include "src/util/namespace-start.h"
typedef Arc<int,int> StdArc;
typedef Node<int,int> StdNode;
typedef int StateId;

class DecodeNet
{
public:
	DecodeNet(){};
	~DecodeNet(){};
	const StdNode *GetNode(unsigned stateid)
	{
		if(stateid < _state.size())
			return &_state[stateid];
		else
			return NULL;
	}
	unsigned int GetNetSize() { return _state.size();}
	int Start() { return _start_state;}
private:
	vector<StdNode> _state;
	int _start_state; // default 0
	int _tot_state;
	int _tot_arc;
	int _tot_in_eps;
	int _tot_out_eps;
};
#include "src/util/namespace-end.h"

#endif

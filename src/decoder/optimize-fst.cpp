#include <stdio.h>
#include <string.h>
#include "src/decoder/optimize-fst.h"
#include "src/decoder/log.h"

#include "src/util/namespace-start.h"

bool Fst::ReadFst(const char *file)
{
	FILE *fp = fopen(file,"rb");
	if(NULL == fp)
	{
		LOGERR("fopen %s failed.\n",file);
		return false;
	}
	ReadFst(fp);
	fclose(fp);
	return true;
}
bool Fst::ReadFst(FILE *fp)
{
	if(1 != fread(&(_start_stateid),sizeof(int),1,fp))
		return false;
	if(1 != fread(&(_final_stateid),sizeof(int),1,fp))
		return false;
	if(1 != fread(&(_total_states),sizeof(int),1,fp))
		return false;
	if(1 != fread(&(_total_arcs),sizeof(int),1,fp))
		return false;
	if(1 != fread(&(_total_niepsilons),sizeof(int),1,fp))
		return false;
	if(1 != fread(&(_total_noepsilons),sizeof(int),1,fp))
		return false;

	// malloc memery
	_state_arr = new State[_total_states];
	_arc_arr = new Arc[_total_arcs];
	int i = 0;
	int arc_offset = 0;
	for(i = 0 ; i < _total_states ; ++i)
	{
		unsigned int num_arc = 0;
		if(1 != fread(&num_arc,sizeof(int),1,fp))
			return false;
		_state_arr[i]._num_arcs = num_arc;
		_state_arr[i]._arcs = &_arc_arr[arc_offset];
		if(num_arc > 0)
		{
			if(num_arc != fread(_state_arr[i]._arcs,sizeof(Arc),num_arc,fp))
			{
				LOGERR("fread arc error!\n");
				return false;
			}
		}
		arc_offset += num_arc;
	}
	assert(arc_offset == _total_arcs && "arc read is error.\n");
	return true;
}

void Fst::PrintFst()
{
	for(int i =0 ; i < _total_states ; ++i)
	{
		Arc* arc = _state_arr[i]._arcs;
		for(int j = 0; j < _state_arr[i]._num_arcs ; ++j)
		{
			printf("%d\t%d\t%d\t%d\t%f\n", i, arc[j]._to, arc[j]._input, arc[j]._output, arc[j]._w);
		}
	}
}

void Fst::RmOlalel()
{
	for(int i =0 ; i < _total_states ; ++i)
	{
		Arc* arc = _state_arr[i]._arcs;
		for(int j = 0; j < _state_arr[i]._num_arcs ; ++j)
		{
			arc[j]._output=0;
		}
	}
}
#include "src/util/namespace-end.h"

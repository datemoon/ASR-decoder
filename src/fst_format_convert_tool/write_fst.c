
#include <stdio.h>
#include <assert.h>
#include "fst_io.h"

int write_fst(const char *filename,struct Fst *fst)
{
	FILE *fp = fopen(filename,"a");
	if(1 != fwrite(&(fst->start_state),sizeof(int),1,fp))
	{
		return -1;
	}
	if(1 != fwrite(&(fst->final_state),sizeof(int),1,fp));
	if(1 != fwrite(&(fst->total_state),sizeof(int),1,fp));
	if(1 != fwrite(&(fst->total_arc),sizeof(int),1,fp));
	if(1 != fwrite(&(fst->total_niepsilons),sizeof(int),1,fp));
	if(1 != fwrite(&(fst->total_noepsilons),sizeof(int),1,fp));

	int i=0;
	int arc_offset = 0;
	for(i = 0 ; i < fst->total_state ; ++i)
	{
		struct State *state = &fst->state_arr[i];
		if(1 != fwrite(&state->num_arc,sizeof(int),1,fp))
		{
			return -1;
		}
		arc_offset += state->num_arc;
		int j = 0;
		if(state->num_arc > 0)
		{
			if(state->num_arc !=fwrite(state->arc_arr,sizeof(struct Arc),state->num_arc,fp))
			{
				return -1;
			}
			int n_arc= 0;
#ifdef DEBUG
			for(n_arc = 0;n_arc < state->num_arc;++n_arc)
			{
				fprintf(stdout,"%d %d %d %d %f\n",i,state->arc_arr[n_arc].nextstate,state->arc_arr[n_arc].ilabel,state->arc_arr[n_arc].olabel,state->arc_arr[n_arc].weight);
			}
#endif
		}

	}
	assert(arc_offset == fst->total_arc);
	fclose(fp);
	return 0;
}

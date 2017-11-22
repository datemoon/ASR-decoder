#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
//#include "fst.h"
#include "fst_io.h"

static const int32 kFstMagicNumber=2125659606;


int read_fst(const char *filename , struct FstHead *fsthead , struct Fst *fst)
{
	int32 magicnumber=0;
	FILE *fp = fopen(filename,"r");
	if(fp == NULL)
	{
		fprintf(stderr,"fopen %s fail !\n",filename);
		return -1;
	}
	if(1 != fread(&magicnumber,sizeof(int32),1,fp))
	{
		fprintf(stderr,"read fst head  fail!\n");
		return -1;
	}
	if(magicnumber != kFstMagicNumber)
	{
		fprintf(stderr,"FstHeader::Read: Bad FST header!\n");
		return -1;
	}
	memset(fsthead,0x00,sizeof(struct FstHead));
	int32 len=0;
	if(1 != fread(&len,sizeof(int32),1,fp))
	{
		fprintf(stderr,"read fst head  fail!\n");
		return -1;
	}
	if(len != fread(fsthead->fsttype_,sizeof(char),len,fp))
		return -1;
	if(1 != fread(&len,sizeof(int32),1,fp))
	{
		fprintf(stderr,"read fst head  fail!\n");
		return -1;
	}
	if(len != fread(fsthead->arctype_,sizeof(char),len,fp))
	{
		fprintf(stderr,"read fst head  fail!\n");
		return -1;
	}
	if(1 != fread(&fsthead->version_,sizeof(int32),1,fp))
	{
		fprintf(stderr,"read fst head  fail!\n");
		return -1;
	}
	if(1 != fread(&fsthead->flags_,sizeof(int32),1,fp))
	{
		fprintf(stderr,"read fst head  fail!\n");
		return -1;
	}
	if(1 != fread(&fsthead->properties_,sizeof(uint64),1,fp))
	{
		fprintf(stderr,"read fst head  fail!\n");
		return -1;
	}
	if(1 != fread(&fsthead->start_,sizeof(int64),1,fp))
	{
		fprintf(stderr,"read fst head  fail!\n");
		return -1;
	}
	if(1 != fread(&fsthead->numstates_,sizeof(int64),1,fp))
	{
		fprintf(stderr,"read fst head  fail!\n");
		return -1;
	}
	if(1 != fread(&fsthead->numarcs_,sizeof(int64),1,fp))
	{
		fprintf(stderr,"read fst head  fail!\n");
		return -1;
	}
	// add one final state.
	struct State *fst_read = (struct State *)malloc(sizeof(struct State ) * (fsthead->numstates_+1));
	if(fst_read == NULL)
	{
		fprintf(stderr,"malloc fst_read fail!\n");
		return -1;
	}
	memset(fst_read,0x00,sizeof(struct State ) * (fsthead->numstates_+1));
//	struct Arc *arc_read = (struct Arc *)malloc(sizeof(struct Arc ) * fsthead->numarcs_);
//	if(arc_read == NULL)
//	{
//		return -1;
//	}
//	memset(arc_read,0x00,sizeof(struct Arc ) * fsthead->numarcs_);
	int total_num_arcs=0,total_niepsilons = 0, total_noepsilons = 0;
	int state = 0;
	int final_state = fsthead->numstates_;
	fst_read[final_state].num_arc = 0;
	int final_nums = 0;
	for(state=0;state < fsthead->numstates_ ; ++state)
	{
		if(1 != fread(&fst_read[state].final,sizeof(float),1,fp))
		{
			fprintf(stderr,"read %d state final fail!\n",state);
			return -1;
		}
		if(1 != fread(&fst_read[state].num_arc,sizeof(int64),1,fp))
		{
			fprintf(stderr,"read %d state num_arc fail!\n",state);
			return -1;
		}
		if(100.0/0 != fst_read[state].final)
		{
			// add arc
			fst_read[state].num_arc++;
			final_nums++;
			fst_read[state].noepsilons++;
			fst_read[state].niepsilons++;
		}
	//	fst_read[state].arc_arr = &arc_read[total_num_arcs];
		fst_read[state].arc_arr = (struct Arc *)malloc(sizeof(struct Arc) * fst_read[state].num_arc);
		if(fst_read[state].arc_arr == NULL)
		{
			fprintf(stderr,"%s %d line  malloc fail %d!\n",__FILE__,__LINE__,sizeof(struct Arc) *fst_read[state].num_arc);
			return -1;
		}
		memset(fst_read[state].arc_arr,0x00,sizeof(struct Arc) * fst_read[state].num_arc);
		int n_arc = 0;
		if(100.0/0 != fst_read[state].final)
		{
			// add arc
			fst_read[state].arc_arr[n_arc].ilabel = 0;
			fst_read[state].arc_arr[n_arc].olabel = 0;
			fst_read[state].arc_arr[n_arc].weight = fst_read[state].final;
			fst_read[state].arc_arr[n_arc].nextstate = final_state;
			n_arc = 1;
		}
		
		for( ; n_arc < fst_read[state].num_arc ; ++n_arc)
		{

			if(1 != fread(&fst_read[state].arc_arr[n_arc].ilabel,sizeof(int),1,fp))
			{
				fprintf(stderr,"read %d state %d arc ilabel fail!\n",state,n_arc);
				return -1;
			}
			if(1 != fread(&fst_read[state].arc_arr[n_arc].olabel,sizeof(int),1,fp))
			{
				fprintf(stderr,"read %d state %d arc olabel fail!\n",state,n_arc);
				return -1;
			}
			if(1 != fread(&fst_read[state].arc_arr[n_arc].weight,sizeof(float),1,fp))
			{
				fprintf(stderr,"read %d state %d arc weight fail!\n",state,n_arc);
				return -1;
			}
			if(1 != fread(&fst_read[state].arc_arr[n_arc].nextstate,sizeof(int),1,fp))
			{
				fprintf(stderr,"read %d state %d arc newtstate fail!\n",state,n_arc);
				return -1;
			}
			if(fst_read[state].arc_arr[n_arc].ilabel == 0)
				fst_read[state].niepsilons++;
			if(fst_read[state].arc_arr[n_arc].olabel == 0)
				fst_read[state].noepsilons++;
			//if(fst_read[state].arc_arr[n_arc].ilabel == 0 && fst_read[state].arc_arr[n_arc].olabel == 0)
			{
				fprintf(stdout,"%d %d %d %d %f\n",state,fst_read[state].arc_arr[n_arc].nextstate,fst_read[state].arc_arr[n_arc].ilabel,fst_read[state].arc_arr[n_arc].olabel,fst_read[state].arc_arr[n_arc].weight);
			}
		}
		if(100.0/0 != fst_read[state].final)
			fprintf(stdout,"%d %f\n",state,fst_read[state].final);
		total_num_arcs += fst_read[state].num_arc;
		total_niepsilons +=fst_read[state].niepsilons;
		total_noepsilons += fst_read[state].noepsilons;
	}
	fclose(fp);
	fst->state_arr = fst_read;
	
	fst->total_state = state + 1;

//	assert(fsthead->numarcs_ == (total_num_arcs - final_nums) && "arc is not right.\n");
	fst->total_arc = total_num_arcs ;
	fst->start_state = fsthead->start_;
	fst->final_state = final_state;
	fst->total_niepsilons = total_niepsilons;
	fst->total_noepsilons = total_noepsilons;
	return 0;
}

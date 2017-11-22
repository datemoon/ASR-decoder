#ifndef __FST__
#define __FST__
typedef int int32;
typedef long long int64;
typedef unsigned long long uint64;

struct Arc
{
	int ilabel;
	int olabel;
	int nextstate;
	float weight;
};

struct State
{
	float final;
	struct Arc *arc_arr;
	int num_arc;
	int niepsilons;
	int noepsilons;
};

struct Fst
{
	struct State *state_arr;
	int start_state;
	int final_state;
	int total_state;
	int total_arc;
	int total_niepsilons;
	int total_noepsilons;
};

#endif

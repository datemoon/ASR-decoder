#include <stdio.h>
#include "decoder/optimize-fst.h"

int main(int argc,char *argv[])
{
	if(argc != 2)
	{
		fprintf(stderr,"%s fst\n",argv[0]);
		return -1;
	}
	Fst fst;
	if(true != fst.ReadFst(argv[1]))
	{
		fprintf(stderr,"ReadFst %s error.\n",argv[1]);
		return -1;
	}
	fst.PrintFst();
	return 0;
}


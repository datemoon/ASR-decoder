#include <stdio.h>
#include <string.h>
#include "fst_io.h"

int main(int argc,char *argv[])
{
	if(argc != 3)
	{
		fprintf(stderr,"input error!\nplease input %s fst out\n",argv[0]);
		return -1;
	}
	struct FstHead fsthead;
	memset(&fsthead,0x00,sizeof(fsthead));
	struct Fst fst;
	memset(&fst,0x00,sizeof(struct Fst));
	if(0 != read_fst(argv[1],&fsthead,&fst))
	{
		fprintf(stderr,"read_fst error!\n");
		return -1;
	}
	if(0 != write_fst(argv[2],&fst))
	{
		fprintf(stderr,"write_fst error!\n");
	 	return -1;
	}
	return 0;
}

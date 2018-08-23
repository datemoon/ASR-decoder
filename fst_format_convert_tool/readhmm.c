#include <stdio.h>
#include <string.h>
#include "fst_io.h"

int main(int argc,char *argv[])
{
	if(argc != 3)
	{
		fprintf(stderr,"input error!\nplease input %s fstlist out\n",argv[0]);
		return -1;
	}
	struct FstHead fsthead;
	memset(&fsthead,0x00,sizeof(fsthead));
	struct Fst fst;
	char line[128];
	FILE *fp=fopen(argv[1],"r");
	if( fp == NULL)
	{
		fprintf(stderr,"open %s failed!\n",argv[1]);
		return -1;
	}
	FILE *fp_out=fopen(argv[2],"w");
	if(fp_out == NULL)
	{
		fprintf(stderr,"open %s failed!\n",argv[1]);
		return -1;
	}
	int numhmm=0;
	fwrite(&numhmm,sizeof(int),1,fp_out);
	fclose(fp_out);

	while(fgets(line,sizeof(line),fp)!= NULL)
	{
		line[strlen(line)-1] = '\0';
		memset(&fst,0x00,sizeof(struct Fst));
		if(0 != read_fst(line,&fsthead,&fst))
		{
			fprintf(stderr,"read_fst error!\n");
			return -1;
		}
		if(0 != write_fst(argv[2],&fst))
		{
			fprintf(stderr,"write_fst error!\n");
			return -1;
		}
		numhmm++;
	}
	fp_out=fopen(argv[2],"r+");
	fseek(fp_out,0,SEEK_SET);
	printf("%ld\n",ftell(fp_out));
	fwrite(&numhmm,sizeof(int),1,fp_out);
	printf("%ld\n",ftell(fp_out));
	fclose(fp_out);
	fclose(fp);
	printf("load %d hmm\n",numhmm);
	return 0;
}

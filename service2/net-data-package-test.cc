#include "service2/net-data-package.h"

#define LEN 1024

int main(int argc, char*argv[])
{
	char *infile = argv[1];
	char *outfile = argv[2];
	C2SPackageAnalysis cli,ser;
	
	FILE* infp = fopen(infile, "r");
	int fd = open(outfile, O_RDWR);
	char cache[LEN];
	int n=0;
	while(true)
	{
		memset(cache, 0x00, sizeof(cache));
		size_t ret = fread((void*)cache, sizeof(char), LEN, infp);
		if(ret == 0)
		{
			cli.C2SWrite(fd, cache, ret, n, 1);
			ser.C2SRead(fd);
			cli.Print();
			ser.Print()
			break;
		}
		n++;
		cli.C2SWrite(fd, cache, ret, n, 0);
		ser.C2SRead(fd);
		cli.Print();
		ser.Print()
	}
	return 0;
}

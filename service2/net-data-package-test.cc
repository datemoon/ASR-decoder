
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "service2/net-data-package.h"

#define LEN 1024

bool C2SPackageAnalysisTest(char *infile, char *outfile)
{
	C2SPackageAnalysis cli,ser;

	const char *tmpfile = "tmp.io";
	FILE* infp = fopen(infile, "r");
	if(infp == NULL)
	{
		std::cout << "fopen " << infile << " failed."  << std::endl;
		return false;
	}
	int fd = open(tmpfile, O_CREAT|O_WRONLY|O_TRUNC, S_IRWXG|S_IRWXU|S_IRWXU);
	if(fd < 0)
	{
		std::cout << "open " << tmpfile << " failed." << std::endl;
		return false;
	}
	char cache[LEN];
	int n=0;
	while(true)
	{
		memset(cache, 0x00, sizeof(cache));
		size_t ret = fread((void*)cache, sizeof(char), LEN, infp);
		n++;
		if(ret == 0)
		{
			if(cli.C2SWrite(fd, cache, ret, n, 1) != true)
			{
				std::cout << "C2SWrite failed." << std::endl;
				return -1;
			}
			cli.Print("cli");
			break;
		}
		if(cli.C2SWrite(fd, cache, ret, n, 0) != true)
		{
			std::cout << "C2SWrite failed." << std::endl;
			return -1;
		}
		cli.Print("cli");
	}
	close(fd);
	fd = open(tmpfile, O_RDONLY);
	if(fd < 0)
	{
		std::cout << "open " << tmpfile << " failed." << std::endl;
		return false;
	}
	FILE* outfp = fopen(outfile, "w");
	if(outfp == NULL)
	{
		std::cout << "fopen " << outfile << " failed."  << std::endl;
		return false;
	}
	while(true)
	{
		if( true != ser.C2SRead(fd))
		{
			std::cerr << "C2SRead failed." << std::endl;
			return -1;
		}
		ser.Print("ser");
		uint data_len =0;
		char *data = ser.GetData(&data_len);
		std::cout << "data_len : " << data_len << std::endl;
		size_t ret = fwrite(data,sizeof(char), data_len, outfp);
		if(ret != data_len)
		{
			std::cout << "write data error." << std::endl;
		}
		if(ser.IsEnd())
		{
			break;
		}
	}
	close(fd);
	fclose(outfp);
	fclose(infp);
	return true;
}

int main(int argc, char*argv[])
{
	if (argc != 3)
	{
		std::cout << argv[0] << " infile tmpfile" << std::endl;
		return -1;
	}
	char *infile = argv[1];
	char *outfile = argv[2];
	C2SPackageAnalysisTest(infile, outfile);
	return 0;
}

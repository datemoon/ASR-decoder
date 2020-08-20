
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "src/service2/net-data-package.h"

#define LEN 1024
#define TEST_BEST 10
#define PCM_LEN 10000
#ifdef NAMESPACE
using namespace datemoon;
#endif
bool C2SPackageAnalysisTest(const char *infile, const char *outfile)
{
	C2SPackageAnalysis cli,ser;

	const char *tmpfile = "c2stmp.io";
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
			if(cli.C2SWrite(fd, cache, ret, 1) != true)
			{
				std::cout << "C2SWrite failed." << std::endl;
				return false;
			}
			cli.Print("cli");
			cli.Reset();
			break;
		}
		if(cli.C2SWrite(fd, cache, ret, 0) != true)
		{
			std::cout << "C2SWrite failed." << std::endl;
			return false;
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
			return false;
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

bool S2CPackageAnalysisTest(const char *infile)
{
	S2CPackageAnalysis cli,ser;

	const char *tmpfile = "s2ctmp.io";
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
		char *ret = fgets(cache, LEN, infp);
		if(ret == NULL)
		{
			if(ser.S2CWrite(fd, S2CPackageAnalysis::S2CEND) != true)
			{
				std::cerr << "S2CWrite all end failed." << std::endl;
				return false;
			}
			break;
			ser.Print("ser");
			ser.Reset();
			ser.Print("reset-ser");
		}
		n++;
		if(n%3==0)
		{
			if(ser.S2CWrite(fd, S2CPackageAnalysis::S2CMIDDLEEND) != true)
			{
				std::cerr << "S2CWrite failed." << std::endl;
				return false;
			}
			ser.Print("ser");
			ser.Reset();
			ser.Print("reset-ser");
		}
		std::string res1(cache);
		ser.SetNbest(res1);
	}
	close(fd);
	fd = open(tmpfile, O_RDONLY);
	if(fd < 0)
	{
		std::cout << "open " << tmpfile << " failed." << std::endl;
		return false;
	}
	while(true)
	{
		if(true != cli.S2CRead(fd))
		{
			std::cerr << "S2CRead failed." << std::endl;
			return false;
		}
		cli.Print("cli");
		std::vector<std::string> nbest;
		cli.GetData(&nbest);
		std::cout << "nbest : " << nbest.size() << std::endl;
		for(size_t i=0; i < nbest.size(); ++i)
		{
			std::cout << nbest[i] << std::endl;
		}
		if(cli.IsAllEnd() == true)
			break;
	}
	close(fd);
	fclose(infp);
	return true;
}

bool CreatNbestFile(const char *nbest_file)
{
	FILE* infp = fopen(nbest_file, "w");
	if(infp == NULL)
	{
		std::cout << "fopen " << nbest_file << " failed."  << std::endl;
		return false;
	}
	for(int i =0;i<TEST_BEST;++i)
	{
		fprintf(infp,"%d best\n",i);
	}
	fclose(infp);
	return true;
}

bool CreatePcmFile(const char *infile)
{
	FILE* infp = fopen(infile, "w");
	if(infp == NULL)
	{
		std::cout << "fopen " << infile << " failed."  << std::endl;
		return false;
	}
	fprintf(infp,"pcm:");
	for(int i =0;i<PCM_LEN;++i)
	{
		fprintf(infp,"%d ",i);
	}
	fclose(infp);
	return true;
}
int main(int argc, char*argv[])
{
	const char *infile = "c2spcm.send";
	const char *outfile = "c2spcm.recv";
	if(CreatePcmFile(infile) != true)
	{
		std::cerr << "CreatePcmFile failed." << std::endl;
		return -1;
	}
	if(C2SPackageAnalysisTest(infile, outfile) != true)
	{
		std::cerr << "C2SPackageAnalysisTest error." << std::endl;
		return -1;
	}
	const char * nbest_file = "test.nbest";
	if(CreatNbestFile(nbest_file) != true)
	{
		std::cout << "CreatNbestFile failed." << std::endl;
		return -1;
	}
	if(S2CPackageAnalysisTest(nbest_file) != true)
	{
		std::cout << "S2CPackageAnalysisTest failed." << std::endl;
		return -1;
	}
	std::cout << "sizeof(C2SPackageHead)=" << sizeof(C2SPackageHead) << "\n"
		<< "sizeof(S2CPackageHead)=" << sizeof(S2CPackageHead) << std::endl;
	return 0;
}

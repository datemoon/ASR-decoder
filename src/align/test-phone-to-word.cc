#include <iostream>
#include <fstream>
#include <string.h>
#include <vector>
#include <stdlib.h>
#include <assert.h>
#include "src/align/phone-to-word.h"
#include "src/util/text-util.h"

#ifdef NAMESPACE
using namespace datemoon ;
#endif

int main(int argc,char *argv[])
{
	if(argc != 6)
	{
		std::cerr << argv[0] 
			<< " worddict phonedict prondict output test.list" << std::endl;
		exit(-1);
	}
	PhoneToWord pronounce;
	if( 0 != pronounce.ReadText(argv[1],argv[2]))
	{
		std::cerr << "ReadText" << argv[1] << argv[2] << 
			"failed." << std::endl;
		return -1;
	}
	if(0 != pronounce.ProcessPronDict(argv[3]))
	{
		std::cerr << "ProcessPronDict" << argv[3] << "failed." << std::endl;
		return -1;
	}
	std::ifstream fin(argv[5]);
	std::string line;
	std::vector<std::string> str;
	int wordid = 1;
   	int pinyins[30] ;
	int pinyin_num = 0;
	while(getline(fin, line))
	{
		pinyin_num = 0;
		CutLineToStr(line,str);
		wordid = atoi(str[0].c_str());
		for(size_t k = 1; k < str.size();++k)
		{
			pinyins[k-1] = atoi(str[k].c_str());
			pinyin_num++;
		}
		assert((size_t)pinyin_num == str.size()-1);
		int ret = pronounce.SearchWord(pinyins,pinyin_num,wordid);

		if(ret == -1)
		{
			std::cerr << line << "not find this word" << std::endl;
		}
		else
		{
			std::cerr << line << " find this word use " << ret << " phone." << std::endl;
		}
	}
	pronounce.PrintStruct();
	/*
	wordid = 7;
   	int pinyinseq[10] = {12, 12, 169,48,51,35};
	int ret = pronounce.SearchWord(pinyinseq,6,wordid);
	if(ret == 0)
	{
		std::cerr << "not find this word" << std::endl;
		return -1;
	}
	std::cerr << "find this word use " << ret << " phone." << std::endl;
	*/
	return 0;
}


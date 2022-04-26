#include <string.h>
#include <vector>
#include <stdlib.h>
#include <assert.h>
#include <functional>
#include "src/align/align-info.h"

#ifdef NAMESPACE
using namespace datemoon ;
#endif

template<typename T>
void PrintVector(std::vector<T> &v)
{
	for(auto iter = v.begin(); iter != v.end(); ++iter)
		std::cout << *iter << " ";
	std::cout << std::endl;
}
int main(int argc, char *argv[])
{
	bool binary = false;
	WordToToken word_to_token;
	std::string align_file, word_file;
	std::string lexiconfile;
	bool number = true;
	if(argc == 6)
	{
		std::string wordfile = argv[1];
		std::string tokenfile = argv[2];
		lexiconfile = argv[3];
		align_file = argv[4];
		word_file = argv[5];
		if(!word_to_token.ReadWordAndTokenText(wordfile, tokenfile))
		{
			LOG_ERR << "ReadWordAndTokenText failed!!!";
		}
		number = false;
	}
	else if(argc == 4)
	{
		lexiconfile = argv[1];
		align_file = argv[2];
		word_file = argv[3];
		number = true;
	}
	else
	{
		const char *usage = "Usage: test-align-info lexiconfile align_file word_file or\n"
			"test-align-info wordfile tokenfile lexiconfile align_file word_file";
		LOG_WARN << usage;
		return -1;
	}
	if(!word_to_token.ReadLexicon(lexiconfile))
	{
		LOG_ERR << "ReadLexicon failed!!!";
	}
	word_to_token.PrintWordMapToken();
	word_to_token.ConvertStrToInt(number);

	TokenToWordTreeInstance token_to_word(word_to_token);
	token_to_word.Print();

	AlignInfo ali_info(token_to_word);

	std::vector<std::vector<std::string> > ali_read, word_read;
	if(!ReadStringVector(align_file, ali_read, binary))
	{
		LOG_ERR << "read " << align_file << " failed!!!";
	}
	if(!ReadStringVector(word_file, word_read, binary))
	{
		LOG_ERR << "read " << word_file << " failed!!!";
	}
	
	LOG_ASSERT(ali_read.size() == word_read.size());

	//std::function<std::vector<int>() > 
	auto Convert=[](std::vector<std::string> &input, size_t offset)
	{
		std::vector<int> output;
		for(size_t i = offset ; i<input.size();++i)
		{
			output.push_back(std::atoi(input[i].c_str()));
		}
		return std::move(output);
	};
	for(size_t i = 0 ; i < ali_read.size(); ++i)
	{
		std::vector<int> word_list, token_list;
		word_list = Convert(word_read[i], 0);
		token_list = Convert(ali_read[i], 0);
		bool invert=true;
		std::vector<std::pair<int, int> > ali_res = 
			ali_info.GetAlignInfo(word_list, token_list, 0, invert);
		if(ali_res.size() != word_list.size())
		{
			LOG_WARN << "align error!!!";
		}
		auto PrintAli = [word_list](std::vector<std::pair<int, int> > &ali_res)
		{
			int i = 0;
			for(auto iter = ali_res.begin(); iter != ali_res.end(); ++iter)
			{
				std::cout << "word :" << word_list[i] << " ali:\t" << 
					iter->first << "\t" << iter->second << std::endl;
				i++;
			}
		};
		PrintVector(ali_read[i]);
		PrintVector(word_read[i]);
		PrintAli(ali_res);
		std::cout << "----------------------" << std::endl;
	}


	return 0;
}

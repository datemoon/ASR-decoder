#ifndef __WORDID_TO_WORDSTR_H__
#define __WORDID_TO_WORDSTR_H__

#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "src/util/namespace-start.h"
typedef int WordIdType;

class WordSymbol
{
private:
	WordIdType *_words; // word offset
	char *_mem;
	WordIdType _word_maxid;
	int _word_range;
	size_t _mem_offset;
	size_t _mem_len;
	size_t _mem_range;
public:
	WordSymbol()
	{
		_word_maxid = 1e4 * 5;
		_words = new WordIdType[_word_maxid];
		_mem_len = 1e4 * 5 * 8;//default word list is 5w 
		_mem = new char[_mem_len];
		_mem_offset = 0;
		_word_range = 1000;
		_mem_range = 1000;
	}

	~WordSymbol()
	{
		if(NULL != _words)
			delete []_words;
		if(NULL != _mem)
			delete []_mem;
	}
	int AddWord(char *word,WordIdType wordid)
	{
		if(wordid >= _word_maxid)
		{
			// realloc words
			WordIdType needlen = wordid + _word_range;
			WordIdType *tmp = new WordIdType[needlen];
			memcpy(tmp, _words,sizeof(WordIdType)*_word_maxid);
			delete []_words;
			_words = tmp;
			_word_maxid = needlen;
			if(_word_range < 8000)
				_word_range *= 2;
		}
		int wordlen = strlen(word);
		if(wordlen + 1 + _mem_offset > _mem_len)
		{
			// realloc memery
			size_t needlen = wordlen + 1 + _mem_offset + _mem_range;
			char *tmp = new char[needlen];
			memcpy(tmp, _mem, sizeof(char) * _mem_len);
			delete []_mem;
			_mem = tmp;
			_mem_len = needlen;
			if(_mem_range < 8000 * 5)
				_mem_range *= 2;
		}
		// add word to _mem

		_words[wordid] = _mem_offset;
		memcpy(_mem+_mem_offset, word, wordlen);
		_mem[_mem_offset+wordlen] = '\0';
		_mem_offset += wordlen+1;
		return wordlen;
	}

	int ReadText(const char *wordfile)
	{
		FILE *fp = fopen(wordfile, "r");
		if(NULL == fp)
		{
			fprintf(stderr,"fopen %s failed.\n",wordfile);
			return -1;
		}

		char line[256];
		memset(line,0x00,sizeof(line));
		int wordid = 0;
		char word[256];
		memset(word,0x00,sizeof(word));
		while(NULL != fgets(line,sizeof(line),fp))
		{
			line[strlen(line)-1] = '\0';
			if(2 != sscanf(line,"%s %d",word,&wordid))
			{
				fprintf(stderr," read line \"%s\" error\n",line);
				return -1;
			}
			int ret = AddWord(word, wordid);
			assert(ret != 0);
		}
		fclose(fp);
		return 0;
	}

	char *FindWordStr(int wordid)
	{
		if(wordid > _word_maxid)
		{
			fprintf(stderr,"it's error id %d, no word.\n",wordid);
			return NULL;
		}
		size_t offset = _words[wordid];
		return _mem + offset;
	}
};

#include "src/util/namespace-end.h"
#endif

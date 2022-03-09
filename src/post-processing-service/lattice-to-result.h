#ifndef __LATTICE_TO_RESULT_H__
#define __LATTICE_TO_RESULT_H__

#include "fstext/fstext-lib.h"

#include <sstream>
#include <algorithm>
#include <string>
#include <vector>

namespace kaldi
{

template<class Arc>
void FstToNbestVector(const fst::Fst<Arc> &fst,
		size_t n,
		std::vector<std::vector<int32> > *alignments,
		std::vector<std::vector<int32> > *words,
		std::vector<typename Arc::Weight> *weights = NULL)
{
	std::vector<fst::VectorFst<Arc> > fsts_out;
	fst::VectorFst<Arc> nbest_fst;
	fst::ShortestPath(fst, &nbest_fst, n);
	fst::ConvertNbestToVector(nbest_fst, &fsts_out);
	//fst::NbestAsFsts(fst, n, &fsts_out);
	for(size_t i = 0 ; i < fsts_out.size(); ++i)
	{
		std::vector<int32> alignment;
		std::vector<int32> word;
		typename Arc::Weight weight;
		GetLinearSymbolSequence(fsts_out[i],
				&alignment,
				&word,
				&weight);//, NULL);
		alignments->push_back(alignment);
		words->push_back(word);
		if(weights != NULL)
			weights->push_back(weight);
	}
}

template<class T>
std::string ConvertWordId2Str(const fst::SymbolTable &word_syms, 
		const std::vector<T> &words)
{
	std::ostringstream result;
	for(size_t i = 0 ; i < words.size() ; ++i)
	{
		std::string s = word_syms.Find(words[i]);
		if(s.empty())
		{
			KALDI_WARN << "wrod-id " << std::to_string(words[i])
			   	<< " not in symbol table!!!" 
				<< result.str() << "<#" << std::to_string(words[i]) << "> ";
		}
		else
			result << s << " ";
	}
	return result.str();
}

} // namespace kaldi

#endif

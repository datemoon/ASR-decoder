
#include "src/v1-asr/asr-source.h"

namespace kaldi {
std::string LatticeToString(const Lattice &lat, const fst::SymbolTable &word_syms) 
{
	LatticeWeight weight;
	std::vector<int32> alignment;
	std::vector<int32> words;
	GetLinearSymbolSequence(lat, &alignment, &words, &weight);

	std::ostringstream msg;
	for (size_t i = 0; i < words.size(); i++) 
	{
		std::string s = word_syms.Find(words[i]);
		if (s.empty()) 
		{
			KALDI_LOG << "Word-id " << words[i] << " not in symbol table.";
			msg << "<#" << std::to_string(i) << "> ";
		} else
			msg << s << " ";
	}
	KALDI_LOG << "Cost for utterance " <<  weight.Value1() << " + " << weight.Value2();
	return msg.str();
}
std::string GetTimeString(int32 t_beg, int32 t_end, BaseFloat time_unit) 
{
	char buffer[100];
	double t_beg2 = t_beg * time_unit;
	double t_end2 = t_end * time_unit;
	snprintf(buffer, 100, "%.2f %.2f", t_beg2, t_end2);
	return std::string(buffer);
}
int32 GetLatticeTimeSpan(const Lattice& lat) 
{
	std::vector<int32> times;
	LatticeStateTimes(lat, &times);
	return times.back();
}
std::string LatticeToString(const CompactLattice &clat, const fst::SymbolTable &word_syms) 
{
	if (clat.NumStates() == 0) {
		KALDI_LOG << "Empty lattice.";
		return "";
	}
	CompactLattice best_path_clat;
	CompactLatticeShortestPath(clat, &best_path_clat);

	Lattice best_path_lat;
	ConvertLattice(best_path_clat, &best_path_lat);
	return LatticeToString(best_path_lat, word_syms);
}
}

#ifndef __CONST_LM_RESCORE_H__
#define __CONST_LM_RESCORE_H__

#include "base/kaldi-common.h"
#include "fstext/fstext-lib.h"
#include "lat/kaldi-lattice.h"
#include "lat/lattice-functions.h"
#include "lm/arpa-file-parser.h"
#include "util/common-utils.h"
#include "lm/const-arpa-lm.h"

namespace kaldi 
{
struct LatticeRescoreOpt
{
	std::string word_syms_rxfilename,
		old_const_arpa_lm_rxfilename,
		new_const_arpa_lm_rxfilename;
	BaseFloat oldscale;
	BaseFloat newscale;
	bool determinize;
	LatticeRescoreOpt():
		oldscale(-1.0), newscale(1.0), determinize(false) { }

	void Register(ParseOptions *opts)
	{
		opts->Register("determinize", &determinize,
				"determinize rescore lattice");
		opts->Register("oldscale", &oldscale,
				"old arpa scale");
		opts->Register("newscale", &newscale,
				"new arpa scale");
		opts->Register("old-const-arpa-lm", &old_const_arpa_lm_rxfilename,
				"old const arpa lm filename");
		opts->Register("new-const-arpa-lm", &new_const_arpa_lm_rxfilename,
				"new const arpa lm filename");
		opts->Register("word-syms-filename", &word_syms_rxfilename,
				"word syms filename");
	}

	bool Check()
	{
		if(old_const_arpa_lm_rxfilename.empty())
		{
			KALDI_LOG << "old const arpa lm filename is NULL";
			return false;
		}
		if(new_const_arpa_lm_rxfilename.empty())
		{
			KALDI_LOG << "new const arpa lm filename is NULL";
			return false;
		}
		if(word_syms_rxfilename.empty())
		{
			KALDI_LOG << "word syms filename is NULL";
			return false;
		}
		return true;
	}
};

// This class warps a ConstArpaLm format language model with the
// interface defined in DeterministicOnDemandFst.
// This class it's same kaldi::ConstArpaLmDeterministicFst 
// except will add rescore parameter and change arc.weight scale.
class ConstArpaLmDeterministicFstScale:
	public fst::DeterministicOnDemandFst<fst::StdArc>
{
public:
	typedef fst::StdArc::Weight Weight;
	typedef fst::StdArc::StateId StateId;
	typedef fst::StdArc::Label Label;

	explicit ConstArpaLmDeterministicFstScale(const ConstArpaLm& lm, 
			BaseFloat scale = 1.0): lm_(lm), scale_(scale)
	{
		// Creates a history state for <s>.
		std::vector<Label> bos_state(1, lm_.BosSymbol());
		state_to_wseq_.push_back(bos_state);
		wseq_to_state_[bos_state] = 0;
		start_state_ = 0;
	}
	// We cannot use "const" because the pure virtual function in the interface is
	// not const.
	virtual StateId Start() { return start_state_; }

	// We cannot use "const" because the pure virtual function in the interface is
	// not const.
	virtual Weight Final(StateId s)
	{
		// At this point, we should have created the state.
		KALDI_ASSERT(static_cast<size_t>(s) < state_to_wseq_.size());
		const std::vector<Label>& wseq = state_to_wseq_[s];
		float logprob = lm_.GetNgramLogprob(lm_.EosSymbol(), wseq);
		logprob = logprob * scale_;
		return Weight(-logprob);
	}

	virtual bool GetArc(StateId s, Label ilabel, fst::StdArc* oarc)
	{
		// At this point, we should have created the state.
		KALDI_ASSERT(static_cast<size_t>(s) < state_to_wseq_.size());
		std::vector<Label> wseq = state_to_wseq_[s];

		float logprob = lm_.GetNgramLogprob(ilabel, wseq);
		if (logprob == -std::numeric_limits<float>::infinity()) 
		{
			return false;
		}
		logprob = logprob * scale_;

		// Locates the next state in ConstArpaLm. Note that OOV and backoff have been
		// taken care of in ConstArpaLm.
		wseq.push_back(ilabel);
		while (wseq.size() >= lm_.NgramOrder())
		{ // History state has at most lm_.NgramOrder() -1 words in the state.
			wseq.erase(wseq.begin(), wseq.begin() + 1);			
		}
		while (!lm_.HistoryStateExists(wseq)) 
		{
			KALDI_ASSERT(wseq.size() > 0);
			wseq.erase(wseq.begin(), wseq.begin() + 1);
		}
		std::pair<const std::vector<Label>, StateId> wseq_state_pair(
				wseq, static_cast<Label>(state_to_wseq_.size()));
		// Attemps to insert the current <wseq_state_pair>. If the pair already exists
		// then it returns false.
		typedef MapType::iterator IterType;
		std::pair<IterType, bool> result = wseq_to_state_.insert(wseq_state_pair);

		// If the pair was just inserted, then also add it to <state_to_wseq_>.
		if (result.second == true)
			state_to_wseq_.push_back(wseq);

		// Creates the arc.
		oarc->ilabel = ilabel;
		oarc->olabel = ilabel;
		oarc->nextstate = result.first->second;
		oarc->weight = Weight(-logprob);

		return true;
	}

private:
	typedef unordered_map<std::vector<Label>,
			StateId, VectorHasher<Label> > MapType;
	StateId start_state_;
	MapType wseq_to_state_;
	std::vector<std::vector<Label> > state_to_wseq_;
	const ConstArpaLm& lm_;
	BaseFloat scale_;
};

// This class is for compose lattice and const arpa lm,
// for biglm rescore use.
class KaldiConstArpaLmRescore
{
public:
	KaldiConstArpaLmRescore(const ConstArpaLm& lm, 
			BaseFloat scale=1.0, bool determinize=false):
		_const_arpa_fst_scale(lm, scale), _determinize(determinize) { }

	bool Compose(CompactLattice &clat, 
			CompactLattice* composed_clat)
	{
		fst::ArcSort(&clat, fst::OLabelCompare<CompactLatticeArc>());
		ComposeCompactLatticeDeterministic(clat, 
				&_const_arpa_fst_scale, composed_clat);
		// 如果确定化，就按olabel确定化
		if(_determinize)
		{ // Determinizes the composed lattice.
			Lattice composed_lat;
			ConvertLattice(*composed_clat, &composed_lat);
			Invert(&composed_lat); // make it so word labels are on the input.
			// determinize for word(olabel) and 
			// output CompactLattice will be ilable is trads_id, 
			// olabel is wordid.
			DeterminizeLattice(composed_lat, composed_clat); 
		}
		if(composed_clat->Start() == fst::kNoStateId)
		{
			KALDI_WARN << "Empty lattice "
				<< " (incompatible LM?)";
			return false;
		}
		return true;
	}

	bool Compose(Lattice &lat,
			Lattice* composed_lat)
	{
		CompactLattice clat;
		CompactLattice composed_clat;

		ConvertLattice(lat, &clat);
		
		if(Compose(clat, 
			&composed_clat) != true)
			return false;
		ConvertLattice(composed_clat, composed_lat);
		return true;
	}
private:
	ConstArpaLmDeterministicFstScale _const_arpa_fst_scale;
	bool _determinize;
};

} // namespace kaldi

#endif

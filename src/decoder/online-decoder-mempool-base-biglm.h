#ifndef __ONLINE_DECODER_MEMPOOL_BASE_BIGLM_H__
#define __ONLINE_DECODER_MEMPOOL_BASE_BIGLM_H__

#include "src/decoder/online-decoder-mempool-base.h"
#include "src/newlm/diff-lm.h"

#include "src/util/namespace-start.h"

// 这是一个动态的解码器，在解码过程中动态的修改语言模型权重，
// 用大的语言模型替换原来语言模型分数
template<typename FST, typename Token = StdToken>
class OnlineLatticeDecoderMempoolBaseBiglm:
	public OnlineLatticeDecoderMempoolBase<FST, Token>
{
public:
	typedef uint64 PairId;
	using ForwardLinkT = ForwardLink<Token>;
	typedef typename HashList<PairId, Token*>::Elem Elem;
	//typedef int Label;

	OnlineLatticeDecoderMempoolBaseBiglm(FST *fst,
			const LatticeFasterDecoderConfig &config, 
			ArpaLm *oldlm,
			ArpaLm *newlm):
		OnlineLatticeDecoderMempoolBase<FST, Token>(fst, config),
		_diff_lm(oldlm, newlm) 
	{
		this->_toks.SetSize(this->_config._max_active * this->_config._hash_ratio);
		LOG_COM << "--- Construct OnlineLatticeDecoderMempoolBaseBiglm class ---";
   	}

	virtual ~OnlineLatticeDecoderMempoolBaseBiglm() { }

	virtual void InitDecoding();

	virtual BaseFloat ProcessEmitting(AmInterface *decodable);

    virtual void ProcessNonemitting(BaseFloat cutoff);

	virtual Elem *FindOrAddToken(PairId stateid, int32 frame_plus_one, BaseFloat tot_cost,
			Token *backpointer, bool *changed);


	virtual void ComputeFinalCosts(
			unordered_map<Token*, BaseFloat> *final_costs,
			BaseFloat *final_relative_cost,
			BaseFloat *final_best_cost);

	virtual BaseFloat GetCutoff(Elem *list_head,
			size_t *tok_count, BaseFloat *adaptive_beam, Elem **best_elem);

	virtual void PruneForwardLinksFinal();

	StateId NextLmState(StateId lm_state, Label olabel, BaseFloat *lm_score)
	{
		if(olabel == 0)
		{
			*lm_score = 0;
			return lm_state;
		}
		LatticeWeight diff_weight ;
		StateId nextstateid;
		Label tmp1;
		if(false == this->_diff_lm.GetArc(lm_state, olabel, &nextstateid, &diff_weight, &tmp1))
		{
			LOG_WARN << "No word id " << olabel << " in difflm!!!";
		}
		*lm_score = diff_weight.Value1();
		return nextstateid;
	}
protected:

	HashList<PairId, Token*> _toks;
	std::vector<const Elem *> _queue;
	DiffArpaLm _diff_lm;

	inline PairId ConstructPair(StateId fst_state, StateId lm_state)
	{
		return static_cast<PairId>(fst_state) +(static_cast<PairId>( lm_state) << 32);
	}

	static inline StateId PairToState(PairId state_pair)
	{
		return static_cast<StateId>(static_cast<uint32>(state_pair));
	}

	static inline StateId PairToLmState(PairId state_pair)
	{
		return static_cast<StateId>(static_cast<uint32>(state_pair >> 32));
	}
};
template<typename FST, typename Token>
void  OnlineLatticeDecoderMempoolBaseBiglm<FST, Token>::InitDecoding()
{
	this->ClearActiveTokens();
	// clean up from last time:
	this->_toks.DeleteElems();
	this->_active_toks.clear();
	this->_queue.clear();
	this->_tmp_array.clear(); // for calculate cutoff score
	this->_cost_offsets.clear();
	this->_final_costs.clear();
	this->_warned = false;

	assert(this->_num_toks == 0);
	assert(this->_num_links == 0);

 	this->_decoding_finalized = false;
	// biglm
	this->_diff_lm.Reset();

	PairId start_pair = ConstructPair(this->_graph->Start(),this->_diff_lm.Start());

	this->_active_toks.resize(1);
	Token *start_tok = this->NewToken(0.0, 0.0, NULL, NULL, NULL);
	this->_active_toks[0]._toks = start_tok;
	this->_toks.Insert(start_pair, start_tok);
	ProcessNonemitting(this->_config._beam);
	this->_num_frames_decoded = 0;

}
template <typename FST, typename Token>
typename OnlineLatticeDecoderMempoolBaseBiglm<FST, Token>::Elem *OnlineLatticeDecoderMempoolBaseBiglm<FST, Token>::FindOrAddToken(
		PairId stateid, int32 frame_plus_one, BaseFloat tot_cost,
		Token *backpointer, bool *changed)
{
	LOG_ASSERT((size_t)frame_plus_one < this->_active_toks.size());
	Token *&toks = this->_active_toks[frame_plus_one]._toks;
	Elem *e_found = this->_toks.Insert(stateid, NULL);
	if(e_found->val == NULL)
	{
		const BaseFloat extra_cost = 0.0;
		Token *new_tok = this->NewToken (tot_cost, extra_cost, NULL, toks, backpointer);

		toks = new_tok;
		e_found->val = new_tok;
		if (changed != NULL)
			*changed = true;
		return e_found;
	}
	else
	{
		Token *tok = e_found->val;
		if(tok->_tot_cost > tot_cost)
		{
			tok->_tot_cost = tot_cost;
			tok->_backpointer = backpointer;
			if (changed != NULL)
				*changed = true;
		}
		else
		{
			if (changed != NULL)
				*changed = false;
		}
		return e_found;
	}

}
template <typename FST, typename Token>
void OnlineLatticeDecoderMempoolBaseBiglm<FST, Token>::ComputeFinalCosts(
		unordered_map<Token*, BaseFloat> *final_costs,
		BaseFloat *final_relative_cost,
		BaseFloat *final_best_cost) 
{
	LOG_ASSERT(!this->_decoding_finalized);
	if (final_costs != NULL)
		final_costs->clear();

	const Elem *final_toks = this->_toks.GetList();

	BaseFloat infinity = FLOAT_INF;
	BaseFloat best_cost = infinity;
	BaseFloat best_cost_with_final = infinity;

	while(final_toks != NULL)
	{
		PairId state_pair = final_toks->key;
		Token *tok = final_toks->val;
		StateId stateid = this->PairToState(state_pair), // state in "fst"
				lm_stateid = this->PairToLmState(state_pair); // in lm

		const Elem *next = final_toks->tail;
		bool fst_final = this->_graph->IsFinal(stateid); // here I don't use infinity inside of log(0).
		BaseFloat lm_final = this->_diff_lm.Final(lm_stateid);
		BaseFloat cost_with_final = tok->_tot_cost + lm_final;
		best_cost = std::min(tok->_tot_cost, best_cost);
		best_cost_with_final = std::min(cost_with_final, best_cost_with_final);
		if(final_costs != NULL && fst_final == true)
		{ // for all final state mark.
			(*final_costs)[tok] = lm_final; // because fst is superfinalfst.
		}
		final_toks = next;
	}
	if(final_relative_cost != NULL)
	{
		if (best_cost == infinity && best_cost_with_final == infinity)
			// Likely this will only happen if there are no tokens surviving.
			// This seems the least bad way to handle it.
			*final_relative_cost = infinity;
		else
			*final_relative_cost = best_cost_with_final - best_cost;
	}
	if (final_best_cost != NULL)
	{
		if (best_cost_with_final != infinity) 
		{ // final-state exists.
  			*final_best_cost = best_cost_with_final;
		} 
		else 
		{ // no final-state exists.
			*final_best_cost = best_cost;
		}
	}
}

template <typename FST, typename Token>
BaseFloat OnlineLatticeDecoderMempoolBaseBiglm<FST, Token>::GetCutoff(Elem *list_head, 
		size_t *tok_count, BaseFloat *adaptive_beam, Elem **best_elem)
{
	//size_t tok_count = 0;
	BaseFloat best_weight = FLOAT_INF;

	size_t count = 0;
	if(this->_config._max_active == std::numeric_limits<int>::max() &&
			this->_config._min_active == 0)
	{
		for(Elem *e = list_head; e!= NULL; e = e->tail, ++count)
		{
			BaseFloat w = e->val->_tot_cost;
			if(w < best_weight)
			{
				best_weight = w;
				if(best_elem != NULL)
					*best_elem = e;
			}
		}
		if(tok_count != NULL)
			*tok_count = count;
		if(adaptive_beam != NULL)
			*adaptive_beam = this->_config._beam;
		return best_weight + this->_config._beam;
	}
	else
	{ // record the best socre , and add score to _tmp_array for cut off.
		this->_tmp_array.clear();
		//for(auto it = _cur_toks.begin(); it != _cur_toks.end();++it,++tok_count)
		for(Elem *e=list_head; e!= NULL; e = e->tail, ++count)
		{
			BaseFloat w = e->val->_tot_cost;
			this->_tmp_array.push_back(w);
			if(w < best_weight)
			{
				best_weight = w;
				if(best_elem != NULL)
					*best_elem = e;
			}
		}
		if (tok_count != NULL) *tok_count = count;

		BaseFloat beam_cutoff = best_weight + this->_config._beam;
		BaseFloat min_active_cutoff = FLOAT_INF;
		BaseFloat max_active_cutoff = FLOAT_INF;

		VLOG_COM(3) << "Number of tokens active on frame " << this->NumFramesDecoded() 
			<< "is" << this->_tmp_array.size();
		if(this->_tmp_array.size() > static_cast<size_t>(this->_config._max_active))
		{
			std::nth_element(this->_tmp_array.begin(),
					this->_tmp_array.begin() + this->_config._max_active,
					this->_tmp_array.end());
			max_active_cutoff = this->_tmp_array[this->_config._max_active];
//			return max_active_cutoff;
		}

		if(max_active_cutoff < beam_cutoff)
		{ // max_active is tighter than beam.
			// turn down beam.
			if(adaptive_beam != NULL)
				*adaptive_beam = max_active_cutoff - best_weight + this->_config._beam_delta;
			return max_active_cutoff;
		}

		if (this->_tmp_array.size() > static_cast<size_t>(this->_config._min_active))
		{
			if (this->_config._min_active == 0) 
				min_active_cutoff = best_weight;
			else
			{
				std::nth_element(this->_tmp_array.begin(),
						this->_tmp_array.begin() + this->_config._min_active,
						this->_tmp_array.size() > static_cast<size_t>(this->_config._max_active) ?
						this->_tmp_array.begin() + this->_config._max_active :
						this->_tmp_array.end());
				min_active_cutoff = this->_tmp_array[this->_config._min_active];
			}
		}

		if(min_active_cutoff > beam_cutoff)
		{ // min_active is looser than beam.
			// turn up beam.
			if (adaptive_beam != NULL)
				*adaptive_beam = min_active_cutoff - best_weight + this->_config._beam_delta;
			return min_active_cutoff;
		}
		else
		{
			if (adaptive_beam != NULL)
				*adaptive_beam = this->_config._beam;
			return beam_cutoff;
		}
	}// end if
}

template <typename FST, typename Token>
BaseFloat OnlineLatticeDecoderMempoolBaseBiglm<FST, Token>::ProcessEmitting(AmInterface *decodable)
{
	LOG_ASSERT(this->_active_toks.size() > 0 && "it's serious bug.");

	int32 nnetframe = this->_num_frames_decoded;
    int32 frame = this->_active_toks.size() - 1;
    this->_active_toks.resize(this->_active_toks.size() + 1);

	Elem *final_toks = this->_toks.Clear(); // analogous to swapping prev_toks_ / cur_toks_

	Elem *best_elem = NULL;
	BaseFloat adaptive_beam;
	size_t tok_cnt = 0;
	
	BaseFloat cur_cutoff = this->GetCutoff(final_toks, &tok_cnt, &adaptive_beam, &best_elem);

	VLOG_COM(2) << "Adaptive beam on frame " << this->NumFramesDecoded()
		<< "nnet frame " << nnetframe
		<< " is " << adaptive_beam ;
	this->PossiblyResizeHash(tok_cnt);  // This makes sure the hash is always big enough.

	BaseFloat next_cutoff = FLOAT_INF;
	if (best_elem != NULL)
	{
		PairId state_pair = best_elem->key;
		StateId stateid = PairToState(state_pair), // state in "fst"
				lm_stateid = PairToLmState(state_pair); // in lm

		Token *tok = best_elem->val;
		for(ArcIterator<FST> iter(this->_graph, stateid); !iter.Done(); iter.Next())
		{
			const StdArc *arc = &iter.Value();
			if (arc->_input != 0)
			{ // propagate
				BaseFloat lm_score;
				this->NextLmState(lm_stateid, arc->_output, &lm_score);
				BaseFloat tot_score = lm_score + tok->_tot_cost + arc->_w \
									  - decodable->LogLikelihood(nnetframe, arc->_input);
				if(tot_score + adaptive_beam < next_cutoff)
					next_cutoff = tot_score + adaptive_beam;
			}
		}
	}

	VLOG_COM(2) << "frame is : " << frame << " -> current cutoff " << cur_cutoff << " next cutoff " << next_cutoff ;
     VLOG_COM(2) << "tokens is " << this->_num_toks  << " links is " << this->_num_links ;
     VLOG_COM(3) << "current cutoff " << cur_cutoff << " next cutoff " << next_cutoff;

	for(Elem *e = final_toks, *e_tail = NULL; e != NULL; e = e_tail)
	{
		PairId state_pair = e->key;
		StateId stateid = PairToState(state_pair),
				lm_stateid = PairToLmState(state_pair);
		Token *tok = e->val;
		if(tok->_tot_cost <= cur_cutoff)
		{
			for(ArcIterator<FST> iter(this->_graph, stateid); !iter.Done(); iter.Next())
			{
				const StdArc *arc = &iter.Value();
				if (arc->_input != 0)
				{
					BaseFloat lm_score;
					StateId next_lm_state = this->NextLmState(lm_stateid, arc->_output, &lm_score);
					BaseFloat ac_cost = - decodable->LogLikelihood(nnetframe, arc->_input);
					BaseFloat graph_cost = arc->_w + lm_score;
					BaseFloat cur_cost = tok->_tot_cost;
					BaseFloat tot_cost = cur_cost + ac_cost + graph_cost;
					if(tot_cost >= next_cutoff)
						continue;
					else if(tot_cost + adaptive_beam < next_cutoff)
						next_cutoff = tot_cost + adaptive_beam; // prune by best current token

					PairId next_pair = this->ConstructPair(arc->_to, next_lm_state);

					Elem *next_tok = this->FindOrAddToken(next_pair,
							frame + 1, tot_cost, tok, NULL);
					tok->_links = this->NewForwardLink(next_tok->val, arc->_input, arc->_output,
							graph_cost, ac_cost, tok->_links);
				}
			}// for all arc
		}
		e_tail = e->tail;
		this->_toks.Delete(e); // e give back hash, delete e
	} // end propagate
	this->_num_frames_decoded++;
	return next_cutoff;
}

template <typename FST, typename Token>
void OnlineLatticeDecoderMempoolBaseBiglm<FST, Token>::ProcessNonemitting(float cutoff)
{
	LOG_ASSERT(!this->_active_toks.empty() && "_active_toks shouldn't be empty.");

	int32 frame = static_cast<int>(this->_active_toks.size()) - 1;
	LOG_ASSERT(this->_queue.empty());

	if (this->_toks.GetList() == NULL)
	{
		if (this->_warned != true)
		{
			LOG_WARN << "Error, no surviving tokens: frame is " << frame;
			this->_warned = true;
		}
	}
	for(const Elem *e = this->_toks.GetList(); e!= NULL; e = e->tail)
	{
		PairId state_pair = e->key;
		StateId state = this->PairToState(state_pair);
		if(this->_graph->NumInputEpsilons(state) != 0)
			this->_queue.push_back(e);
	}

	while (!_queue.empty())
	{
		const Elem *elem = _queue.back();
		_queue.pop_back();

		PairId state_pair = elem->key;
		Token *tok = elem->val;
		BaseFloat cur_cost = tok->_tot_cost;
		if(cur_cost >= cutoff) // Don't process
			continue;

		StateId state = this->PairToState(state_pair),
				lm_state = this->PairToLmState(state_pair);
		this->DeleteForwardLinks(tok);
		for(ArcIterator<FST> iter(this->_graph, state);
				!iter.Done(); iter.Next())
		{
			const StdArc *arc = &iter.Value();
			if(arc->_input == 0)
			{
				BaseFloat lm_score;
				StateId next_lm_state = this->NextLmState(lm_state, arc->_output, &lm_score);
				BaseFloat graph_cost = arc->_w + lm_score;
				BaseFloat tot_cost = cur_cost + graph_cost;
				if(tot_cost < cutoff)
				{
					bool changed = false;
					PairId next_pair = this->ConstructPair(arc->_to, next_lm_state);
					Elem *new_tok = this->FindOrAddToken(next_pair, frame, tot_cost, tok,
							&changed);
					tok->_links = this->NewForwardLink(new_tok->val, 0, arc->_output,
							graph_cost, 0, tok->_links);
					if(changed && this->_graph->NumInputEpsilons(arc->_to) != 0)
						this->_queue.push_back(new_tok);
				}
			}
		} // for all arcs in this node.
	} // while queue not empty.
}

template <typename FST, typename Token>
void OnlineLatticeDecoderMempoolBaseBiglm<FST, Token>::PruneForwardLinksFinal()
{
	typedef typename unordered_map<Token*, BaseFloat>::const_iterator IterType;
	LOG_ASSERT(!this->_active_toks.empty());
	int32 frame_plus_one = this->_active_toks.size() - 1;
	if (this->_active_toks[frame_plus_one]._toks == NULL)  // empty list; should not happen.
		LOG_WARN << "No tokens alive at end of file";
	ComputeFinalCosts(&this->_final_costs, &this->_final_relative_cost, &this->_final_best_cost);
	this->_decoding_finalized = true;

	// We call DeleteElems() as a nicety, not because it's really necessary;
	// otherwise there would be a time, after calling PruneTokensForFrame() on the
	// final frame, when toks_.GetList() or toks_.Clear() would contain pointers
	// to nonexistent tokens.
	this->_toks.DeleteElems();
	// Now go through tokens on this frame, pruning forward links...  may have to
	// iterate a few times until there is no more change, because the list is not
	// in topological order.  This is a modified version of the code in
	// PruneForwardLinks, but here we also take account of the final-probs.
	bool changed = true;
	BaseFloat delta = 1.0e-5;
	while(changed)
	{
		changed = false;
		for(Token *tok = this->_active_toks[frame_plus_one]._toks;
				tok != NULL; tok = tok->_next)
		{
			ForwardLinkT *link, *prev_link = NULL;
			// will recompute tok_extra_cost.  It has a term in it that corresponds
			// to the "final-prob", so instead of initializing tok_extra_cost to infinity
			// below we set it to the difference between the (score+final_prob) of this token,
			// and the best such (score+final_prob).
			BaseFloat final_cost = 0;
			if(this->_final_costs.empty())
			{
				final_cost = 0.0;
			}
			else
			{
				IterType iter = this->_final_costs.find(tok);
				if(iter != this->_final_costs.end())
				{
					final_cost = iter->second;
				}
				else
				{
					final_cost = FLOAT_INF;
				}
			}
			BaseFloat tok_extra_cost = tok->_tot_cost + final_cost - 
				this->_final_best_cost; // - _final_best_cost for cut lattice
			// tok_extra_cost will be a "min" over either directly being final, or
			// being indirectly final through other links, and the loop below may
			// decrease its value:
			for(link = tok->_links; link != NULL; )
			{ // See if we need to excise this link...
				Token *next_tok = link->_next_tok;
				BaseFloat link_extra_cost = next_tok->_extra_cost +
					((tok->_tot_cost + link->_acoustic_cost + link->_graph_cost)
					 - next_tok->_tot_cost);
				if(link_extra_cost > this->_config._lattice_beam)
				{
					ForwardLinkT *next_link = link->_next;
					if (prev_link != NULL)
						prev_link->_next = next_link;
					else
						tok->_links = next_link;
					this->DeleteForwardLink(link);
					link = next_link; // advance link but leave prev_link the same.
				}
				else
				{ // keep the link and update the tok_extra_cost if needed.
					if (link_extra_cost < 0.0) 
					{ // this is just a precaution.
						if (link_extra_cost < -0.01)
						{
							LOG_WARN << "Negative extra_cost: " << link_extra_cost;
						}
						link_extra_cost = 0.0;
					}
					if (link_extra_cost < tok_extra_cost)
						tok_extra_cost = link_extra_cost;
					prev_link = link;
					link = link->_next;
				}
			}
			// prune away tokens worse than lattice_beam above best path.  This step
			// was not necessary in the non-final case because then, this case
			// showed up as having no forward links.  Here, the tok_extra_cost has
			// an extra component relating to the final-prob.
			if (tok_extra_cost > this->_config._lattice_beam)
				tok_extra_cost = FLOAT_INF;
			// to be pruned in PruneTokensForFrame

			if(fabs(tok->_extra_cost - tok_extra_cost) > delta)
				changed = true;
			tok->_extra_cost = tok_extra_cost; // will be +infinity or <= lattice_beam_.
		}
	} // while changed
}

typedef OnlineLatticeDecoderMempoolBaseBiglm<Fst, StdToken> OnlineLatticeDecoderMempoolBiglm;

#include "src/util/namespace-end.h"

#endif

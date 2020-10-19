
#include <iostream>
#include <math.h>
#include <unordered_set>
#include "src/util/log-message.h"
#include "src/decoder/mem-optimize-clg-lattice-faster-online-decoder.h"

#include "src/util/namespace-start.h"

void MemOptimizeClgLatticeFasterOnlineDecoder::InitDecoding()
{
	ClearActiveTokens();
	DeletePool();
	// clean up from last time:
	_toks.DeleteElems();
//	_prev_toks.clear();
	_active_toks.clear();
	_queue.clear();
	_tmp_array.clear(); // for calculate cutoff score
	_cost_offsets.clear();
	_final_costs.clear();

	_num_toks = 0;
	LOG_ASSERT(_num_toks == 0);
	LOG_ASSERT(_num_links == 0);

	_decoding_finalized = false;

	ClgTokenStateId start_state = _graph->Start();

	_active_toks.resize(1);
	Token *start_tok = NewToken(0.0, 0.0, NULL, NULL, NULL);
	_active_toks[0]._toks = start_tok;

	_toks.Insert(start_state, start_tok);
	_num_toks++;
	ProcessNonemitting(_config._beam);
	_num_frames_decoded = 0;
}

void MemOptimizeClgLatticeFasterOnlineDecoder::ClearActiveTokens()
{// a cleanup routine, at utt end/begin
	for(size_t i = 0; i < _active_toks.size(); ++i)
	{
		for(Token *tok = _active_toks[i]._toks; tok != NULL ;)
		{
			DeleteForwardLinks(tok);
			Token *next_tok = tok->_next;
			LOG_ASSERT(tok->CheckNoLink());
			DeleteToken(tok);
			_num_toks--;
			tok = next_tok;
		}
	}
	_active_toks.clear();
	LOG_ASSERT(_num_toks == 0 && _num_links == 0);
}

// FindOrAddToken either locates a token in hash of _toks,
MemOptimizeClgLatticeFasterOnlineDecoder::Elem *MemOptimizeClgLatticeFasterOnlineDecoder::FindOrAddToken(ClgTokenStateId stateid,
		int frame_plus_one, float tot_cost, Token *backpointer, bool *changed)
{
	// Returns the Token pointer.  Sets "changed" (if non-NULL) to true
	// if the token was newly created or the cost changed.
	LOG_ASSERT((size_t)frame_plus_one < _active_toks.size());
	Token *&toks = _active_toks[frame_plus_one]._toks;

	Elem *e_found = _toks.Find(stateid);
	if(e_found == NULL)
	{ // this state not arrive in current frame.
		const float extra_cost = 0.0;
		Token *new_tok = NewToken(tot_cost, extra_cost, NULL, toks, backpointer);
		// NULL: no forward links yet
		toks = new_tok;
		_num_toks++;
		e_found = _toks.Insert(stateid, new_tok);
		if (changed != NULL) 
			*changed = true;
		return e_found;
	}
	else
	{
		Token *tok = e_found->val;
		if(tok->_tot_cost > tot_cost)
		{
			// we don't allocate a new token, the old stays linked in _active_toks
			// we only replace the tot_cost
			// in the current frame, there are no forward links (and no extra_cost)
			// only in ProcessNonemitting we have to delete forward links
			// in case we visit a state for the second time
			// those forward links, that lead to this replaced token before:
			// they remain and will hopefully be pruned later (PruneForwardLinks...)
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

/*************
 * before ProcessEmitting, call GetCutoff, get this frame cutoff.
 *************/
float MemOptimizeClgLatticeFasterOnlineDecoder::GetCutoff(Elem *list_head, size_t *tok_count,
		float *adaptive_beam, Elem **best_elem)
{
	size_t count = 0;
	float best_weight = FLOAT_INF;

	if(_config._max_active == std::numeric_limits<int>::max() &&
			_config._min_active == 0)
	{
		for(Elem *e=list_head; e!= NULL; e = e->tail, ++count)
		{
			float w = e->val->_tot_cost;
			if(w < best_weight)
			{
				best_weight = w;
				*best_elem = e;
			}
		}
		*tok_count = count;
		*adaptive_beam = _config._beam;
		return best_weight + _config._beam;
	}
	else
	{ // record the best socre , and add score to _tmp_array for cut off.
		_tmp_array.clear();
		for(Elem *e=list_head; e!= NULL; e = e->tail, ++count)
		{
			float w = e->val->_tot_cost;
			_tmp_array.push_back(w);
			if(w < best_weight)
			{
				best_weight = w;
				*best_elem = e;
			}
		}
		*tok_count = count;

		float beam_cutoff = best_weight + _config._beam;
		float min_active_cutoff = FLOAT_INF;
		float max_active_cutoff = FLOAT_INF;

#ifdef DEBUG
		VLOG(2) << "Number of tokens active on frame " << NumFramesDecoded() 
			<< "is" << _tmp_array.size() ;
#endif
		if(_tmp_array.size() > static_cast<size_t>(_config._max_active))
		{
			std::nth_element(_tmp_array.begin(),
					_tmp_array.begin() + _config._max_active,
					_tmp_array.end());
			max_active_cutoff = _tmp_array[_config._max_active];
//			return max_active_cutoff;
		}

		if(max_active_cutoff < beam_cutoff)
		{ // max_active is tighter than beam.
			// turn down beam.
			*adaptive_beam = max_active_cutoff - best_weight + _config._beam_delta;
			return max_active_cutoff;
		}

		if (_tmp_array.size() > static_cast<size_t>(_config._min_active))
		{
			if (_config._min_active == 0) 
				min_active_cutoff = best_weight;
			else
			{
				std::nth_element(_tmp_array.begin(),
						_tmp_array.begin() + _config._min_active,
						_tmp_array.size() > static_cast<size_t>(_config._max_active) ?
						_tmp_array.begin() + _config._max_active :
						_tmp_array.end());
				min_active_cutoff = _tmp_array[_config._min_active];
			}
		}

		if(min_active_cutoff > beam_cutoff)
		{ // min_active is looser than beam.
			// turn up beam.
			*adaptive_beam = min_active_cutoff - best_weight + _config._beam_delta;
			return min_active_cutoff;
		}
		else
		{
			*adaptive_beam = _config._beam;
			return beam_cutoff;
		}
	}// end if
}

void MemOptimizeClgLatticeFasterOnlineDecoder::PossiblyResizeHash(size_t num_toks)
{
	size_t new_sz = static_cast<size_t>(static_cast<float>(num_toks) 
			* _config._hash_ratio);
	if(new_sz > _toks.Size())
	{
		_toks.SetSize(new_sz);
	}
}

float MemOptimizeClgLatticeFasterOnlineDecoder::ProcessEmitting(DecodableInterface *decodable)
{
	LOG_ASSERT(_active_toks.size() > 0 && "it's serious bug.");

	int nnetframe = _num_frames_decoded;
	int frame = _active_toks.size() - 1;
	_active_toks.resize(_active_toks.size() + 1);

//	ClgTokenStateId tot_state = _graph->TotState();
//	Label blkid = decodable->GetBlockTransitionId();

	Elem *final_toks = _toks.Clear(); // analogous to swapping prev_toks_ / cur_toks_

	Elem *best_elem = NULL;
	float adaptive_beam;
	size_t tok_cnt = 0;

	// through prev frame token score , calculate current cutoff value.
	// first , get currnet frame cut off , function PruneActiveTokens 
	// will use this cut off.

	float cur_cutoff = GetCutoff(final_toks, &tok_cnt, &adaptive_beam, &best_elem);
#ifdef DEBUG
	VLOG(2) << "Adaptive beam on frame " << NumFramesDecoded() 
		<< "nnet frame " << nnetframe 
		<< " is " << adaptive_beam ;
#endif
	PossiblyResizeHash(tok_cnt);  // This makes sure the hash is always big enough.

	float next_cutoff = FLOAT_INF;//std::numeric_limits<float>::infinity();

//  I think it's useless.
//	float cost_offset = 0.0; // Used to keep probabilities in a good dynamic range.
							 // keep the best tot_cost every times offset to 0.0.
	// First process the best token to get a hopefully
	// reasonably tight bound on the next cutoff.  The only
	// products of the next block are "next_cutoff" and "cost_offset".
	if (best_elem != NULL)
	{
		ClgTokenStateId stateid = best_elem->key; 
		Token *tok = best_elem->val;
		StdState *state = _graph->GetState(stateid);
		if(_graph->StateIdInClg(stateid))
		{ // in clg fst
			for(int i = 0; i < static_cast<int>(state->GetArcSize()); ++i)
			{
				StdArc *clgarc = state->GetArc((unsigned)i);
				if(clgarc->_input != 0)
				{
					ClgTokenStateId hmmstateid = _graph->MapClgTokenStateId(stateid, clgarc);
					StdState *hmmstate = _graph->GetState(hmmstateid);
					// in hmm propagate
					for(int j = 0; j < static_cast<int>(hmmstate->GetArcSize()); ++j)
					{
						StdArc *arc = hmmstate->GetArc((unsigned)j);
						if (arc->_input != 0)
						{ // propagate
							float tot_score =  tok->_tot_cost + 
								clgarc->_w.Value() + 
								arc->_w.Value() - decodable->LogLikelihood(nnetframe, arc->_input);
							if(tot_score + adaptive_beam < next_cutoff)
								next_cutoff = tot_score + adaptive_beam;
						}
					}
				}
			}
		}
		else
		{ // in hmm fst
			for(int i = 0; i < static_cast<int>(state->GetArcSize()); ++i)
			{
				StdArc *arc = state->GetArc((unsigned)i);
				if (arc->_input != 0)
				{ // propagate
					float tot_score =  tok->_tot_cost + arc->_w.Value()
						- decodable->LogLikelihood(nnetframe, arc->_input);
					if(tot_score + adaptive_beam < next_cutoff)
						next_cutoff = tot_score + adaptive_beam;
				}
			}
		}
		if(next_cutoff == FLOAT_INF)
		{
			next_cutoff = tok->_tot_cost + adaptive_beam;
			LOG_WARN << "it shouldn't happen!!!";
			LOG_WARN << "frame is : " << frame << " -> current cutoff " << cur_cutoff << " next cutoff " << next_cutoff ;
		}
	}
	else
	{
		LOG_ERR << "it's serious error." ;
	}

	VLOG_COM(2) << "frame is : " << frame << " -> current cutoff " << cur_cutoff << " next cutoff " << next_cutoff ;
	VLOG_COM(2) << "tokens is " << _num_toks  << " links is " << _num_links ;
#ifdef DEBUG
	VLOG(2) << "current cutoff " << cur_cutoff << " next cutoff " << next_cutoff ;
#endif
	// the tokens are now owned here, in _prev_toks.
	//for(auto it = _prev_toks.begin(); it != _prev_toks.end(); ++it)
	for(Elem *e = final_toks, *e_tail = NULL; e != NULL; e = e_tail)
	{
		ClgTokenStateId stateid = e->key;
		Token *tok = e->val;
		if(tok->_tot_cost < cur_cutoff)
		{// it's OK.
			StdState *state = _graph->GetState((unsigned)stateid);
			if(_graph->StateIdInClg(stateid))
			{ // in clg fst
				for(int i = 0; i < static_cast<int>(state->GetArcSize()); ++i)
				{
					StdArc *clgarc = state->GetArc((unsigned)i);
					if(clgarc->_input != 0)
					{
						ClgTokenStateId hmmstateid = _graph->MapClgTokenStateId(stateid, clgarc);
						StdState *hmmstate = _graph->GetState(hmmstateid);
						// hmm propagate
						for(int j = 0; j < static_cast<int>(hmmstate->GetArcSize()); ++j)
						{
							StdArc *arc = hmmstate->GetArc((unsigned)j);
							if (arc->_input != 0)
							{
								float ac_cost = - decodable->LogLikelihood(nnetframe, arc->_input);
								float graph_cost = arc->_w.Value() + clgarc->_w.Value();
								float cur_cost = tok->_tot_cost;
								float tot_cost = cur_cost + ac_cost + graph_cost;
								if(tot_cost > next_cutoff)
									continue;
								else if(tot_cost + adaptive_beam < next_cutoff)
									next_cutoff = tot_cost + adaptive_beam; // prune by best current token
								// Note: the frame indexes into _active_toks are one-based,
								// hence the + 1.
								ClgTokenStateId arrivestateid = _graph->MapClgTokenStateId(hmmstateid, arc);
								Elem *next_tok = FindOrAddToken(arrivestateid,
										frame + 1, tot_cost, tok, NULL);
								// Add ForwardLink from tok to next_tok (put on head of list tok->links)
								tok->_links = NewForwardLink(next_tok->val, arc->_input, clgarc->_output,
										graph_cost, ac_cost, tok->_links);
								_num_links++;
							}
						}
					}
				} // all clg arc
			}
			else
			{ // in hmm state
				for(int i = 0; i < static_cast<int>(state->GetArcSize()); ++i)
				{
					const StdArc *arc = state->GetArc((unsigned)i);
					if(arc->_input != 0) // can't be black state
					{ // propagate
						float ac_cost = - decodable->LogLikelihood(nnetframe, arc->_input);
						float graph_cost = arc->_w.Value();
						float cur_cost = tok->_tot_cost;
						float tot_cost = cur_cost + ac_cost + graph_cost;
						if(tot_cost > next_cutoff)
							continue;
						else if(tot_cost + adaptive_beam < next_cutoff)
							next_cutoff = tot_cost + adaptive_beam; // prune by best current token
						// Note: the frame indexes into _active_toks are one-based,
						// hence the + 1.
						ClgTokenStateId arrivestateid = _graph->MapClgTokenStateId(stateid, arc);
						Elem *next_tok = FindOrAddToken(arrivestateid,
								frame + 1, tot_cost, tok, NULL);
						// Add ForwardLink from tok to next_tok (put on head of list tok->links)
						tok->_links = NewForwardLink(next_tok->val, arc->_input, arc->_output,
								graph_cost, ac_cost, tok->_links);
						_num_links++;
					}
				}
			}// for all arc.
		} // this state ok
		e_tail = e->tail;
		_toks.Delete(e); // e give back hash
	} // end propagate
//	_prev_toks.clear();
	_num_frames_decoded++;
	return next_cutoff;
}

void MemOptimizeClgLatticeFasterOnlineDecoder::ProcessNonemitting(float cutoff)
{
	LOG_ASSERT(!_active_toks.empty() && "_active_toks shouldn't be empty.");
	int frame = static_cast<int>(_active_toks.size()) - 1;
	// frame is the time-index, if frame=1 , it's the first frame.
	// call from InitDecoding()
	
	// Processes nonemitting arcs for one frame.  Propagates within toks_.
	// Note-- this queue structure is not very optimal as
	// it may cause us to process states unnecessarily (e.g. more than once),
	// but I think this problem did not reduce speed.
	LOG_ASSERT(_queue.empty() && "_queue must be empty.");

	//for(auto it = _cur_toks.begin(); it != _cur_toks.end();++it)
	for(const Elem *e = _toks.GetList(); e!= NULL; e = e->tail)
		_queue.push_back(e);

	if (_queue.empty())
	{
		LOG_ERR << "Error, no surviving tokens: frame is " 
			<< frame ;
	}

	while (!_queue.empty())
	{
		const Elem *e= _queue.back();
		_queue.pop_back();

		ClgTokenStateId stateid = e->key;
		Token *tok = e->val; // would segfault if state not in toks_ but this can't happen.
		//Token *tok = _cur_toks[stateid];
		float cur_cost = tok->_tot_cost;
		if(cur_cost > cutoff) // Don't process
			continue;

		// If "tok" has any existing forward links, delete them,
		// because we're about to regenerate them.  This is a kind
		// of non-optimality (remember, this is the simple decoder),
		// but since most states are emitting it's not a huge issue.
		// because this state can be possible arrive from an eps arc.
		DeleteForwardLinks(tok); // necessary when re-visiting
// up need process ,but now don't change it.
// all code is ok,I will optomize.
		StdState *state = _graph->GetState((unsigned)stateid);
		LOG_ASSERT(state != NULL && "can not find this state.");

		for(int i=0; i < static_cast<int>(state->GetArcSize()); ++i)
		{
			const StdArc *arc = state->GetArc((unsigned)i);
			if(arc->_input == 0)
			{ // propagate eps edge
				float graph_cost = arc->_w.Value();
				float tot_cost = cur_cost + graph_cost;
				if(tot_cost < cutoff)
				{
					bool changed = false;
					ClgTokenStateId arrivestateid = _graph->MapClgTokenStateId(stateid, arc);
					Elem *new_tok = FindOrAddToken(arrivestateid, frame, tot_cost,
							tok, &changed);
					tok->_links = NewForwardLink(new_tok->val, 0, arc->_output,
							graph_cost, 0, tok->_links);
					_num_links++;
					// if add node have exist and the tot_cost > this tot_cost
					// this situation don't add this node to _queue.
					if(changed)
						_queue.push_back(new_tok);
				}
			}
		}// for all arcs in this node.
	}// while queue not empty.
}

// Go backwards through still-alive tokens, pruning them, starting not from
// the current frame (where we want to keep all tokens) but from the frame before
//  that.  We go backwards through the frames and stop when we reach a point
//  where the delta-costs are not changing (and the delta controls when we consider
// a cost to have "not changed").
void MemOptimizeClgLatticeFasterOnlineDecoder::PruneActiveTokens(float delta)
{
	int cur_frame_plus_one = NumFramesDecoded();
#ifdef DEBUG
	int num_toks_begin = _num_toks;
#endif
	// The index "f" below represents a "frame plus one", i.e. you'd have to subtract
	// one to get the corresponding index for the decodable object.
	for (int f = cur_frame_plus_one - 1; f >= 0; f--) 
	{
		// Reason why we need to prune forward links in this situation:
		// (1) we have never pruned them (new TokenList)
		// (2) we have not yet pruned the forward links to the next f,
		// after any of those tokens have changed their extra_cost.
		// first cut current eps arc.
		
		// here I only prune don't arrive the last frame the links and tokens
		// don't use score cut.
		if (_active_toks[f]._must_prune_forward_links)
		{
			bool links_pruned = false, extra_costs_changed = false;

			// because token and ForwardLink storage struction , 
			// must be cut prev frame ForwardLink , before cut current frame Token.
			// and examine prev frame whether be cut.
			// if not ,need not be cut continue.
			PruneForwardLinks(f, &extra_costs_changed, &links_pruned, delta);
			if (extra_costs_changed && f > 0) // any token has changed extra_cost
				_active_toks[f-1]._must_prune_forward_links = true;
			if (links_pruned) // any link was pruned
				_active_toks[f]._must_prune_tokens = true;
			_active_toks[f]._must_prune_forward_links = false; // job done
		}
		// except for last f (no forward links
		if (f+1 < cur_frame_plus_one && 
				_active_toks[f+1]._must_prune_tokens)
		{
			PruneTokensForFrame(f+1);
			_active_toks[f+1]._must_prune_tokens = false;
		}
	}
#ifdef DEBUG
	VLOG(2) << "pruned tokens from " << num_toks_begin
   		<< " to " << _num_toks ;
#endif
}

void MemOptimizeClgLatticeFasterOnlineDecoder::PruneForwardLinks(int frame_plus_one,
		bool *extra_costs_changed, bool *links_pruned,
		float delta)
{
	LOG_ASSERT(frame_plus_one >= 0 && 
			(size_t)frame_plus_one < _active_toks.size() && "it's error.");
	LOG_ASSERT(_active_toks[frame_plus_one]._toks != NULL && "empty list; should not happen.");
	*extra_costs_changed = false;
	*links_pruned = false;

	// delta is the amount by which the extra_costs must change
	// If delta is larger,  we'll tend to go back less far
	//    toward the beginning of the file.
	// prev_link_pruned is set to true if token have no links.
	// links_pruned is set to true if any link in any token was pruned
	
	LOG_ASSERT(frame_plus_one >= 0 && (size_t)frame_plus_one < _active_toks.size());
	if(_active_toks[frame_plus_one]._toks == NULL)
	{
		//empty list; should not happen.
		LOG_ERR << "No tokens alive [doing pruning].. warning first time only for each utterance" ;
	}

	// We have to iterate until there is no more change, because the links
	// are not guaranteed to be in topological order.
	bool changed = true; // difference new minus old extra cost >= delta ?
	while (changed)
	{
		changed = false;	
		for (Token *tok = _active_toks[frame_plus_one]._toks;
				tok != NULL; tok = tok->_next) 
		{
			ForwardLink *link, *prev_link = NULL;
			float tok_extra_cost = FLOAT_INF;
			for (link = tok->_links; link != NULL; )
			{
				// See if we need to excise this link...
				Token *next_tok = link->_next_tok;
				float link_extra_cost = next_tok->_extra_cost +
					((tok->_tot_cost + link->_acoustic_cost + link->_graph_cost)
					 - next_tok->_tot_cost);  // difference in brackets is >= 0
			
				// link_exta_cost is the difference in score between the best paths
				// through link source state and through link destination state
			
				LOG_ASSERT(link_extra_cost == link_extra_cost);  // check for NaN;
				if ( link_extra_cost > _config._lattice_beam )
				{ // excise link
					ForwardLink *next_link = link->_next;
					if (prev_link != NULL)
					   	prev_link->_next = next_link;
					else
						tok->_links = next_link;
					DeleteForwardLink(link);
					_num_links--;
					link = next_link;
					*links_pruned = true;
				}
				else // keep the link and update the tok_extra_cost if needed.
				{
					if (link_extra_cost < 0.0) 
					{  // this is just a precaution.
						if (link_extra_cost < -0.01)
							LOG_WARN << "Negative extra_cost: " 
								<< link_extra_cost ;
						link_extra_cost = 0.0;
					}
					if(link_extra_cost < tok_extra_cost)
					{
						tok_extra_cost = link_extra_cost;
					}
					prev_link = link;
					link = link->_next;
				}
			} // for all outgoing links
			if(fabs(tok_extra_cost - tok->_extra_cost) > delta)
				changed = true; // difference new minus old is bigger than delta
			tok->_extra_cost = tok_extra_cost;
			// will be +infinity or <= lattice_beam_.
		    // infinity indicates, that no forward link survived pruning
		} // for all Token on _active_toks[frame]
		if (changed) 
			*extra_costs_changed = true;
		// Note: it's theoretically possible that aggressive compiler
		// optimizations could cause an infinite loop here for small delta and
		// high-dynamic-range scores.
	}   // while changed
}

// Prune away any tokens on this frame that have no forward links.
// [we don't do this in PruneForwardLinks because it would give us
// a problem with dangling pointers].
// It's called by PruneActiveTokens if any forward links have been pruned
void MemOptimizeClgLatticeFasterOnlineDecoder::PruneTokensForFrame(int frame_plus_one)
{
	LOG_ASSERT(frame_plus_one >= 0 && (size_t)frame_plus_one < _active_toks.size());
	Token *&toks = _active_toks[frame_plus_one]._toks;

	if(toks == NULL)
		LOG_WARN << "No tokens alive [doing pruning]" ;

	Token *tok, *next_tok, *prev_tok = NULL;
	for (tok = toks; tok != NULL; tok = next_tok)
	{
		next_tok = tok->_next;
		if (tok->_extra_cost == FLOAT_INF)
		{ // token is unreachable from end of graph; (no forward links survived)
		  // excise tok from list and delete tok.
			if (prev_tok != NULL) 
				prev_tok->_next = tok->_next;
			else 
				toks = tok->_next;
			// check token
			LOG_ASSERT(tok->CheckNoLink());
			DeleteToken(tok);
			_num_toks -- ;
		}
		else
		{
			prev_tok = tok;
		}
	}
}

bool MemOptimizeClgLatticeFasterOnlineDecoder::Decode(DecodableInterface *decodable)
{
	InitDecoding();

	while((_num_frames_decoded - 1) < decodable->NumFramesReady())
	{
		// prune active tokens.
		if (NumFramesDecoded() % _config._prune_interval == 0)
			PruneActiveTokens(_config._lattice_beam * _config._prune_scale);
//		_prev_toks.clear();
//		_cur_toks.swap(_prev_toks);
		float cost_cutoff = ProcessEmitting(decodable);
		ProcessNonemitting(cost_cutoff);
	}
	FinalizeDecoding();

	return !_active_toks.empty() && _active_toks.back()._toks != NULL;
}

void MemOptimizeClgLatticeFasterOnlineDecoder::AdvanceDecoding(DecodableInterface *decodable,
		int max_num_frames)
{
	LOG_ASSERT(_num_frames_decoded >= 0 && !_decoding_finalized &&
			"You must call InitDecoding() before AdvanceDecoding");
	int num_frames_ready = decodable->NumFramesReady();
	// num_frames_ready must be >= num_frames_decoded, or else
	// the number of frames ready must have decreased (which doesn't
	// make sense) or the decodable object changed between calls
	// (which isn't allowed).
	LOG_ASSERT(num_frames_ready >= _num_frames_decoded);

	int target_frames_decoded = num_frames_ready;
	if(max_num_frames >= 0)
	{
		target_frames_decoded = std::min(target_frames_decoded,				    
				_num_frames_decoded + max_num_frames);
	}
	while(_num_frames_decoded < target_frames_decoded)
	{
		/*
		if(true == decodable->SkipBlockFrame(_num_frames_decoded))
		{
			_num_frames_decoded++;
			continue;
		}
		*/
#ifdef DEBUG
		VLOG(2) << "frame:" << _num_frames_decoded << "@number links " << _num_links << " number tokens "
			<< _num_toks ;
#endif
		// prune active tokens.
		if (NumFramesDecoded() % _config._prune_interval == 0)
			PruneActiveTokens(_config._lattice_beam * _config._prune_scale);
//		_prev_toks.clear();
//		_cur_toks.swap(_prev_toks);
		// note: ProcessEmitting() increments num_frames_decoded_
		float weight_cutoff = ProcessEmitting(decodable);
		ProcessNonemitting(weight_cutoff);
	}
}

void MemOptimizeClgLatticeFasterOnlineDecoder::ComputeFinalCosts(
		unordered_map<Token*, float> *final_costs,
		float *final_relative_cost,
		float *final_best_cost) const
{
	LOG_ASSERT(!_decoding_finalized);
	if (final_costs != NULL)
		final_costs->clear();

	float infinity = FLOAT_INF;
	float best_cost = infinity;
	float best_cost_with_final = infinity;

	const Elem *final_toks = _toks.GetList();
	//for(_cur_toks.begin() ; it != _cur_toks.end() ; ++it)
	while(final_toks != NULL)
	{
		ClgTokenStateId stateid = final_toks->key;
		Token *tok = final_toks->val;
		const Elem *next = final_toks->tail;
		bool fst_final = _graph->IsFinal(stateid); // here I don't use infinity inside of log(0).
		best_cost = std::min(tok->_tot_cost, best_cost);
		if(final_costs != NULL && fst_final == true)
		{ // for all final state mark.
			(*final_costs)[tok] = 0;
			best_cost_with_final = std::min(tok->_tot_cost, best_cost_with_final);
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

// PruneForwardLinksFinal is a version of PruneForwardLinks that we call
// on the final frame.  If there are final tokens active, it uses
// the final-probs for pruning, otherwise it treats all tokens as final.
void MemOptimizeClgLatticeFasterOnlineDecoder::PruneForwardLinksFinal()
{
	typedef unordered_map<Token*, float>::const_iterator IterType;
	LOG_ASSERT(!_active_toks.empty());
//	float infinity = FLOAT_INF;
	int frame_plus_one = _active_toks.size() - 1;
	ComputeFinalCosts(&_final_costs, &_final_relative_cost, &_final_best_cost);
	_decoding_finalized = true;
	
	// We call DeleteElems() as a nicety, not because it's really necessary;
	// otherwise there would be a time, after calling PruneTokensForFrame() on the
	// final frame, when toks_.GetList() or toks_.Clear() would contain pointers
	// to nonexistent tokens.
	_toks.DeleteElems();

	// Now go through tokens on this frame, pruning forward links...  may have to
	// iterate a few times until there is no more change, because the list is not
	// in topological order.  This is a modified version of the code in
	// PruneForwardLinks, but here we also take account of the final-probs.
	bool changed = true;
	float delta = 1.0e-5;
	while(changed)
	{
		changed = false;
		for(Token *tok = _active_toks[frame_plus_one]._toks;
				tok != NULL; tok = tok->_next)
		{
			ForwardLink *link, *prev_link = NULL;
			// will recompute tok_extra_cost.  It has a term in it that corresponds
			// to the "final-prob", so instead of initializing tok_extra_cost to infinity
			// below we set it to the difference between the (score+final_prob) of this token,
			// and the best such (score+final_prob).
			float final_cost = 0;
			if(_final_costs.empty())
			{
				final_cost = 0.0;
			}
			else
			{
				IterType iter = _final_costs.find(tok);
				if(iter != _final_costs.end())
				{
					final_cost = iter->second;
				}
				else
				{
					final_cost = FLOAT_INF;
				}
			}
			float tok_extra_cost = tok->_tot_cost + final_cost - _final_best_cost; // - _final_best_cost for cut lattice
			// tok_extra_cost will be a "min" over either directly being final, or
			// being indirectly final through other links, and the loop below may
			// decrease its value:
			for(link = tok->_links; link != NULL; )
			{ // See if we need to excise this link...
				Token *next_tok = link->_next_tok;
				float link_extra_cost = next_tok->_extra_cost +
					((tok->_tot_cost + link->_acoustic_cost + link->_graph_cost)
					 - next_tok->_tot_cost);
				if(link_extra_cost > _config._lattice_beam)
				{
					ForwardLink *next_link = link->_next;
					if (prev_link != NULL)
						prev_link->_next = next_link;
					else
						tok->_links = next_link;
					DeleteForwardLink(link);
					_num_links--;
					link = next_link; // advance link but leave prev_link the same.
				}
				else
				{ // keep the link and update the tok_extra_cost if needed.
					if (link_extra_cost < 0.0) 
					{ // this is just a precaution.
						if (link_extra_cost < -0.01)
						{
							LOG_WARN << "Negative extra_cost: " << link_extra_cost ;
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
			if (tok_extra_cost > _config._lattice_beam)
				tok_extra_cost = FLOAT_INF;
			// to be pruned in PruneTokensForFrame

			if(fabs(tok->_extra_cost - tok_extra_cost) > delta)
				changed = true;
			tok->_extra_cost = tok_extra_cost; // will be +infinity or <= lattice_beam_.
		}
	} // while changed
}

// FinalizeDecoding() is a version of PruneActiveTokens that we call
// (optionally) on the final frame.  Takes into account the final-prob of
// tokens.  This function used to be called PruneActiveTokensFinal().
void MemOptimizeClgLatticeFasterOnlineDecoder::FinalizeDecoding() 
{
	int final_frame_plus_one = NumFramesDecoded();
#ifdef DEBUG
	int num_toks_begin = _num_toks;
#endif
	// PruneForwardLinksFinal() prunes final frame (with final-probs), and
	// sets _decoding_finalized.
	PruneForwardLinksFinal();
	for(int f = final_frame_plus_one - 1; f >= 0; --f)
	{
		bool b1, b2; // values not used.
		float dontcare = 0.0; // delta of zero means we must always update
	    PruneForwardLinks(f, &b1, &b2, dontcare);
	    PruneTokensForFrame(f + 1);
	}
	PruneTokensForFrame(0);
#ifdef DEBUG
	VLOG(2) << "pruned tokens from " << num_toks_begin
		<< " to " << _num_toks ;
#endif
}


// Outputs an FST corresponding to the raw, state-level
// tracebacks.
bool MemOptimizeClgLatticeFasterOnlineDecoder::GetRawLattice(Lattice *ofst,
		bool use_final_probs)
{
	typedef LatticeArc Arc;
//typedef Arc::StateId StateId;
	typedef Arc::Weight Weight;
//	typedef Arc::Label Label;

	// Note: you can't use the old interface (Decode()) if you want to
	// get the lattice with use_final_probs = false.  You'd have to do
	// InitDecoding() and then AdvanceDecoding().
	if(_decoding_finalized && !use_final_probs)
		LOG_ERR << "You cannot call FinalizeDecoding() and then call "
			<< "GetRawLattice() with use_final_probs == false";

	unordered_map<Token*, float> final_costs_local;

	const unordered_map<Token*, float> &final_costs = 
		(_decoding_finalized ? _final_costs : final_costs_local);

	if (!_decoding_finalized && use_final_probs)
	{
		PruneActiveTokens(_config._lattice_beam * _config._prune_scale);
		ComputeFinalCosts(&final_costs_local, NULL, NULL);
	}
	ofst->DeleteStates();
	// num-frames plus one (since frames are one-based, and we have
	// an extra frame for the start-state).
	int num_frames = _active_toks.size() - 1;
	LOG_ASSERT(num_frames > 0);

	const int bucket_count = _num_toks/2+3;
	unordered_map<Token*, ClgTokenStateId> tok_map(bucket_count);
	// First create all states.
	std::vector<Token*> token_list;
	for(int f = 0; f <= num_frames; ++f)
	{
		if(_active_toks[f]._toks == NULL)
		{
			LOG_WARN << "GetRawLattice: no tokens active on frame " << f
				<< ": not producing lattice.\n";
			return false;
		}
		TopSortTokens(_active_toks[f]._toks, &token_list);
		for(size_t i = 0; i < token_list.size(); ++i)
		{
			if(token_list[i] != NULL)
				tok_map[token_list[i]] = ofst->AddState();
		}
	}
	// The next statement sets the start state of the output FST.  Because we
	// topologically sorted the tokens, state zero must be the start-state.
	ofst->SetStart(0);

#ifdef DEBUG
	VLOG(2) << "init:" << _num_toks/2 + 3 << " buckets:"
		<< tok_map.bucket_count() << " load:" << tok_map.load_factor()
		<< " max:" << tok_map.max_load_factor() ;
#endif

	// Now create all arcs.
	for (int f = 0; f <= num_frames; ++f)
	{
		for (Token *tok = _active_toks[f]._toks; tok != NULL; tok = tok->_next)
		{
			ClgTokenStateId cur_state = tok_map[tok];
			for (ForwardLink *l = tok->_links; l != NULL; l = l->_next)
			{
				unordered_map<Token*, ClgTokenStateId>::const_iterator iter =
					tok_map.find(l->_next_tok);
				ClgTokenStateId nextstate = iter->second;
				LOG_ASSERT(iter != tok_map.end());
				float cost_offset = 0.0;
				/* don't use _cost_offsets
				if (l->_ilabel != 0) 
				{ // emitting
					LOG_ASSERT(f >= 0 && f < _cost_offsets.size());
					cost_offset = _cost_offsets[f];
				}
				*/
				Arc arc(l->_ilabel, l->_olabel, nextstate, 
						Weight(l->_graph_cost, l->_acoustic_cost - cost_offset));
				ofst->AddArc(cur_state, arc);
			}

			if(f == num_frames)
			{
				if(use_final_probs && !final_costs.empty())
				{
					unordered_map<Token*, float>::const_iterator iter =
						final_costs.find(tok);
					if (iter != final_costs.end())
						ofst->SetFinal(cur_state);
				}
				else
				{
					ofst->SetFinal(cur_state);
				}
			}
		}
	}
	return (ofst->NumStates() > 0);
}

void MemOptimizeClgLatticeFasterOnlineDecoder::TopSortTokens(Token *tok_list,
		std::vector<Token*> *topsorted_list)
{
	unordered_map<Token*, int> token2pos;
	typedef unordered_map<Token*, int>::iterator IterType;
	int num_toks = 0;
	for(Token *tok = tok_list; tok != NULL; tok = tok->_next)
		++num_toks;

	int cur_pos = 0;
	// We assign the tokens numbers num_toks - 1, ... , 2, 1, 0.
	// This is likely to be in closer to topological order than
	// if we had given them ascending order, because of the way
	// new tokens are put at the front of the list.
	for (Token *tok = tok_list; tok != NULL; tok = tok->_next)
		token2pos[tok] = num_toks - ++cur_pos;
	
	unordered_set<Token*> reprocess;

	for(IterType iter = token2pos.begin(); iter != token2pos.end(); ++iter)
	{
		Token *tok = iter->first;
	    int pos = iter->second;
		for(ForwardLink *link = tok->_links; link != NULL; link = link->_next)
		{
			if(link->_ilabel == 0)
			{
				// We only need to consider epsilon links, since non-epsilon links
				// transition between frames and this function only needs to sort a list
				// of tokens from a single frame.
				IterType following_iter = token2pos.find(link->_next_tok);
				if(following_iter != token2pos.end())
				{   // another token on this frame,
					// so must consider it.
					int next_pos = following_iter->second;
					if (next_pos < pos)
					{ // reassign the position of the next Token.
						following_iter->second = cur_pos++;
						reprocess.insert(link->_next_tok);
					}
				}
				else
				{
					LOG_ASSERT("this shouldn't append.");
				}
			}
		}
		// In case we had previously assigned this token to be reprocessed, we can
		// erase it from that set because it's "happy now" (we just processed it).
		reprocess.erase(tok);
	}

	size_t max_loop = 1000000, loop_count; // max_loop is to detect epsilon cycles.

	for (loop_count = 0; !reprocess.empty() && loop_count < max_loop; ++loop_count)
   	{
		std::vector<Token*> reprocess_vec;
		for (unordered_set<Token*>::iterator iter = reprocess.begin();
				iter != reprocess.end(); ++iter)
		{
			reprocess_vec.push_back(*iter);
		}
		reprocess.clear();
		for (std::vector<Token*>::iterator iter = reprocess_vec.begin();
				iter != reprocess_vec.end(); ++iter)
		{
			Token *tok = *iter;
			int pos = token2pos[tok];
			// Repeat the processing we did above (for comments, see above).
			for (ForwardLink *link = tok->_links; link != NULL; link = link->_next)
			{
				if (link->_ilabel == 0)
				{
					IterType following_iter = token2pos.find(link->_next_tok);
					if (following_iter != token2pos.end())
					{
						int next_pos = following_iter->second;
						if(next_pos < pos)
						{
							following_iter->second = cur_pos++;
							reprocess.insert(link->_next_tok);
						}
					}
				}
			}
		}
	}
	LOG_ASSERT(loop_count < max_loop && "Epsilon loops exist in your decoding "
			"graph (this is not allowed!)");

	topsorted_list->clear();
	topsorted_list->resize(cur_pos, NULL); // create a list with NULLs in between.

	for (IterType iter = token2pos.begin(); iter != token2pos.end(); ++iter)
		(*topsorted_list)[iter->second] = iter->first;
}

// Outputs an FST corresponding to the single best path through the lattice.
bool MemOptimizeClgLatticeFasterOnlineDecoder::GetBestPath(Lattice *ofst, 
		bool use_final_probs) const
{
	ofst->DeleteStates();
	float final_graph_cost;
	BestPathIterator iter = BestPathEnd(use_final_probs, &final_graph_cost);
	if (iter.Done())
		return false;  // would have printed warning.
	StateId state = ofst->AddState();
	ofst->SetFinal(state);
	while (!iter.Done())
	{
		LatticeArc arc;
		iter = TraceBackBestPath(iter, &arc);
		arc._to = state;
		StateId new_state = ofst->AddState();
		ofst->AddArc(new_state, arc);
		state = new_state;
	}
	ofst->SetStart(state);
	return true;

}

MemOptimizeClgLatticeFasterOnlineDecoder::BestPathIterator MemOptimizeClgLatticeFasterOnlineDecoder::BestPathEnd(
		bool use_final_probs, float *final_cost_out) const
{
	if (_decoding_finalized && !use_final_probs)
		LOG_ERR << "You cannot call FinalizeDecoding() and then call "
			<< "BestPathEnd() with use_final_probs == false";

	if (NumFramesDecoded() <= 0)
	{
		LOG_WARN << "You cannot call BestPathEnd if no frames were decoded.";
		return BestPathIterator(NULL, 0);
	}
	//LOG_ASSERT(NumFramesDecoded() > 0 &&
	//		"You cannot call BestPathEnd if no frames were decoded.");
	unordered_map<Token*, float> final_costs_local;

	const unordered_map<Token*, float> &final_costs =
		(_decoding_finalized ? _final_costs : final_costs_local);
	if (!_decoding_finalized && use_final_probs)
		ComputeFinalCosts(&final_costs_local, NULL, NULL);

	// Singly linked list of tokens on last frame (access list through "next"
	// pointer).
	float best_cost = FLOAT_INF;
	float best_final_cost = 0;
	Token *best_tok = NULL;
	for(Token *tok = _active_toks.back()._toks; tok != NULL; tok = tok->_next)
	{
		float cost = tok->_tot_cost, final_cost = 0.0;
		if (use_final_probs && !final_costs.empty())
		{
			// if we are instructed to use final-probs, and any final tokens were
			// active on final frame, include the final-prob in the cost of the token.
			unordered_map<Token*, float>::const_iterator iter = final_costs.find(tok);
			if (iter != final_costs.end())
			{
				final_cost = iter->second;
				cost += final_cost;
			}
			else
			{
				cost = FLOAT_INF;
			}
		}
		if (cost < best_cost)
		{
			best_cost = cost;
			best_tok = tok;
			best_final_cost = final_cost;
		}
	}
	if (best_tok == NULL)
	{
		// this should not happen, and is likely a code error or
		// caused by infinities in likelihoods, but I'm not making
		// it a fatal error for now.
		LOG_WARN << "No final token found." ;
	}
	if (final_cost_out)
		*final_cost_out = best_final_cost;
	return BestPathIterator(best_tok, NumFramesDecoded() - 1);
}

MemOptimizeClgLatticeFasterOnlineDecoder::BestPathIterator MemOptimizeClgLatticeFasterOnlineDecoder::TraceBackBestPath(
		BestPathIterator iter, LatticeArc *oarc) const
{
	LOG_ASSERT(!iter.Done() && oarc != NULL);
	Token *tok = static_cast<Token*>(iter.tok);
	int cur_t = iter.frame, ret_t = cur_t;
	if (tok->_backpointer != NULL)
	{
		ForwardLink *link;
		for (link = tok->_backpointer->_links;
				link != NULL; link = link->_next)
		{
			if (link->_next_tok == tok)
			{// this is the link to "tok"
				oarc->_input = link->_ilabel;
				oarc->_output = link->_olabel;
				float graph_cost = link->_graph_cost,
					  acoustic_cost = link->_acoustic_cost;
				if (link->_ilabel != 0)
				{
					ret_t--;
				}
				oarc->_w = LatticeWeight( graph_cost, acoustic_cost);
				break;
			}
		}
		if(link == NULL)
		{ // Did not find correct link.
			LOG_WARN << "Error tracing best-path back (likely "
				<< "bug in token-pruning algorithm)" ;
		}
	}
	else
	{
		oarc->_input = 0;
		oarc->_output = 0;
		oarc->_w = LatticeWeight::One();; // zero costs.
	}
	return BestPathIterator(tok->_backpointer, ret_t);
}

MemOptimizeClgLatticeFasterOnlineDecoder::Token *MemOptimizeClgLatticeFasterOnlineDecoder::NewToken(
		float tot_cost, float extra_cost, ForwardLink *links,
		Token *next, Token *backpointer)
{
	if(_token_pool_head == NULL)
	{
		Token *tok = new Token[_allocate_block_size];
		for(size_t i=0; i+1<_allocate_block_size; ++i)
			tok[i]._next = tok+i+1;
		tok[_allocate_block_size-1]._next = NULL;
		_token_pool_head = tok;
		_token_pool_size += _allocate_block_size;
		_token_pools.push_back(tok);
	}
	Token *tok = _token_pool_head;
	_token_pool_head = _token_pool_head->_next;
	_token_pool_size--;
	tok->_tot_cost = tot_cost;
	tok->_extra_cost = extra_cost;
	tok->_links = links;
	tok->_next = next;
	tok->_backpointer = backpointer;
	return tok;
}

MemOptimizeClgLatticeFasterOnlineDecoder::ForwardLink *MemOptimizeClgLatticeFasterOnlineDecoder::NewForwardLink(Token *next_tok, Label ilabel, Label olabel,
		float graph_cost, float acoustic_cost, ForwardLink *next)
{
	if(_link_pool_head == NULL)
	{
		ForwardLink *link = new ForwardLink[_allocate_block_size];
		for(size_t i=0; i+1<_allocate_block_size; ++i)
			link[i]._next = link+i+1;
		link[_allocate_block_size-1]._next = NULL;
		_link_pool_head = link;
		_link_pool_size += _allocate_block_size;
		_link_pools.push_back(link);
	}
	ForwardLink *link = _link_pool_head;
	_link_pool_head = _link_pool_head->_next;
	_link_pool_size--;
	link->_next_tok = next_tok;
	link->_ilabel = ilabel;
	link->_olabel = olabel;
	link->_graph_cost = graph_cost;
	link->_acoustic_cost = acoustic_cost;
	link->_next = next;
	return link;
}

void MemOptimizeClgLatticeFasterOnlineDecoder::DeleteToken(Token *tok)
{
	tok->_next = _token_pool_head;
	_token_pool_head = tok;
	_token_pool_size++;
}

void MemOptimizeClgLatticeFasterOnlineDecoder::DeleteForwardLink(ForwardLink *link)
{
	link->_next = _link_pool_head;
	_link_pool_head = link;
	_link_pool_size++;
}

void MemOptimizeClgLatticeFasterOnlineDecoder::DeletePool()
{
	ClearActiveTokens();
	for(size_t i=0;i<_token_pools.size();++i)
	{
		delete []_token_pools[i];
		_token_pool_size -= _allocate_block_size;
	}
	_token_pools.clear();
	_token_pool_head = NULL;
	for(size_t i=0;i<_link_pools.size();++i)
	{
		delete []_link_pools[i];
		_link_pool_head -= _allocate_block_size;
	}
	_link_pools.clear();
	_link_pool_head = NULL;
	LOG_ASSERT(_token_pool_size == 0 && _link_pool_head == 0);
}

#include "src/util/namespace-end.h"

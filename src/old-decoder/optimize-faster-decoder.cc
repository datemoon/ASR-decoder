
#include <stdio.h>
#include <assert.h>
#include "src/nnet/nnet-nnet.h"
#include "src/decoder/optimize-faster-decoder.h"

#include "src/util/namespace-start.h"

FasterDecoder::FasterDecoder(Fst *graph,
		const FasterDecoderOptions *opts):
	_graph(graph), _config(opts), _num_frames_decoded(-1) 
{
	assert(_config->_hash_ratio >= 1.0);  // less doesn't make much sense.
  	assert(_config->_max_active > 1);
	assert(_config->_min_active >= 0 && _config->_min_active < _config->_max_active);
//  	_prev_toks.reserve(1000);  // just so on the first frame we do something reasonable.
//  	_cur_toks.reserve(1000);  // just so on the first frame we do something reasonable.
}


void FasterDecoder::InitDecoding()
{
  	// clean up from last time:
  	ClearToks();
  	_cur_toks.clear();
  	_prev_toks.clear();
  	StateId start_state = _graph->Start();
  
  	StdArc dummy_arc(0,0,start_state,0.0);
  	Token *start_tok = new Token(dummy_arc, NULL);
  
  	_cur_toks[start_state] = start_tok;
  	_num_frames_decoded = 0;
  	ProcessNonemitting(std::numeric_limits<float>::max());
}


void FasterDecoder::Decode(AmInterface *decodable)
{
  	InitDecoding();
  	while (!decodable->ExamineFrame(_num_frames_decoded - 1))
   	{
		double weight_cutoff = ProcessEmitting(decodable);
		ProcessNonemitting(weight_cutoff);
  	}
}

void FasterDecoder::AdvanceDecoding(AmInterface *decodable,
		int max_num_frames) 
{
  	assert(_num_frames_decoded >= 0 &&
			"You must call InitDecoding() before AdvanceDecoding()");
  	int num_frames_ready = decodable->NumFramesReady();
  	// num_frames_ready must be >= num_frames_decoded, or else
	// the number of frames ready must have decreased (which doesn't
	// make sense) or the decodable object changed between calls
  	// (which isn't allowed).
  	assert(num_frames_ready >= _num_frames_decoded);
  	int target_frames_decoded = num_frames_ready;
  	if (max_num_frames >= 0)
		target_frames_decoded = std::min(target_frames_decoded,
				_num_frames_decoded + max_num_frames);
  	while (_num_frames_decoded < target_frames_decoded)
   	{
		if(decodable->LogLikelihood(_num_frames_decoded, 1) > 20)
		{
			_num_frames_decoded++;
			continue;
		}
#ifdef DEBUGTOKEN
		std::cerr << "frame:" << _num_frames_decoded << " total token " << _total_toks << std::endl;
#endif
		// note: ProcessEmitting() increments num_frames_decoded_
		double weight_cutoff = ProcessEmitting(decodable);
		ProcessNonemitting(weight_cutoff);
		ClearToks(_prev_toks);
  	}    
}


bool FasterDecoder::ReachedFinal()
{
  	for(auto it = _cur_toks.begin(); it != _cur_toks.end();++it)
   	{
		if (it->second->_tot_cost != std::numeric_limits<double>::infinity() &&
				_graph->IsFinal(it->first) == true)
	  		return true;
  	}
  	return false;
}

bool FasterDecoder::GetBestPath(vector<int> &words_arr, vector<int>&phone_arr,
		bool use_final_probs)
{
	words_arr.clear();
	Token *best_tok = NULL;
	bool is_final = ReachedFinal();
	if(is_final == true)
	{
		for(auto it = _cur_toks.begin(); it != _cur_toks.end();++it)
		{
			if((best_tok == NULL || *best_tok > *(it->second)) 
					&& _graph->IsFinal(it->first) == true)
				best_tok = it->second;
		}
	}
	else
	{
		for(auto it = _cur_toks.begin(); it != _cur_toks.end();++it)
			if(best_tok == NULL || *best_tok > *(it->second))
				best_tok = it->second;
	}
	if(best_tok == NULL)
		return false; // no output

	// traverse the best path , save the best result.
	for (Token *tok = best_tok; tok != NULL; tok = tok->_prev)
	{
		int wordid = 0;
		int phoneid = 0;
		phoneid = tok->GetIlabel();
		wordid = tok->GetOlabel();
//		fprintf(stderr,"%d %d\n",phoneid,wordid);
		if(phoneid != 0)
			phone_arr.push_back(phoneid);
		if(wordid != 0)
			words_arr.push_back(wordid);
	}

	// words_arr invert order
	for(int start = 0,end = words_arr.size()-1; start < end ; ++start,--end)
	{
		 words_arr[start] = words_arr[start] ^ words_arr[end];
		 words_arr[end] = words_arr[start] ^ words_arr[end];
		 words_arr[start] = words_arr[start] ^ words_arr[end];
	}

	for(int start = 0,end = phone_arr.size()-1; start < end ; ++start,--end)
	{
		 phone_arr[start] = phone_arr[start] ^ phone_arr[end];
		 phone_arr[end] = phone_arr[start] ^ phone_arr[end];
		 phone_arr[start] = phone_arr[start] ^ phone_arr[end];
	}
	
	return true;
}
/*
bool FasterDecoder::GetBestPath(fst::MutableFst<LatticeArc> *fst_out,
		bool use_final_probs) 
{
  	// GetBestPath gets the decoding output.  If "use_final_probs" is true
  	// AND we reached a final state, it limits itself to final states;
   	// otherwise it gets the most likely token not taking into
  	// account final-probs.  fst_out will be empty (Start() == kNoStateId) if
  	// nothing was available.  It returns true if it got output (thus, fst_out
  	// will be nonempty).
  	fst_out->DeleteStates();
  	Token *best_tok = NULL;
  	bool is_final = ReachedFinal();
  	if (!is_final)
   	{
		for (const Elem *e = toks_.GetList(); e != NULL; e = e->tail)
	  		if (best_tok == NULL || *best_tok < *(e->val) )
				best_tok = e->val;
  	}
   	else
   	{
		double infinity =  std::numeric_limits<double>::infinity(),
        best_cost = infinity;
    for (const Elem *e = toks_.GetList(); e != NULL; e = e->tail) {
      double this_cost = e->val->cost_ + fst_.Final(e->key).Value();
      if (this_cost < best_cost && this_cost != infinity) {
        best_cost = this_cost;
        best_tok = e->val;
      }
    }
  }
  if (best_tok == NULL) return false;  // No output.

  std::vector<LatticeArc> arcs_reverse;  // arcs in reverse order.

  for (Token *tok = best_tok; tok != NULL; tok = tok->prev_) {
    BaseFloat tot_cost = tok->cost_ -
        (tok->prev_ ? tok->prev_->cost_ : 0.0),
        graph_cost = tok->arc_.weight.Value(),
        ac_cost = tot_cost - graph_cost;
    LatticeArc l_arc(tok->arc_.ilabel,
                     tok->arc_.olabel,
                     LatticeWeight(graph_cost, ac_cost),
                     tok->arc_.nextstate);
    arcs_reverse.push_back(l_arc);
  }
  KALDI_ASSERT(arcs_reverse.back().nextstate == fst_.Start());
  arcs_reverse.pop_back();  // that was a "fake" token... gives no info.

  StateId cur_state = fst_out->AddState();
  fst_out->SetStart(cur_state);
  for (ssize_t i = static_cast<ssize_t>(arcs_reverse.size())-1; i >= 0; i--) {
    LatticeArc arc = arcs_reverse[i];
    arc.nextstate = fst_out->AddState();
    fst_out->AddArc(cur_state, arc);
    cur_state = arc.nextstate;
  }
  if (is_final && use_final_probs) {
    Weight final_weight = fst_.Final(best_tok->arc_.nextstate);
    fst_out->SetFinal(cur_state, LatticeWeight(final_weight.Value(), 0.0));
  } else {
    fst_out->SetFinal(cur_state, LatticeWeight::One());
  }
  RemoveEpsLocal(fst_out);
  return true;
}

*/
// Gets the weight cutoff.  Also counts the active tokens.
double FasterDecoder::GetCutoff(unordered_map<StateId, Token*> &toks, size_t *tok_count,
		float *adaptive_beam, Token **best_tok, StateId *best_stateid) 
{
  	double best_cost = std::numeric_limits<double>::infinity();
  	size_t count = 0;
  	if (_config->_max_active == std::numeric_limits<int>::max() &&
	  		_config->_min_active == 0)
   	{
		for (unordered_map<StateId, Token*>::const_iterator it = toks.begin(); it != toks.end(); ++it, count++)
	   	{
	  		double w = it->second->_tot_cost;
			if (w < best_cost) 
			{
				best_cost = w;
				if (best_tok) 
				{
					*best_tok = it->second;
					*best_stateid = it->first;
				}
	  		}
		}
		if (tok_count != NULL) 
			*tok_count = count;
		if (adaptive_beam != NULL)
		   	*adaptive_beam = _config->_beam;
		return best_cost + _config->_beam;
  	}
   	else 
	{
		_tmp_array.clear();
		for (unordered_map<StateId, Token*>::const_iterator it = toks.begin(); it != toks.end(); ++it, count++) 
		{
	  		double w = it->second->_tot_cost;
	  		_tmp_array.push_back(w);
	  		if (w < best_cost)
		   	{
				best_cost = w;
				if (best_tok)
			   	{
					*best_tok = it->second;
					*best_stateid = it->first;
				}
	  		}
		}
		if (tok_count != NULL) *tok_count = count;
		double beam_cutoff = best_cost + _config->_beam,
	   		   min_active_cutoff = std::numeric_limits<double>::infinity(),
	   		   max_active_cutoff = std::numeric_limits<double>::infinity();
    
		if (_tmp_array.size() > static_cast<size_t>(_config->_max_active)) 
		{
	  		std::nth_element(_tmp_array.begin(),
					_tmp_array.begin() + _config->_max_active,
					_tmp_array.end());
	  		max_active_cutoff = _tmp_array[_config->_max_active];
		}
		if (max_active_cutoff < beam_cutoff) 
		{ // max_active is tighter than beam.
	  		if (adaptive_beam)
				*adaptive_beam = max_active_cutoff - best_cost + _config->_beam_delta;
	  		return max_active_cutoff;
		}    
		if (_tmp_array.size() > static_cast<size_t>(_config->_min_active))
	   	{
	  		if (_config->_min_active == 0) 
				min_active_cutoff = best_cost;
	  		else
		   	{
				std::nth_element(_tmp_array.begin(),
						_tmp_array.begin() + _config->_min_active,
						_tmp_array.size() > static_cast<size_t>(_config->_max_active) ?
						_tmp_array.begin() + _config->_max_active :
						_tmp_array.end());
				min_active_cutoff = _tmp_array[_config->_min_active];
	  		}
		}
		if (min_active_cutoff > beam_cutoff)
	   	{ // min_active is looser than beam.
	  		if (adaptive_beam)
				*adaptive_beam = min_active_cutoff - best_cost + _config->_beam_delta;
	  		return min_active_cutoff;
		}
	   	else
	   	{
	  		*adaptive_beam = _config->_beam;
	  		return beam_cutoff;
		}
  	}
}

// ProcessEmitting returns the likelihood cutoff used.
double FasterDecoder::ProcessEmitting(AmInterface *decodable)
{
  	int frame = _num_frames_decoded;
  
  	_prev_toks.clear();
   	_cur_toks.swap(_prev_toks);

  	size_t tok_cnt;
  	float adaptive_beam;
  	Token *best_tok = NULL;
  	StateId best_stateid = 0;
  	double cur_cutoff = GetCutoff(_prev_toks, &tok_cnt,
			&adaptive_beam, &best_tok, &best_stateid);
#ifdef TOKEN_ACTIVE_PRINT
  	fprintf(stderr,"%ld tokens active.\n",tok_cnt);
#endif
  	// This is the cutoff we use after adding in the log-likes (i.e.
  	// for the next frame).  This is a bound on the cutoff we will use
  	// on the next frame.
   	double next_weight_cutoff = std::numeric_limits<double>::infinity();
  
  	// First process the best token to get a hopefully
  	// reasonably tight bound on the next cutoff.
  	if (best_tok)
   	{
	 	StateId stateid = best_stateid;
		Token *tok = best_tok;
		StdState *state = (StdState *)_graph->GetState((unsigned)stateid);
		for (int i = 0; i < static_cast<int>(state->GetArcSize()); ++i)
	   	{
	  		const StdArc *arc = state->GetArc((unsigned)i);
	  		if (arc->_input != 0) 
			{  // we'd propagate..
				float ac_cost = - decodable->LogLikelihood(frame, arc->_input);
				double new_weight = arc->_w + tok->_tot_cost + ac_cost;
				if (new_weight + adaptive_beam < next_weight_cutoff)
		  			next_weight_cutoff = new_weight + adaptive_beam;
	  		}
		}
		if(next_weight_cutoff == std::numeric_limits<double>::infinity())
		{
			next_weight_cutoff = tok->_tot_cost + adaptive_beam;
		}
  	}
#ifdef DEBUGTOKEN
	std::cerr << "current cutoff " << cur_cutoff << " next cutoff "<< next_weight_cutoff << std::endl;
#endif
	// int n = 0, np = 0;
  	// the tokens are now owned here, in last_toks, and the hash is empty.
  	// 'owned' is a complex thing here; the point is we need to call TokenDelete
  	// on each elem 'e' to let toks_ know we're done with them.
  	for (unordered_map<StateId, Token*>::const_iterator it = _prev_toks.begin(); it != _prev_toks.end(); ++it) 
	{  // loop this way
		// n++;
		// because we delete "e" as we go.
		StateId stateid = it->first;
		Token *tok = it->second;
		if (tok->_tot_cost < cur_cutoff)
	   	{  // not pruned.
	  		// np++;
			assert(stateid == tok->_arc._to);
			StdState *state = (StdState *)_graph->GetState((unsigned)stateid);
			for (int i = 0; i < static_cast<int>(state->GetArcSize()); ++i) 
			{
				const StdArc *arc = state->GetArc((unsigned)i);
				if (arc->_input != 0)
				{  // propagate..
					float ac_cost =  - decodable->LogLikelihood(frame, arc->_input);
					double new_weight = arc->_w + tok->_tot_cost + ac_cost;
					if (new_weight < next_weight_cutoff) 
					{  // not pruned..
						Token *new_tok = new Token(*arc, ac_cost, tok);
						if (new_weight + adaptive_beam < next_weight_cutoff)
							next_weight_cutoff = new_weight + adaptive_beam;

						unordered_map<StateId, Token*>::iterator iter = _cur_toks.find(arc->_to);
						if (iter == _cur_toks.end()) { // not find
							_cur_toks[arc->_to] = new_tok;
						} 
						else
						{ // find the same state
							if ( *(iter->second) > *new_tok ) 
							{
								Token::TokenDelete(iter->second);
								iter->second = new_tok;
							}
							else 
							{
								Token::TokenDelete(new_tok);
							}
						}
					}
				}
			}// state all arc 
		}
		else
		{// here can delete tok.
			;
		}
  	} // all token
  	_num_frames_decoded++;
  	return next_weight_cutoff;
}

// TODO: first time we go through this, could avoid using the queue.
void FasterDecoder::ProcessNonemitting(double cutoff) 
{
  	// Processes nonemitting arcs for one frame. 
   	assert(_queue.empty() && "it's serious");
  	for (auto it = _cur_toks.begin(); it != _cur_toks.end();++it)
		_queue.push_back(it->first);
//	printf("frame = %d\n",_num_frames_decoded);
  	while (!_queue.empty()) 
	{
		StateId stateid = _queue.back();
		_queue.pop_back();
		Token *tok = _cur_toks[stateid];  // would segfault if state not
		// in toks_ but this can't happen.
		if (tok->_tot_cost > cutoff)
		{ // Don't bother processing successors.
	  		continue;
		}
		assert(tok != NULL && stateid == tok->_arc._to);
		
		StdState *state = (StdState *)_graph->GetState((unsigned)stateid);
		assert(state != NULL);
		for (int i = 0; i < static_cast<int>(state->GetArcSize()); ++i) 
		{
			const StdArc *arc = state->GetArc((unsigned)i);
			if (arc->_input == 0)  // eps arc
		   	{  // propagate..
				Token *new_tok = new Token(*arc,  tok);
				if (new_tok->_tot_cost > cutoff) 
				{  // prune
					Token::TokenDelete(new_tok);
				}
				else
				{
					unordered_map<StateId, Token*>::iterator iter = _cur_toks.find(arc->_to);
					if (iter == _cur_toks.end()) 
					{ // not find
						_cur_toks[arc->_to] = new_tok;
						_queue.push_back(arc->_to);
					} 
					else
				   	{ // find the same state
						if ( *(iter->second) > *new_tok ) 
						{
							Token::TokenDelete(iter->second);
							iter->second = new_tok;
							_queue.push_back(arc->_to);
		  				}
					   	else 
						{
							Token::TokenDelete(new_tok);
						}
					}
				}
			}
		}//	state all arc 
  	} // _queue empty.
}

void FasterDecoder::ClearToks() 
{
#ifdef TOKEN_ACTIVE_PRINT
	printf("cleartok before %d\n",GetTotalTok());
#endif
  	for (auto it = _cur_toks.begin(); it != _cur_toks.end();++it) 
	{
		Token::TokenDelete(it->second);
  	}
#ifdef TOKEN_ACTIVE_PRINT
	printf("cleartok after %d\n",GetTotalTok());
#endif
}

void FasterDecoder::ClearToks(unordered_map<StateId, Token*> &prev_toks)
{
#ifdef TOKEN_ACTIVE_PRINT
	printf("cleartok before %d\n",GetTotalTok());
#endif
	for(auto it = prev_toks.begin(); it != prev_toks.end();++it)
	{
		Token::TokenDelete(it->second);
	}
#ifdef TOKEN_ACTIVE_PRINT
	printf("cleartok after %d\n",GetTotalTok());
#endif
}
#include "src/util/namespace-end.h"

#ifndef __ONLINE_CLG_DECODER_MEMPOOL_BASE_H__
#define __ONLINE_CLG_DECODER_MEMPOOL_BASE_H__

#include "src/decoder/online-decoder-mempool-base.h"

#include "src/util/namespace-start.h"
// This is the same decoder strategy,
// but use mempool control token and forwardlink
// new and delete space.
template<typename FST, typename Token = StdToken>
class OnlineClgLatticeDecoderMempoolBase:
	public OnlineLatticeDecoderMempoolBase<FST, Token>
{
public:
	using ForwardLinkT = ForwardLink<Token>;
	typedef typename HashList<StateId, Token*>::Elem Elem;

	OnlineClgLatticeDecoderMempoolBase(FST *fst,
			const LatticeFasterDecoderConfig &config):
		OnlineLatticeDecoderMempoolBase<FST, Token>(fst, config) 
	{
	   LOG_COM << "--- Construct OnlineClgLatticeDecoderMempoolBase class ---";
	}

	~OnlineClgLatticeDecoderMempoolBase(){ }
	virtual BaseFloat ProcessEmitting(AmInterface *decodable);
	virtual void ProcessNonemitting(BaseFloat cutoff);
protected:
};
template <typename FST, typename Token>
BaseFloat OnlineClgLatticeDecoderMempoolBase<FST, Token>::ProcessEmitting(AmInterface *decodable)
{
	LOG_ASSERT(this->_active_toks.size() > 0 && "it's serious bug.");

	int nnetframe = this->_num_frames_decoded;
	int frame = this->_active_toks.size() - 1;
	this->_active_toks.resize(this->_active_toks.size() + 1);

//	ClgTokenStateId tot_state = this->_graph->TotState();
//	Label blkid = decodable->GetBlockTransitionId();

	Elem *final_toks = this->_toks.Clear(); // analogous to swapping prev_toks_ / cur_toks_

	Elem *best_elem = NULL;
	float adaptive_beam;
	size_t tok_cnt = 0;

	// through prev frame token score , calculate current cutoff value.
	// first , get currnet frame cut off , function PruneActiveTokens 
	// will use this cut off.

	float cur_cutoff = this->GetCutoff(final_toks, &tok_cnt, &adaptive_beam, &best_elem);
	VLOG_COM(2) << "Adaptive beam on frame " << this->NumFramesDecoded() 
		<< "nnet frame " << nnetframe 
		<< " is " << adaptive_beam ;
	this->PossiblyResizeHash(tok_cnt);  // This makes sure the hash is always big enough.

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
		//StdState *state = this->_graph->GetState(stateid);
		if(this->_graph->StateIdInClg(stateid))
		{ // in clg fst
			//for(int i = 0; i < static_cast<int>(state->GetArcSize()); ++i)
			//{
			//	StdArc *clgarc = state->GetArc((unsigned)i);
			for(ArcIterator<FST> iter(this->_graph->GetState(stateid)); !iter.Done(); iter.Next())
			{
				const StdArc *clgarc = &iter.Value();
				if(clgarc->_input != 0)
				{
					ClgTokenStateId hmmstateid = this->_graph->MapClgTokenStateId(stateid, clgarc);
					//StdState *hmmstate = this->_graph->GetState(hmmstateid);
					// in hmm propagate
					//for(int j = 0; j < static_cast<int>(hmmstate->GetArcSize()); ++j)
					for(ArcIterator<FST> hmmiter(this->_graph->GetState(hmmstateid)); !hmmiter.Done(); hmmiter.Next())
					{
						const StdArc *arc = &hmmiter.Value();
						if (arc->_input != 0)
						{ // propagate
							float tot_score =  tok->_tot_cost + clgarc->_w.Value() + arc->_w.Value() - decodable->LogLikelihood(nnetframe, arc->_input);
							if(tot_score + adaptive_beam < next_cutoff)
								next_cutoff = tot_score + adaptive_beam;
						}
					}
				}
			}
		}
		else
		{ // in hmm fst
			//for(int i = 0; i < static_cast<int>(state->GetArcSize()); ++i)
			//{
			//	StdArc *arc = state->GetArc((unsigned)i);
			for(ArcIterator<FST> iter(this->_graph->GetState(stateid)); !iter.Done(); iter.Next())
			{
				const StdArc *arc = &iter.Value();
				if (arc->_input != 0)
				{ // propagate
					float tot_score =  tok->_tot_cost + arc->_w.Value() - decodable->LogLikelihood(nnetframe, arc->_input);
					if(tot_score + adaptive_beam < next_cutoff)
						next_cutoff = tot_score + adaptive_beam;
				}
			}
		}
	}

	VLOG_COM(2) << "frame is : " << frame << " -> current cutoff " << cur_cutoff << " next cutoff " << next_cutoff ;
	VLOG_COM(2) << "tokens is " << this->_num_toks  << " links is " << this->_num_links ;
	VLOG_COM(2) << "current cutoff " << cur_cutoff << " next cutoff " << next_cutoff ;
	// the tokens are now owned here, in _prev_toks.
	//for(auto it = _prev_toks.begin(); it != _prev_toks.end(); ++it)
	for(Elem *e = final_toks, *e_tail = NULL; e != NULL; e = e_tail)
	{
		ClgTokenStateId stateid = e->key;
		Token *tok = e->val;
		if(tok->_tot_cost < cur_cutoff)
		{// it's OK.
			//StdState *state = this->_graph->GetState((unsigned)stateid);
			if(this->_graph->StateIdInClg(stateid))
			{ // in clg fst
				//for(int i = 0; i < static_cast<int>(state->GetArcSize()); ++i)
				//{
				//	StdArc *clgarc = state->GetArc((unsigned)i);
				for(ArcIterator<FST> iter(this->_graph->GetState(stateid)); !iter.Done(); iter.Next())
				{
					const StdArc *clgarc = &iter.Value();
					if(clgarc->_input != 0)
					{
						ClgTokenStateId hmmstateid = this->_graph->MapClgTokenStateId(stateid, clgarc);
						//StdState *hmmstate = this->_graph->GetState(hmmstateid);
						// hmm propagate
						//for(int j = 0; j < static_cast<int>(hmmstate->GetArcSize()); ++j)
						//{
						//	StdArc *arc = hmmstate->GetArc((unsigned)j);
						for(ArcIterator<FST> hmmiter(this->_graph->GetState(hmmstateid)); !hmmiter.Done(); hmmiter.Next())
						{
							const StdArc *arc = &hmmiter.Value();
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
								ClgTokenStateId arrivestateid = this->_graph->MapClgTokenStateId(hmmstateid, arc);
								Elem *next_tok = this->FindOrAddToken(arrivestateid,
										frame + 1, tot_cost, tok, NULL);
								// Add ForwardLink from tok to next_tok (put on head of list tok->links)
								tok->_links = this->NewForwardLink(next_tok->val, arc->_input, clgarc->_output,
										graph_cost, ac_cost, tok->_links);
							}
						}
					}
				} // all clg arc
			}
			else
			{ // in hmm state
				//for(int i = 0; i < static_cast<int>(state->GetArcSize()); ++i)
				//{
				//	const StdArc *arc = state->GetArc((unsigned)i);
				for(ArcIterator<FST> iter(this->_graph->GetState(stateid)); !iter.Done(); iter.Next())
				{
					const StdArc *arc = &iter.Value();
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
						ClgTokenStateId arrivestateid = this->_graph->MapClgTokenStateId(stateid, arc);
						Elem *next_tok = this->FindOrAddToken(arrivestateid,
								frame + 1, tot_cost, tok, NULL);
						// Add ForwardLink from tok to next_tok (put on head of list tok->links)
						tok->_links = this->NewForwardLink(next_tok->val, arc->_input, arc->_output,
								graph_cost, ac_cost, tok->_links);
					}
				}
			}// for all arc.
		} // this state ok
		e_tail = e->tail;
		this->_toks.Delete(e); // e give back hash
	} // end propagate
//	_prev_toks.clear();
	this->_num_frames_decoded++;
	return next_cutoff;
}

template <typename FST, typename Token>
void OnlineClgLatticeDecoderMempoolBase<FST, Token>::ProcessNonemitting(float cutoff)
{
	LOG_ASSERT(!this->_active_toks.empty() && "_active_toks shouldn't be empty.");
	int frame = static_cast<int>(this->_active_toks.size()) - 1;
	// frame is the time-index, if frame=1 , it's the first frame.
	// call from InitDecoding()
	
	// Processes nonemitting arcs for one frame.  Propagates within toks_.
	// Note-- this queue structure is not very optimal as
	// it may cause us to process states unnecessarily (e.g. more than once),
	// but I think this problem did not reduce speed.
	LOG_ASSERT(this->_queue.empty() && "_queue must be empty.");

	//for(auto it = _cur_toks.begin(); it != _cur_toks.end();++it)
	for(const Elem *e = this->_toks.GetList(); e!= NULL; e = e->tail)
	{
		StateId state = e->key;
        if(this->_graph->NumInputEpsilons(state) != 0)
			this->_queue.push_back(e);
	}

	while (!this->_queue.empty())
	{
		const Elem *e= this->_queue.back();
		this->_queue.pop_back();

		ClgTokenStateId stateid = e->key;
		Token *tok = e->val; // would segfault if state not in toks_ but this can't happen.
		//Token *tok = _cur_toks[stateid];
		float cur_cost = tok->_tot_cost;
		if(cur_cost >= cutoff) // Don't process
			continue;

		// If "tok" has any existing forward links, delete them,
		// because we're about to regenerate them.  This is a kind
		// of non-optimality (remember, this is the simple decoder),
		// but since most states are emitting it's not a huge issue.
		// because this state can be possible arrive from an eps arc.
		this->DeleteForwardLinks(tok); // necessary when re-visiting
// up need process ,but now don't change it.
// all code is ok,I will optomize.
		//StdState *state = this->_graph->GetState((unsigned)stateid);
		//LOG_ASSERT(state != NULL && "can not find this state.");

		//for(int i=0; i < static_cast<int>(state->GetArcSize()); ++i)
		//{
		//	const StdArc *arc = state->GetArc((unsigned)i);
		for(ArcIterator<FST> iter(this->_graph, stateid); !iter.Done(); iter.Next())
		{
			const StdArc *arc = &iter.Value();
			if(arc->_input == 0)
			{ // propagate eps edge
				float graph_cost = arc->_w.Value();
				float tot_cost = cur_cost + graph_cost;
				if(tot_cost < cutoff)
				{
					bool changed = false;
					//StateId nextstate = iter.NextState();
					ClgTokenStateId nextstate = this->_graph->MapClgTokenStateId(stateid, arc);
					Elem *new_tok = this->FindOrAddToken(nextstate, frame, tot_cost,
							tok, &changed);
					tok->_links = this->NewForwardLink(new_tok->val, 0, arc->_output,
							graph_cost, 0, tok->_links);
					// if add node have exist and the tot_cost > this tot_cost
					// this situation don't add this node to _queue.
					if(changed && this->_graph->NumInputEpsilons(nextstate) != 0)
						this->_queue.push_back(new_tok);
				}
			}
		}// for all arcs in this node.
	}// while queue not empty.
}
typedef OnlineClgLatticeDecoderMempoolBase<ClgFst, StdToken> OnlineClgLatticeDecoderMempool;

#include "src/util/namespace-end.h"

#endif

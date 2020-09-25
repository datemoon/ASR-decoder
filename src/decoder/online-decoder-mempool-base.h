#ifndef __ONLINE_DECODER_MEMPOOL_H__
#define __ONLINE_DECODER_MEMPOOL_H__

#include "src/decoder/online-decoder-base.h"
#include "src/util/mem-pool.h"

#include "src/util/namespace-start.h"
// This is the same decoder strategy,
// but use mempool control token and forwardlink
// new and delete space.
template<typename FST, typename Token = StdToken>
class OnlineLatticeDecoderMempoolBase:
	public OnlineLatticeDecoderBase<FST, Token>
{
public:
	using ForwardLinkT = ForwardLink<Token>;

	OnlineLatticeDecoderMempoolBase(FST *fst,
			const LatticeFasterDecoderConfig &config):
		OnlineLatticeDecoderBase<FST, Token>(fst, config) 
	{
	   LOG_COM << "--- Construct OnlineLatticeDecoderMempoolBase clss ---";
	}

	virtual ~OnlineLatticeDecoderMempoolBase()
	{
		this->ClearActiveTokens();
	}
protected:
	// add memory pool save token and forwardlink.
	MemPool<Token> _token_pools;
	MemPool<ForwardLinkT> _link_pools;

	inline virtual Token *NewToken(BaseFloat tot_cost, 
			BaseFloat extra_cost, ForwardLinkT *links,
			Token *next, Token *backpointer)
	{
		Token *tok = _token_pools.NewT();
		tok->_tot_cost = tot_cost;
		tok->_extra_cost = extra_cost;
		tok->_links = links;
		tok->_next = next;
		tok->_backpointer = backpointer;
		this->_num_toks++;
		return tok;
	}

	inline virtual ForwardLinkT *NewForwardLink(Token *next_tok,
			Label ilabel, Label olabel, BaseFloat graph_cost,
			BaseFloat acoustic_cost, ForwardLinkT *next)
	{
		ForwardLinkT *link = _link_pools.NewT();
		link->_next_tok = next_tok;
		link->_ilabel = ilabel;
		link->_olabel = olabel;
		link->_graph_cost = graph_cost;
		link->_acoustic_cost = acoustic_cost;
		link->_next = next;
		this->_num_links++;
		return link;
	}

	inline virtual void DeleteToken(Token *tok)
	{
		_token_pools.DeleteT(tok);
		this->_num_toks--;
	}

	inline virtual void DeleteForwardLink(ForwardLinkT *link)
	{
		_link_pools.DeleteT(link);
		this->_num_links--;
	}

};

typedef OnlineLatticeDecoderMempoolBase<Fst, StdToken> OnlineLatticeDecoderMempool;

//typedef OnlineLatticeDecoderMempoolBase<ClgFst, StdToken> OnlineClgLatticeDecoderMempool;

#include "src/util/namespace-end.h"

#endif

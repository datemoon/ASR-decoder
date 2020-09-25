#ifndef __ONLINE_DECODER_BASE_H__
#define __ONLINE_DECODER_BASE_H__

#include <assert.h>
#include <vector>
#include <unordered_map>
#include <limits>
#include <algorithm>
#include "src/itf/decodable-itf.h"
#include "src/decoder/decoder-itf.h"
#include "src/util/util-common.h"
#include "src/newfst/optimize-fst.h"
#include "src/newfst/lattice-fst.h"
#include "src/decoder/lattice-faster-decoder-conf.h"

using namespace std;
using std::unordered_map;

#include "src/util/namespace-start.h"

#ifndef FLOAT_INF
#define FLOAT_INF std::numeric_limits<BaseFloat>::infinity()
#endif

// ForwardLinks are the links from a token to a token on the next frame.
// or sometimes on the current frame (for input-epsilon links).
// For big lm ,use template<typename Token>
template<typename Token>
struct ForwardLink
{
	Token *_next_tok;      // the next token [or NULL if represents final-state]
	Label _ilabel;         // ilabel on arc.
	Label _olabel;         // olabel on arc.
	BaseFloat _graph_cost; // graph cost of traversing arc (contains LM, etc.)
	BaseFloat _acoustic_cost; // acoustic cost (pre-scaled) of traversing arc
	ForwardLink *_next; //next in singly-linked list of forward arcs from a token. 
						// _next it must be have ,because MemPool is singly linked list and need next point.
	
	// because MemPool so this constructor is necessary.
	inline ForwardLink():_next_tok(NULL), _ilabel(0), _olabel(0), _graph_cost(0), _acoustic_cost(0){ }

	inline ForwardLink(Token *next_tok, Label ilabel, Label olabel,
			BaseFloat graph_cost, BaseFloat acoustic_cost,
			ForwardLink *next):
		_next_tok(next_tok), _ilabel(ilabel), _olabel(olabel),
		_graph_cost(graph_cost), _acoustic_cost(acoustic_cost),
		_next(next) { }
};

// Token is what's resident in a particular state at a particular time.
// In this decoder a Token actually contains *forward* links.
// When first created, a Token just has the (total) cost.    We add forward
// links from it when we process the next frame.
struct StdToken
{
	using ForwardLinkT = ForwardLink<StdToken>;
	using Token = StdToken;
	BaseFloat _tot_cost; // arrive current state the total score,possible have a lot of arc arrive state,but it's the best one.
	BaseFloat _extra_cost; // >= 0.  This is used in pruning a way tokens.
	// there is a comment in lattice-faster-decoder.cc explaining this;
	// search for "a note on the definition of extra_cost".

	ForwardLinkT *_links; // from current state forward arc record.
	Token *_next; // Next in list of tokens for this frame.
				  // _next it must be have ,because MemPool is singly linked list and need next point.

	Token *_backpointer; // best preceding Token (could be on this frame or a
                         // previous frame).  This is only required for an
                         // efficient GetBestPath function, it plays no part in
                         // the lattice generation (the "links" list is what
                         // stores the forward links, for that).
	// because MemPool so this constructor is necessary.
	inline StdToken():
	   	_tot_cost(0.0), _extra_cost(0.0), _links(NULL), 
		_next(NULL), _backpointer(NULL) { }

	inline StdToken(BaseFloat tot_cost, BaseFloat extra_cost, ForwardLinkT *links,
			Token *next, Token *backpointer):
		_tot_cost(tot_cost), _extra_cost(extra_cost),
		_links(links), _next(next), _backpointer(backpointer) { }

	bool CheckNoLink() { return _links == NULL ? true : false; }

};

// because FST have some type, I will use HCLG and CLG fst ,so FST is template.
template<typename FST, typename Token = StdToken>
class OnlineLatticeDecoderBase:public DecoderItf
{
public:
	typedef typename HashList<StateId, Token*>::Elem Elem;
	using ForwardLinkT = ForwardLink<Token>;
	//using Elem = typename HashList<StateId, Token*>::Elem;
public:
	OnlineLatticeDecoderBase(FST *fst, const LatticeFasterDecoderConfig &config);
	virtual ~OnlineLatticeDecoderBase();

	struct BestPathIterator
	{
		void *tok;
		int32 frame;
		// note, "frame" is the frame-index of the frame you'll get the
		// transition-id for next time, if you call TraceBackBestPath on this
		// iterator (assuming it's not an epsilon transition).  Note that this
		// is one less than you might reasonably expect, e.g. it's -1 for
		// the nonemitting transitions before the first frame.
		BestPathIterator(void *t, int32 f): tok(t), frame(f) { }
		bool Done() { return tok == NULL; }
	};

	// InitDecoding initializes the decoding, and should only be used if you
	// intend to call AdvanceDecoding().  If you call Decode(), you don't need to
	// call this.  You can also call InitDecoding if you have already decoded an
	// utterance and want to start with a new utterance.
	virtual void InitDecoding();

	// This will decode until there are no more frames ready in the decodable
	// object.  You can keep calling it each time more frames become available.
	// If max_num_frames is specified, it specifies the maximum number of frames
	// the function will decode before returning.
	void AdvanceDecoding(AmInterface *decodable,
			int32 max_num_frames = -1);

	// This function may be optionally called after AdvanceDecoding(), when you
	// do not plan to decode any further.  It does an extra pruning step that
    // will help to prune the lattices output by GetLattice and (particularly)
    // GetRawLattice more accurately, particularly toward the end of the
    // utterance.  It does this by using the final-probs in pruning (if any
    // final-state survived); it also does a final pruning step that visits all
	// states (the pruning that is done during decoding may fail to prune states
    // that are within kPruningScale = 0.1 outside of the beam).  If you call
    // this, you cannot call AdvanceDecoding again (it will fail), and you
    // cannot call GetLattice() and related functions with use_final_probs =
    // false.
	void FinalizeDecoding();

	// Returns the number of frames decoded so far.  The value returned changes
	// whenever we call ProcessEmitting().
	inline int32 NumFramesDecoded() const { return _active_toks.size() - 1; }

	// Processes emitting arcs for one frame.  Propagates from prev_toks_ to cur_toks_.
	// Returns the cost cutoff for subsequent ProcessNonemitting() to use.
	virtual BaseFloat ProcessEmitting(AmInterface *decodable);

	// Processes nonemitting (epsilon) arcs for one frame.  Called after
	// ProcessEmitting() on each frame.  The cost cutoff is computed by the
	// preceding ProcessEmitting().
	// You must be make sure fst is arc sort.
	virtual void ProcessNonemitting(BaseFloat cost_cutoff);

	// Decodes until there are no more frames left in the "decodable" object..
	// note, this may block waiting for input if the "decodable" object blocks.
	// Returns true if any kind of traceback is available (not necessarily from a
	// final state).
	bool Decode(AmInterface *decodable);

	// Outputs an FST corresponding to the single best path through the lattice.
	// This is quite efficient because it doesn't get the entire raw lattice and find
	// the best path through it; insterad, it uses the BestPathEnd and BestPathIterator
	// so it basically traces it back through the lattice.
	// Returns true if result is nonempty (using the return status is deprecated,
	// it will become void).  If "use_final_probs" is true AND we reached the
	// final-state of the graph then it will include those as final-probs, else
	bool GetBestPath(Lattice *ofst, bool use_final_probs = true);

	// Outputs an FST corresponding to the raw, state-level
	// tracebacks.  Returns true if result is nonempty.
	// If "use_final_probs" is true AND we reached the final-state
	// of the graph then it will include those as final-probs, else
	// it will treat all final-probs as one.
	// The raw lattice will be topologically sorted.
	bool GetRawLattice(Lattice *ofst,
			bool use_final_probs = true);

	// [Deprecated, users should now use GetRawLattice and determinize it
	// themselves, e.g. using DeterminizeLatticePhonePrunedWrapper].
	// Outputs an FST corresponding to the lattice-determinized
	// lattice (one path per word sequence).   Returns true if result is nonempty.
	// If "use_final_probs" is true AND we reached the final-state of the graph
	// then it will include those as final-probs, else it will treat all
	// final-probs as one.
	bool GetLattice(Lattice *ofst,
			bool use_final_probs = true) const;
protected:
	// before ProcessEmitting, call GetCutoff, get this frame cutoff.
	virtual BaseFloat GetCutoff(Elem *list_head, size_t *tok_count,
			BaseFloat *adaptive_beam, Elem **best_elem);

	// Go backwards through still-alive tokens, pruning them, starting not from
	// the current frame (where we want to keep all tokens) but from the frame before
	//  that.  We go backwards through the frames and stop when we reach a point
	//  where the delta-costs are not changing (and the delta controls when we consider
	// a cost to have "not changed").
	void PruneActiveTokens(BaseFloat delta);

	// Prune away any tokens on this frame that have no forward links.
	// [we don't do this in PruneForwardLinks because it would give us
	// a problem with dangling pointers].
	// It's called by PruneActiveTokens if any forward links have been pruned
	void PruneForwardLinks(int32 frame_plus_one, bool *extra_costs_changed,
			bool *prev_link_pruned, BaseFloat delta);

	// PruneForwardLinksFinal is a version of PruneForwardLinks that we call
	// on the final frame.  If there are final tokens active, it uses
	// the final-probs for pruning, otherwise it treats all tokens as final.
	virtual void PruneForwardLinksFinal();

	// Prune away any tokens on this frame that have no forward links.
	// [we don't do this in PruneForwardLinks because it would give us
	// a problem with dangling pointers].
	// It's called by PruneActiveTokens if any forward links have been pruned
	void PruneTokensForFrame(int frame_plus_one);

	// every InitDecoder must be call.
	void ClearActiveTokens();

	// This function computes the final-costs for tokens active on the final
	// frame.  It outputs to final-costs, if non-NULL, a map from the Token*
	// pointer to the final-prob of the corresponding state, for all Tokens
	// that correspond to states that have final-probs.  This map will be
	// empty if there were no final-probs.  It outputs to
	// final_relative_cost, if non-NULL, the difference between the best
	// forward-cost including the final-prob cost, and the best forward-cost
	// without including the final-prob cost (this will usually be positive), or
	// infinity if there were no final-probs.  [c.f. FinalRelativeCost(), which
	// outputs this quanitity].  It outputs to final_best_cost, if
	// non-NULL, the lowest for any token t active on the final frame, of
	// forward-cost[t] + final-cost[t], where final-cost[t] is the final-cost in
	// the graph of the state corresponding to token t, or the best of
	// forward-cost[t] if there were no final-probs active on the final frame.
	// You cannot call this after FinalizeDecoding() has been called; in that
	// case you should get the answer from class-member variables.
	virtual void ComputeFinalCosts( unordered_map<Token*, BaseFloat> *final_costs,
			BaseFloat *final_relative_cost, BaseFloat *final_best_cost);

	// This function takes a singly linked list of tokens for a single frame, and
	// outputs a list of them in topological order (it will crash if no such order
	// can be found, which will typically be due to decoding graphs with epsilon
	// cycles, which are not allowed).  Note: the output list may contain NULLs,
	// which the caller should pass over; it just happens to be more efficient for
	// the algorithm to output a list that contains NULLs.
	static void TopSortTokens(Token *tok_list,
			std::vector<Token*> *topsorted_list);

	// FindOrAddToken either locates a token in hash of _cur_toks,
	virtual Elem *FindOrAddToken(StateId stateid, int32 frame_plus_one, BaseFloat tot_cost,
			Token *backpointer, bool *changed);


	void PossiblyResizeHash(size_t num_toks);
protected:
	struct TokenList
	{
		Token *_toks;
		bool _must_prune_forward_links;
		bool _must_prune_tokens;
		TokenList(): 
			_toks(NULL), _must_prune_forward_links(true),
			_must_prune_tokens(true) { }
	};

	// At a frame ,a token can be indexed by StateId.
	HashList<StateId, Token *> _toks;

	// Lists of tokens, indexed by
	// frame (members of TokenList are toks, must_prune_forward_links,
	// must_prune_tokens).
	std::vector<TokenList> _active_toks; 

	std::vector<const Elem *> _queue; // temp variable used in ProcessNonemitting ,save activate token.

	std::vector<BaseFloat> _tmp_array;  // used in GetCutoff.

	// _graph is a pointer to the FST we are decoding from.
	FST *_graph;
	// delete_fst_ is true if the pointer _graph needs to be deleted when this
	// object is destroyed.
	bool _delete_fst;

	// This contains, for each
	// frame, an offset that was added to the acoustic log-likelihoods on that
	// frame in order to keep everything in a nice dynamic range i.e.  close to
	// zero, to reduce roundoff errors.
	std::vector<BaseFloat> _cost_offsets; // now I don't use _cost_offsets

	LatticeFasterDecoderConfig _config; // decoder config

	int32 _num_toks; // total tokens
	int32 _num_links; // total forwardlinks

	bool _warned;

	// _decoding_finalized is true if someone called FinalizeDecoding().  [note,
	// calling this is optional].  If true, it's forbidden to decode more.  Also,
	// if this is set, then the output of ComputeFinalCosts() is in the next
	// three variables.  The reason we need to do this is that after
	// FinalizeDecoding() calls PruneTokensForFrame() for the final frame, some
	// of the tokens on the last frame are freed, so we free the list from toks_
	// to avoid having dangling pointers hanging around.
	bool _decoding_finalized;

	// ComputeFinalCosts() can be use these parameters.
	unordered_map<Token*, BaseFloat> _final_costs;
	BaseFloat _final_relative_cost;
	BaseFloat _final_best_cost;

	// Keep track of the number of frames decoded in the current file.
	// beacuse I can skip block frame , so add this true frame number.
	int32 _num_frames_decoded;

protected: // best path function
	// This function returns an iterator that can be used to trace back
	// the best path.  If use_final_probs == true and at least one final state
	// survived till the end, it will use the final-probs in working out the best
	// final Token, and will output the final cost to *final_cost (if non-NULL),
	// else it will use only the forward likelihood, and will put zero in
	// *final_cost (if non-NULL).
	// Requires that NumFramesDecoded() > 0.
	BestPathIterator BestPathEnd(bool use_final_probs,
			BaseFloat *final_cost = NULL);

	// This function can be used in conjunction with BestPathEnd() to trace back
	// the best path one link at a time (e.g. this can be useful in endpoint
	// detection).  By "link" we mean a link in the graph; not all links cross
	// frame boundaries, but each time you see a nonzero ilabel you can interpret
	// that as a frame.  The return value is the updated iterator.  It outputs
	// the ilabel and olabel, and the (graph and acoustic) weight to the "arc" pointer,
	// while leaving its "nextstate" variable unchanged.
	BestPathIterator TraceBackBestPath(
			BestPathIterator iter, LatticeArc *arc) const;

protected: // new and delete token or forward link

	inline virtual Token *NewToken(BaseFloat tot_cost, BaseFloat extra_cost, ForwardLinkT *links,
			Token *next, Token *backpointer)
	{
		Token *tok = new Token(tot_cost, extra_cost, links, next, backpointer);
		_num_toks++;
		return tok;
	}

	inline virtual ForwardLinkT *NewForwardLink(Token *next_tok,
			Label ilabel, Label olabel, BaseFloat graph_cost,
			BaseFloat acoustic_cost, ForwardLinkT *next)
	{
		ForwardLinkT *link = new ForwardLinkT(next_tok, ilabel, olabel,
			   	graph_cost, acoustic_cost, next);
		_num_links++;
		return link;
	}

	inline virtual void DeleteToken(Token *tok)
	{
		delete tok;
		_num_toks--;
	}

	inline virtual void DeleteForwardLink(ForwardLinkT *link)
	{
		delete link;
		_num_links--;
	}

	// Deletes the elements of the singly linked list tok->links.
	inline void DeleteForwardLinks(Token *tok);
};

#include "src/util/namespace-end.h"

#include "src/decoder/online-decoder-base-inl.h"
#endif

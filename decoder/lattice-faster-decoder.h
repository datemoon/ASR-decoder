#ifndef __LATTICE_FASTER_DECODE_H__
#define __LATTICE_FASTER_DECODE_H__

#include <assert.h>
#include <vector>
#include <unordered_map>
#include <limits>
#include <algorithm>
#include "nnet/nnet-nnet.h"
#include "decoder/optimize-fst.h"
#include "fst/lattice-fst.h"
#include "util/config-parse-options.h"

using namespace std;
using std::unordered_map;

#ifndef FLOAT_INF
#define FLOAT_INF std::numeric_limits<float>::infinity()
#endif

//typedef int Label;
//typedef int Stateid;
struct LatticeFasterDecoderConfig 
{
	float _beam;
	int _max_active;
	int _min_active;
	float _lattice_beam;
	int _prune_interval;
	bool _determinize_lattice; // no use

	float _beam_delta;
	float _hash_ratio;
	float _prune_scale; // Note: we don't make this configurable on the command line,
						// it's not a very important parameter.  It affects the
						// algorithm that prunes the tokens as we go.
	LatticeFasterDecoderConfig(): 
		_beam(16.0),
		_max_active(std::numeric_limits<int>::max()),
		_min_active(200),
		_lattice_beam(10.0),
		_prune_interval(25),
		_determinize_lattice(true),
		_beam_delta(0.5),
		_hash_ratio(2.0),
		_prune_scale(0.1) { }

	void Register(ConfigParseOptions *conf)
	{
		conf->Register("beam", &_beam, "Decoding beam.  Larger->slower, more accurate.");
		conf->Register("max-active", &_max_active, "Decoder max active states.  Larger->slower; more accurate");
		conf->Register("min-active", &_min_active, "Decoder minimum #active states.");
		conf->Register("lattice-beam", &_lattice_beam, "Lattice generation beam.  Larger->slower, and deeper lattices");
		conf->Register("prune-interval", &_prune_interval, "Interval (in frames) at which to prune tokens");
		conf->Register("determinize-lattice", &_determinize_lattice, "If true, "
				"determinize the lattice (lattice-determinization, keeping only "
				"best pdf-sequence for each word-sequence).");
		conf->Register("beam-delta", &_beam_delta, "Increment used in decoding-- this "
				"parameter is obscure and relates to a speedup in the way the "
				"max-active constraint is applied.  Larger is more accurate.");
		conf->Register("hash-ratio", &_hash_ratio, "Setting used in decoder to "
				"control hash behavior");
	}
	void Check() const 
	{
		assert(_beam > 0.0 && _max_active > 1 && _lattice_beam > 0.0
				&& _prune_interval > 0 && _beam_delta > 0.0 && _hash_ratio >= 1.0
				&& _prune_scale > 0.0 && _prune_scale < 1.0);
	}
};

class LatticeFasterDecoder 
{
private:
	struct Token;
public:

	LatticeFasterDecoder(Fst *graph,
			const LatticeFasterDecoderConfig &config):
		_cur_toks(1000), _prev_toks(1000),
		_graph(graph), _delete_fst(false), _config(config),
		_num_toks(0), _num_links(0),_num_frames_decoded(0)
	{
		_config.Check();
	}
	
	~LatticeFasterDecoder()
	{
		ClearActiveTokens();
		if(_delete_fst)
			delete _graph;
	}
	// Processes emitting arcs for one frame.  Propagates from prev_toks_ to cur_toks_.
	// Returns the cost cutoff for subsequent ProcessNonemitting() to use.
	float ProcessEmitting(AmInterface *decodable);

	// Processes nonemitting (epsilon) arcs for one frame.  Called after
	// ProcessEmitting() on each frame.  The cost cutoff is computed by the
	// preceding ProcessEmitting().
	void ProcessNonemitting(float cost_cutoff);

	/// InitDecoding initializes the decoding, and should only be used if you
	/// intend to call AdvanceDecoding().  If you call Decode(), you don't need to
	/// call this.  You can also call InitDecoding if you have already decoded an
	/// utterance and want to start with a new utterance.
	void InitDecoding();

	/// This will decode until there are no more frames ready in the decodable
	/// object.  You can keep calling it each time more frames become available.
	/// If max_num_frames is specified, it specifies the maximum number of frames
	/// the function will decode before returning.
	void AdvanceDecoding(AmInterface *decodable,
			int max_num_frames = -1);

	float GetCutoff();

	void PruneActiveTokens(float delta);

	bool Decode(AmInterface *decodable);

	/// This function may be optionally called after AdvanceDecoding(), when you
	/// do not plan to decode any further.  It does an extra pruning step that
	/// will help to prune the lattices output by GetLattice and (particularly)
	/// GetRawLattice more accurately, particularly toward the end of the
	/// utterance.  It does this by using the final-probs in pruning (if any
	/// final-state survived); it also does a final pruning step that visits all
	/// states (the pruning that is done during decoding may fail to prune states
	/// that are within kPruningScale = 0.1 outside of the beam).  If you call
	/// this, you cannot call AdvanceDecoding again (it will fail), and you
	/// cannot call GetLattice() and related functions with use_final_probs =
	/// false.
	void FinalizeDecoding();

	// get best path
	bool GetBestPath(Lattice &best_path, 
			vector<int> &best_words_arr, vector<int> &best_phones_arr,
			float &best_tot_score, float &best_lm_score);

	// Returns the number of frames decoded so far.  The value returned changes
	// whenever we call ProcessEmitting().
	inline int NumFramesDecoded() const { return _active_toks.size() - 1; }
	
	/// Outputs an FST corresponding to the raw, state-level
	/// tracebacks.  Returns true if result is nonempty.
	/// If "use_final_probs" is true AND we reached the final-state
	/// of the graph then it will include those as final-probs, else
	/// it will treat all final-probs as one.
	/// The raw lattice will be topologically sorted.
	bool GetRawLattice(Lattice *ofst,
			bool use_final_probs) const;
private:
	Token * FindOrAddToken(StateId stateid,int frame_plus_one, float tot_cost, bool *changed);

	// Prune away any tokens on this frame that have no forward links.
	// [we don't do this in PruneForwardLinks because it would give us
	// a problem with dangling pointers].
	// It's called by PruneActiveTokens if any forward links have been pruned
	void PruneForwardLinks(int frame_plus_one, bool *extra_costs_changed,
		   	bool *prev_link_pruned, float delta);

	void PruneForwardLinksFinal();


	void PruneTokensForFrame(int frame_plus_one);

	inline float MinFloat(float A,float B)
	{
		return A < B ? A : B;
	}

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
	void ComputeFinalCosts( unordered_map<Token*, float> *final_costs,
			float *final_relative_cost,	float *final_best_cost) const ;


	// This function takes a singly linked list of tokens for a single frame, and
	// outputs a list of them in topological order (it will crash if no such order
	// can be found, which will typically be due to decoding graphs with epsilon
	// cycles, which are not allowed).  Note: the output list may contain NULLs,
	// which the caller should pass over; it just happens to be more efficient for
	// the algorithm to output a list that contains NULLs.
	static void TopSortTokens(Token *tok_list,
			std::vector<Token*> *topsorted_list);
private:
	// ForwardLinks are the links from a token to a token on the next frame.
	// or sometimes on the current frame (for input-epsilon links).
	struct ForwardLink 
	{
		Token *_next_tok; // the next token [or NULL if represents final-state]
		Label _ilabel; // ilabel on arc.
		Label _olabel; // olabel on arc.
		float _graph_cost; // graph cost of traversing arc (contains LM, etc.)
		float _acoustic_cost; // acoustic cost (pre-scaled) of traversing arc
		ForwardLink *_next; //next in singly-linked list of forward arcs from a token.
		inline ForwardLink(Token *next_tok, Label ilabel, Label olabel,
				float graph_cost, float acoustic_cost,
				ForwardLink *next):
			_next_tok(next_tok), _ilabel(ilabel), _olabel(olabel),
			_graph_cost(graph_cost), _acoustic_cost(acoustic_cost),
			_next(next) { }
	};

	// Token is what's resident in a particular state at a particular time.
	// In this decoder a Token actually contains *forward* links.
	// When first created, a Token just has the (total) cost.    We add forward
	// links from it when we process the next frame.
	struct Token
	{
		float _tot_cost; // arrive current state the total score,possible have a lot of arc arrive state,but it's the best one.
		float _extra_cost; // >= 0.  This is used in pruning a way tokens.
		// there is a comment in lattice-faster-decoder.cc explaining this;
		// search for "a note on the definition of extra_cost".
	
		ForwardLink *_links; // from current state forward arc record.
		Token *_next; // Next in list of tokens for this frame.

		inline Token(float tot_cost, float extra_cost, ForwardLink *links,
				Token *next):
			_tot_cost(tot_cost), _extra_cost(extra_cost), 
			_links(links), _next(next) { }

		inline void DeleteForwardLinks(int &num_links)
		{
			ForwardLink *l = _links, *m;
			while (l != NULL)
			{
				 m = l->_next;
				 delete l;
				 num_links--;
				 l = m;
			}
			_links = NULL;
		}
		bool CheckNoLink()
		{
			return _links == NULL ? true : false;
		}
	};

	struct TokenList 
	{
		Token *_toks;
		bool _must_prune_forward_links;
		bool _must_prune_tokens;
		TokenList(): _toks(NULL), _must_prune_forward_links(true),
			 _must_prune_tokens(true) { }
	};

	// At a frame ,a token can be indexed by StateId.
	std::unordered_map<StateId,Token *> _cur_toks;
	std::unordered_map<StateId,Token *> _prev_toks;
	std::vector<TokenList> _active_toks;
	
	vector<StateId> _queue;

	std::vector<float> _tmp_array;  // used in GetCutoff.

	Fst *_graph;
	bool _delete_fst;

	LatticeFasterDecoderConfig _config;
	
	int _num_toks; // total tokens
	int _num_links; // total forwardlinks
	
	std::vector<float> _cost_offsets; // now I don't use _cost_offsets

	// next parameters for cut off .
	// record current frame best token , best stateid .
	Token *_best_tok;
	StateId _best_stateid;
	//adaptive_beam : for adapt next frame
	float _adaptive_beam;

	bool _warned;

	bool _decoding_finalized;

	// ComputeFinalCosts() can be use these parameters.
	unordered_map<Token*, float> _final_costs;
	float _final_relative_cost;
	float _final_best_cost;
	
	// Keep track of the number of frames decoded in the current file.
	// beacuse I can skip block frame , so add this true frame number.
	int _num_frames_decoded;
};

#endif

#ifndef __DECODER_FASTER_DECODER_H__
#define __DECODER_FASTER_DECODER_H__

#include <assert.h>
#include <unordered_map>
#include <algorithm> 
#include <vector>
#include <limits>
#include "src/nnet/nnet-nnet.h"
#include "src/decoder/optimize-fst.h"

using namespace std;
using std::unordered_map;


#include "src/util/namespace-start.h"
static int _total_toks = 0;
struct FasterDecoderOptions 
{
  	float _beam;
  	int _max_active;
  	int _min_active;
  	float _beam_delta;
  	float _hash_ratio;
  	FasterDecoderOptions(): 
		_beam(16.0),
		_max_active(std::numeric_limits<int>::max()),
		_min_active(20), // This decoder mostly used for
                		// alignment, use small default.        
		_beam_delta(0.5),
		_hash_ratio(2.0) { }
	FasterDecoderOptions(float beam,int max_active,
			int min_active,float beam_delat,
			float hash_ratio):
		_beam(beam),
		_max_active(max_active),
		_min_active(min_active), // This decoder mostly used for
                		// alignment, use small default.        
		_beam_delta(beam_delat),
		_hash_ratio(hash_ratio) { }

	/*
  	void Register(OptionsItf *opts, bool full) 
	{  /// if "full", use obscure
		/// options too.
		/// Depends on program.
		opts->Register("beam", &_beam, "Decoding beam.  Larger->slower, more accurate.");
		opts->Register("max-active", &_max_active, "Decoder max active states.  Larger->slower; "
				"more accurate");
		opts->Register("min-active", &_min_active,
				"Decoder min active states (don't prune if #active less than this).");
		if (full) {
	  		opts->Register("beam-delta", &_beam_delta,
					"Increment used in decoder [obscure setting]");
	  		opts->Register("hash-ratio", &_hash_ratio,
					"Setting used in decoder to control hash behavior");
		}
  	}
	*/
};

class FasterDecoder 
{
public:
  	FasterDecoder(Fst *graph,
			const FasterDecoderOptions *config);

  	void SetOptions(const FasterDecoderOptions *config) { _config = config; }

  	~FasterDecoder() { ClearToks(); }

  	void Decode(AmInterface *decodable);

  	/// Returns true if a final state was active on the last frame.
  	bool ReachedFinal();


  	/// GetBestPath gets the decoding traceback. If "use_final_probs" is true
  	/// AND we reached a final state, it limits itself to final states;
  	/// otherwise it gets the most likely token not taking into account
  	/// final-probs. Returns true if the output best path was not the empty
  	/// FST (will only return false in unusual circumstances where
  	/// no tokens survived).
  	bool GetBestPath(vector<int> &words_arr , vector<int> &phone_arr, bool use_final_probs = true);


  	/// As a new alternative to Decode(), you can call InitDecoding
  	/// and then (possibly multiple times) AdvanceDecoding().
  	void InitDecoding();


  	/// This will decode until there are no more frames ready in the decodable
  	/// object, but if max_num_frames is >= 0 it will decode no more than
  	/// that many frames.
  	void AdvanceDecoding(AmInterface *decodable,
			int max_num_frames = -1);

  	/// Returns the number of frames already decoded.
  	int NumFramesDecoded() const { return _num_frames_decoded; }

	int GetTotalTok()
	{
		return _total_toks;
	}
protected:

  	class Token
   	{
	public:
		StdArc _arc; // contains only the graph part of the cost;
		// we can work out the acoustic part from difference between
		// "cost_" and prev->cost_.
	 	Token *_prev;
		int _ref_count;

		// if you are looking for weight_ here, it was removed and now we just have
		// cost_, which corresponds to ConvertToCost(weight_).
		double _tot_cost;
		inline int GetOlabel()
		{
			return _arc.GetOutput();
		}
		inline int GetIlabel()
		{
			return _arc.GetInput();
		}
	   	inline Token(const StdArc &arc, float ac_cost, Token *prev):
		  	_arc(arc), _prev(prev), _ref_count(1) 
		{
			if (prev) 
			{
				prev->_ref_count++;
			   	_tot_cost = prev->_tot_cost + arc._w + ac_cost;
	  		} 
			else 
			{
				_tot_cost = arc._w + ac_cost;
	  		}
			++_total_toks;
		}
	 	inline Token(const StdArc &arc, Token *prev):
			_arc(arc), _prev(prev), _ref_count(1) 
		{
	  		if (prev) 
			{
				prev->_ref_count++;
				_tot_cost = prev->_tot_cost + arc._w;
	  		} 
			else
		   	{
				_tot_cost = arc._w;
	  		}
			++_total_toks;
		}
		inline bool operator > (const Token &other)
	   	{
	  		return _tot_cost > other._tot_cost;
		}
/*		inline bool operator < (const Token &other)
	   	{
	  		return _tot_cost > other._tot_cost;
		}
*/
		inline static void TokenDelete(Token *tok) 
		{
	  		while (--tok->_ref_count == 0) 
			{
				Token *prev = tok->_prev;
				delete tok;
				--_total_toks;
				if (prev == NULL) return;
				else tok = prev;
	  		}
	  		assert(tok->_ref_count > 0);
		}
  	};

   	/// Gets the weight cutoff.  Also counts the active tokens.
  	double GetCutoff(unordered_map<StateId, Token*> &toks, size_t *tok_count,
  			float *adaptive_beam, Token **best_tok, StateId *best_stateid);

	void FindAndAddToken(Token *tok, StateId stateid,bool eps);

   	// ProcessEmitting returns the likelihood cutoff used.
	// It decodes the frame num_frames_decoded_ of the decodable object
	// and then increments num_frames_decoded_
   	double ProcessEmitting(AmInterface *decodable);

  	// TODO: first time we go through this, could avoid using the queue.
  	void ProcessNonemitting(double cutoff);

	// HashList defined in ../util/hash-list.h.  It actually allows us to maintain
	// more than one list (e.g. for current and previous frames), but only one of
	// them at a time can be indexed by StateId.
	unordered_map<StateId, Token*> _prev_toks;
   	unordered_map<StateId, Token*> _cur_toks;

  	Fst *_graph;
  	const FasterDecoderOptions *_config;
  	std::vector<StateId> _queue;  // temp variable used in ProcessNonemitting,
   	std::vector<float> _tmp_array;  // used in GetCutoff.
  	// make it class member to avoid internal new/delete.

	// Keep track of the number of frames decoded in the current file.
  	int _num_frames_decoded;

  	// It might seem unclear why we call ClearToks(toks_.Clear()).
   	// There are two separate cleanup tasks we need to do at when we start a new file.
	// one is to delete the Token objects in the list; the other is to delete
	// the Elem objects.  toks_.Clear() just clears them from the hash and gives ownership
	// to the caller, who then has to call toks_.Delete(e) for each one.  It was designed
	// this way for convenience in propagating tokens from one frame to the next.
	void ClearToks();

	void ClearToks(unordered_map<StateId, Token*> &prev_toks);

};

#include "src/util/namespace-end.h"
#endif

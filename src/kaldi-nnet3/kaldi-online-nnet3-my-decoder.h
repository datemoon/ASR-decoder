
#include "nnet3/decodable-online-looped.h"
#include "matrix/matrix-lib.h"
#include "util/common-utils.h"
#include "base/kaldi-error.h"
#include "itf/online-feature-itf.h"
#include "online2/online-nnet2-feature-pipeline.h"
#include "hmm/transition-model.h"
#include "hmm/posterior.h"

#include "src/decoder/mem-optimize-clg-lattice-faster-online-decoder.h"

class OnlineDecoder
{
public:

public:
	OnlineDecoder(const datemoon::LatticeFasterDecoderConfig &decoder_opts,
			datemoon::ClgFst *decoder_fst):
		_decoder_opts(decoder_opts),
	   	_decoder(decoder_fst, _conf._decoder_opts), _decodable(NULL) { }

	void InitDecoding(const nnet3::DecodableNnetSimpleLoopedInfo &info,
			const kaldi::TransitionModel &trans_model,
			OnlineNnet2FeaturePipeline *features, 
			int frame_offset=0)
	{
		if(_decodable != NULL)
			delete _decodable;
		_decodable = new kaldi::nnet3::DecodableAmNnetLoopedOnline(
				trans_model, info,
				feature->InputFeature(), 
				feature->IvectorFeature());
		decodable_->SetFrameOffset(frame_offset);
		_decoder.InitDecoding();
	}

	/// advance the decoding as far as we can.
	void AdvanceDecoding();
	
	/// Finalizes the decoding. Cleans up and prunes remaining tokens, so the
	/// GetLattice() call will return faster.  You must not call this before
	/// calling (TerminateDecoding() or InputIsFinished()) and then Wait().
	void FinalizeDecoding();

	kaldi::int32 NumFramesDecoded() const;

	/// Gets the lattice.  The output lattice has any acoustic scaling in it
	/// (which will typically be desirable in an online-decoding context); if you
	/// want an un-scaled lattice, scale it using ScaleLattice() with the inverse
	/// of the acoustic weight.  "end_of_utterance" will be true if you want the
	/// final-probs to be included.
	void GetLattice(bool end_of_utterance,
			CompactLattice *clat) const;

	/// Outputs an FST corresponding to the single best path through the current
	/// lattice. If "use_final_probs" is true AND we reached the final-state of
	/// the graph then it will include those as final-probs, else it will treat
	/// all final-probs as one.
	 void GetBestPath(bool end_of_utterance,
			 datemoon::Lattice *best_path) const;


	/// This function calls EndpointDetected from online-endpoint.h,
	/// with the required arguments.
	bool EndpointDetected(const kaldi::OnlineEndpointConfig &config);

private:
	const datemoon::LatticeFasterDecoderConfig &_decoder_opts;
	
	// this is remembered from the constructor; it's ultimately
	// derived from calling FrameShiftInSeconds() on the feature pipeline.
	BaseFloat _input_feature_frame_shift_in_seconds;
	
	// decoder
	datemoon::MemOptimizeClgLatticeFasterOnlineDecoder _decoder;
	// nnet forward
	kaldi::nnet3::DecodableAmNnetLoopedOnline *_decodable;
};

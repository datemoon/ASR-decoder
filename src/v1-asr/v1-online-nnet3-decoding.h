
#ifndef __V1_ONLINE_NNET3_DECODING_H__
#define __V1_ONLINE_NNET3_DECODING_H__

#include <string>
#include <vector>
#include <deque>

#include "nnet3/decodable-online-looped.h"
#include "matrix/matrix-lib.h"
#include "util/common-utils.h"
#include "base/kaldi-error.h"
#include "itf/online-feature-itf.h"
#include "online2/online-endpoint.h"
#include "online2/online-nnet2-feature-pipeline.h"
#include "decoder/lattice-faster-online-decoder.h"
#include "hmm/transition-model.h"
#include "hmm/posterior.h"

#include "src/v1-asr/online-nnet3-feature-pipeline-io.h"

namespace kaldi {
/// @addtogroup  onlinedecoding OnlineDecoding
/// @{


/**
   You will instantiate this class when you want to decode a single utterance
   using the online-decoding setup for neural nets.  The template will be
   instantiated only for FST = fst::Fst<fst::StdArc> and FST = fst::GrammarFst.
*/

template <typename FST>
class V1SingleUtteranceNnet3DecoderTpl {
 public:

  // Constructor. The pointer 'features' is not being given to this class to own
  // and deallocate, it is owned externally.
  V1SingleUtteranceNnet3DecoderTpl(const LatticeFasterDecoderConfig &decoder_opts,
                                 const TransitionModel &trans_model,
                                 const nnet3::DecodableNnetSimpleLoopedInfo &info,
                                 const FST &fst,
                                 OnlineNnet2FeaturePipeline *features):
	  decoder_opts_(decoder_opts),
	  input_feature_frame_shift_in_seconds_(features->FrameShiftInSeconds()),
	  trans_model_(trans_model),
	  decodable_(trans_model_, info,
			  features->InputFeature(), features->IvectorFeature()),
	  decoder_(fst, decoder_opts_) 
	{
		decoder_.InitDecoding();
	}


  V1SingleUtteranceNnet3DecoderTpl(const LatticeFasterDecoderConfig &decoder_opts,
                                 const TransitionModel &trans_model,
                                 const nnet3::DecodableNnetSimpleLoopedInfo &info,
                                 const FST &fst,
                                 OnlineNnet3FeaturePipelineIo *features):
	  decoder_opts_(decoder_opts),
	  input_feature_frame_shift_in_seconds_(features->FrameShiftInSeconds()),
	  trans_model_(trans_model),
	  decodable_(trans_model_, info,
			  features->InputFeature(), features->IvectorFeature()),
	  decoder_(fst, decoder_opts_) 
	{
		decoder_.InitDecoding();
	}

  /// Initializes the decoding and sets the frame offset of the underlying
  /// decodable object. This method is called by the constructor. You can also
  /// call this method when you want to reset the decoder state, but want to
  /// keep using the same decodable object, e.g. in case of an endpoint.
  void InitDecoding(int32 frame_offset = 0)
  {
	  decoder_.InitDecoding();
	  decodable_.SetFrameOffset(frame_offset);
  }

  /// Advances the decoding as far as we can.
  void AdvanceDecoding()
  {
	  decoder_.AdvanceDecoding(&decodable_);
  }

  /// Finalizes the decoding. Cleans up and prunes remaining tokens, so the
  /// GetLattice() call will return faster.  You must not call this before
  /// calling (TerminateDecoding() or InputIsFinished()) and then Wait().
  void FinalizeDecoding()
  {
	  decoder_.FinalizeDecoding();
  }

  int32 NumFramesDecoded() const
  {
	  return decoder_.NumFramesDecoded();
  }

  /// Gets the lattice.  The output lattice has any acoustic scaling in it
  /// (which will typically be desirable in an online-decoding context); if you
  /// want an un-scaled lattice, scale it using ScaleLattice() with the inverse
  /// of the acoustic weight.  "end_of_utterance" will be true if you want the
  /// final-probs to be included.
  void GetLattice(bool end_of_utterance,
                  CompactLattice *clat) const
  {
	  if (NumFramesDecoded() == 0)
		  KALDI_ERR << "You cannot get a lattice if you decoded no frames.";
	  Lattice raw_lat;
	  decoder_.GetRawLattice(&raw_lat, end_of_utterance);

	  if (!decoder_opts_.determinize_lattice)
		  KALDI_ERR << "--determinize-lattice=false option is not supported at the moment";

	  BaseFloat lat_beam = decoder_opts_.lattice_beam;
	  DeterminizeLatticePhonePrunedWrapper(
			  trans_model_, &raw_lat, lat_beam, clat, decoder_opts_.det_opts);
  }

  /// Outputs an FST corresponding to the single best path through the current
  /// lattice. If "use_final_probs" is true AND we reached the final-state of
  /// the graph then it will include those as final-probs, else it will treat
  /// all final-probs as one.
  void GetBestPath(bool end_of_utterance,
                   Lattice *best_path) const
  {
	  decoder_.GetBestPath(best_path, end_of_utterance);
  }


  /// This function calls EndpointDetected from online-endpoint.h,
  /// with the required arguments.
  bool EndpointDetected(const OnlineEndpointConfig &config)
  {
	  BaseFloat output_frame_shift =
		  input_feature_frame_shift_in_seconds_ *
		  decodable_.FrameSubsamplingFactor();
	  return kaldi::EndpointDetected(config, trans_model_,
			  output_frame_shift, decoder_);
  }

  const LatticeFasterOnlineDecoderTpl<FST> &Decoder() const { return decoder_; }

  ~V1SingleUtteranceNnet3DecoderTpl() { }
 private:

  const LatticeFasterDecoderConfig &decoder_opts_;

  // this is remembered from the constructor; it's ultimately
  // derived from calling FrameShiftInSeconds() on the feature pipeline.
  BaseFloat input_feature_frame_shift_in_seconds_;

  // we need to keep a reference to the transition model around only because
  // it's needed by the endpointing code.
  const TransitionModel &trans_model_;

  nnet3::DecodableAmNnetLoopedOnline decodable_;

  LatticeFasterOnlineDecoderTpl<FST> decoder_;

};

template class V1SingleUtteranceNnet3DecoderTpl<fst::Fst<fst::StdArc> >;
template class SingleUtteranceNnet3DecoderTpl<fst::ConstGrammarFst >;
template class SingleUtteranceNnet3DecoderTpl<fst::VectorGrammarFst >;
typedef V1SingleUtteranceNnet3DecoderTpl<fst::Fst<fst::StdArc> > V1SingleUtteranceNnet3Decoder;

/// @} End of "addtogroup onlinedecoding"

}  // namespace kaldi

#endif  // __V1_ONLINE_NNET3_DECODING_H__

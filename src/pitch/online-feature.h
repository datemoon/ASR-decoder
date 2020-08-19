#ifndef KALDI_FEAT_ONLINE_FEATURE_H_
#define KALDI_FEAT_ONLINE_FEATURE_H_

#include "src/pitch/kaldi-common.h"
#include "src/pitch/kaldi-matrix.h"
#include "src/pitch/kaldi-vector.h"
#include "src/pitch/kaldi-type.h"
#include "src/pitch/online-feature-itf.h"
#include "src/util/config-parse-options.h"

#ifdef NAMESPACE
using namespace datemoon;
#endif
struct DeltaFeaturesOptions {
	  int32 order;
	  int32 window;  // e.g. 2; controls window size (window size is 2*window + 1)
	  // the behavior at the edges is to replicate the first or last frame.
	  // this is not configurable.
		  
 	  DeltaFeaturesOptions(int32 order = 2, int32 window = 2):
		  order(order), window(window) { }
	  void Register(ConfigParseOptions *opts) {
		  opts->Register("delta-order", &order, "Order of delta computation");
		  opts->Register("delta-window", &window,		  
				  "Parameter controlling window for delta computation (actual window"
				  " size for each delta order is 1 + 2*delta-window-size)");
	  }
};

class DeltaFeatures {
 public:
  // This class provides a low-level function to compute delta features.
  // The function takes as input a matrix of features and a frame index
  // that it should compute the deltas on.  It puts its output in an object
  // of type VectorBase, of size (original-feature-dimension) * (opts.order+1).
  // This is not the most efficient way to do the computation, but it's
  // state-free and thus easier to understand

  explicit DeltaFeatures(const DeltaFeaturesOptions &opts);

  void Process(const MatrixBase<BaseFloat> &input_feats,
               int32 frame,
               VectorBase<BaseFloat> *output_frame) const;
 private:
  DeltaFeaturesOptions opts_;
  std::vector<Vector<BaseFloat> > scales_;  // a scaling window for each
  // of the orders, including zero: multiply the features for each
  // dimension by this window.
};
class OnlineDeltaFeature: public OnlineFeatureInterface {
 public:
  //
  // First, functions that are present in the interface:
  //
  virtual int32 Dim() const;

  virtual bool IsLastFrame(int32 frame) const {
    return src_->IsLastFrame(frame);
  }
  virtual BaseFloat FrameShiftInSeconds() const {
    return src_->FrameShiftInSeconds();
  }

  virtual int32 NumFramesReady() const;

  virtual void GetFrame(int32 frame, VectorBase<BaseFloat> *feat);

  //
  // Next, functions that are not in the interface.
  //
  OnlineDeltaFeature(const DeltaFeaturesOptions &opts,
                     OnlineFeatureInterface *src);

  virtual ~OnlineDeltaFeature() { }
 private:
  OnlineFeatureInterface *src_;  // Not owned here
  DeltaFeaturesOptions opts_;
  DeltaFeatures delta_features_;  // This class contains just a few
                                  // coefficients.
};


// ComputeDeltas is a convenience function that computes deltas on a feature
// file.  If you want to deal with features coming in bit by bit you would have
// to use the DeltaFeatures class directly, and do the computation frame by
// frame.  Later we will have to come up with a nice mechanism to do this for
// features coming in.
void ComputeDeltas(const DeltaFeaturesOptions &delta_opts,
                   const MatrixBase<BaseFloat> &input_features,
                   Matrix<BaseFloat> *output_features);



#endif

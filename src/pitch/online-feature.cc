
#include <cassert>
#include "src/pitch/online-feature.h"
#include "src/pitch/kaldi-common.h"

OnlineDeltaFeature::OnlineDeltaFeature(const DeltaFeaturesOptions &opts,
		OnlineFeatureInterface *src):
	src_(src), opts_(opts), delta_features_(opts) { }


int32 OnlineDeltaFeature::Dim() const {
  int32 src_dim = src_->Dim();
  return src_dim * (1 + opts_.order);
}

int32 OnlineDeltaFeature::NumFramesReady() const {
  int32 num_frames = src_->NumFramesReady(),
      context = opts_.order * opts_.window;
  // "context" is the number of frames on the left or (more relevant
  // here) right which we need in order to produce the output.
  if (num_frames > 0 && src_->IsLastFrame(num_frames-1))
    return num_frames;
  else
    return std::max<int32>(0, num_frames - context);
}

void OnlineDeltaFeature::GetFrame(int32 frame,
                                      VectorBase<BaseFloat> *feat) {
  assert(frame >= 0 && frame < NumFramesReady());
  assert(feat->Dim() == Dim());
  // We'll produce a temporary matrix containing the features we want to
  // compute deltas on, but truncated to the necessary context.
  int32 context = opts_.order * opts_.window;
  int32 left_frame = frame - context,
      right_frame = frame + context,
      src_frames_ready = src_->NumFramesReady();
  if (left_frame < 0) left_frame = 0;
  if (right_frame >= src_frames_ready)
    right_frame = src_frames_ready - 1;
  assert(right_frame >= left_frame);
  int32 temp_num_frames = right_frame + 1 - left_frame,
      src_dim = src_->Dim();
  Matrix<BaseFloat> temp_src(temp_num_frames, src_dim);
  for (int32 t = left_frame; t <= right_frame; t++) {
    SubVector<BaseFloat> temp_row(temp_src, t - left_frame);
    src_->GetFrame(t, &temp_row);
  }
  int32 temp_t = frame - left_frame;  // temp_t is the offset of frame "frame"
                                      // within temp_src
  delta_features_.Process(temp_src, temp_t, feat);
}




DeltaFeatures::DeltaFeatures(const DeltaFeaturesOptions &opts): opts_(opts) {
  assert(opts.order >= 0 && opts.order < 1000);  // just make sure we don't get binary junk.
  // opts will normally be 2 or 3.
  assert(opts.window > 0 && opts.window < 1000);  // again, basic sanity check.
  // normally the window size will be two.

  scales_.resize(opts.order+1);
  scales_[0].Resize(1);
  scales_[0](0) = 1.0;  // trivial window for 0th order delta [i.e. baseline feats]

  for (int32 i = 1; i <= opts.order; i++) {
    Vector<BaseFloat> &prev_scales = scales_[i-1],
        &cur_scales = scales_[i];
    int32 window = opts.window;  // this code is designed to still
    // work if instead we later make it an array and do opts.window[i-1],
    // or something like that. "window" is a parameter specifying delta-window
    // width which is actually 2*window + 1.
    assert(window != 0);
    int32 prev_offset = (static_cast<int32>(prev_scales.Dim()-1))/2,
        cur_offset = prev_offset + window;
    cur_scales.Resize(prev_scales.Dim() + 2*window);  // also zeros it.

    BaseFloat normalizer = 0.0;
    for (int32 j = -window; j <= window; j++) {
      normalizer += j*j;
      for (int32 k = -prev_offset; k <= prev_offset; k++) {
        cur_scales(j+k+cur_offset) +=
            static_cast<BaseFloat>(j) * prev_scales(k+prev_offset);
      }
    }
    cur_scales.Scale(1.0 / normalizer);
  }
}

void DeltaFeatures::Process(const MatrixBase<BaseFloat> &input_feats,
                            int32 frame,
                            VectorBase<BaseFloat> *output_frame) const {
  assert(frame < input_feats.NumRows());
  int32 num_frames = input_feats.NumRows(),
      feat_dim = input_feats.NumCols();
  assert(static_cast<int32>(output_frame->Dim()) == feat_dim * (opts_.order+1));
  output_frame->SetZero();
  for (int32 i = 0; i <= opts_.order; i++) {
    const Vector<BaseFloat> &scales = scales_[i];
    int32 max_offset = (scales.Dim() - 1) / 2;
    SubVector<BaseFloat> output(*output_frame, i*feat_dim, feat_dim);
    for (int32 j = -max_offset; j <= max_offset; j++) {
      // if asked to read
      int32 offset_frame = frame + j;
      if (offset_frame < 0) offset_frame = 0;
      else if (offset_frame >= num_frames)
        offset_frame = num_frames - 1;
      BaseFloat scale = scales(j + max_offset);
      if (scale != 0.0)
        output.AddVec(scale, input_feats.Row(offset_frame));
    }
  }
}

void ComputeDeltas(const DeltaFeaturesOptions &delta_opts,
                   const MatrixBase<BaseFloat> &input_features,
                   Matrix<BaseFloat> *output_features) {
  output_features->Resize(input_features.NumRows(),
                          input_features.NumCols()
                          *(delta_opts.order + 1));
  DeltaFeatures delta(delta_opts);
  for (int32 r = 0; r < static_cast<int32>(input_features.NumRows()); r++) {
    SubVector<BaseFloat> row(*output_features, r);
    delta.Process(input_features, r, &row);
  }
}

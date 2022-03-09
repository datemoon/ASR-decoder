// bin/copy-matrix.cc

// Copyright 2009-2011  Microsoft Corporation

// See ../../COPYING for clarification regarding multiple authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED
// WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE,
// MERCHANTABLITY OR NON-INFRINGEMENT.
// See the Apache 2 License for the specific language governing permissions and
// limitations under the License.

#include "base/kaldi-common.h"
#include "util/common-utils.h"
#include "matrix/kaldi-matrix.h"
#include "matrix/kaldi-vector.h"
#include "transform/transform-common.h"

namespace kaldi {

void ApplySoftMaxPerRow(MatrixBase<BaseFloat> *mat) {
  for (int32 i = 0; i < mat->NumRows(); i++) {
    mat->Row(i).ApplySoftMax();
  }
}

}  // namespace kaldi

int main(int argc, char *argv[]) {
  try {
    using namespace kaldi;

    const char *usage =
        "Calculate matrix row min or max value and index \n"
        "\n"
        "Usage: matrix-row-min [options] <matrix-in-rspecifier> <min-index-wspecifier> <min-value-wspecifier> [<matrix-out-wspecifier>]\n"
        " e.g.: matrix-row-min ark:2.trans ark,t:index.txt ark,t:index_mapval.txt \n";

    bool binary = true;
    bool apply_log = false;
    bool apply_exp = false;
    bool apply_softmax_per_row = false;
	bool delete_blank = true;
	bool delete_repeat = true;
	bool use_min = true;
	int32 wordid_offset = 0;
    BaseFloat apply_power = 1.0;
    BaseFloat scale = 1.0;
	int blank_id = 0;

    ParseOptions po(usage);

    po.Register("binary", &binary,
                "Write in binary mode (only relevant if output is a wxfilename)");
    po.Register("wordid-offset", &wordid_offset,
                "wordid offset");
    po.Register("use-min", &use_min,
                "if use-min=true find matrix row min,else find matrix row max");
    po.Register("delete-blank", &delete_blank,
                "delete blank dim");
    po.Register("blank-id", &blank_id,
                "delete blank id");
	po.Register("delete-repeat", &delete_repeat,
			"delete repeat dim");
    po.Register("scale", &scale,
                "This option can be used to scale the matrices being copied.");
    po.Register("apply-log", &apply_log,
                "This option can be used to apply log on the matrices. "
                "Must be avoided if matrix has negative quantities.");
    po.Register("apply-exp", &apply_exp,
                "This option can be used to apply exp on the matrices");
    po.Register("apply-power", &apply_power,
                "This option can be used to apply a power on the matrices");
    po.Register("apply-softmax-per-row", &apply_softmax_per_row,
                "This option can be used to apply softmax per row of the matrices");

    po.Read(argc, argv);

    if (po.NumArgs() < 3 || po.NumArgs() > 4) {
      po.PrintUsage();
      exit(1);
    }

    if ( (apply_log && apply_exp) || (apply_softmax_per_row && apply_exp) ||
          (apply_softmax_per_row && apply_log) )
      KALDI_ERR << "Only one of apply-log, apply-exp and "
                << "apply-softmax-per-row can be given";

    std::string matrix_in_fn = po.GetArg(1),
		min_id_wspecifier = po.GetArg(2),
		min_val_wspecifier = po.GetArg(3),
        matrix_out_fn = po.GetOptArg(4);

	Int32VectorWriter writer_index(min_id_wspecifier);
	TableWriter<BasicVectorHolder<float> > writer_value(min_val_wspecifier);
	//BaseFloatVectorWriter writer_value(min_val_wspecifier);
    // all these "fn"'s are either rspecifiers or filenames.

/*    if (!in_is_rspecifier) {
      Matrix<BaseFloat> mat;
      ReadKaldiObject(matrix_in_fn, &mat);
      if (scale != 1.0) mat.Scale(scale);
      if (apply_log) {
        mat.ApplyFloor(1.0e-20);
        mat.ApplyLog();
      }
      if (apply_exp) mat.ApplyExp();
      if (apply_softmax_per_row) ApplySoftMaxPerRow(&mat);
      if (apply_power != 1.0) mat.ApplyPow(apply_power);
      Output ko(matrix_out_fn, binary);
      mat.Write(ko.Stream(), binary);
      KALDI_LOG << "Copied matrix to " << matrix_out_fn;
      return 0;
    } else*/ {
      int num_done = 0;
      BaseFloatMatrixWriter writer(matrix_out_fn);
      SequentialBaseFloatMatrixReader reader(matrix_in_fn);
      for (; !reader.Done(); reader.Next(), num_done++) {
      //  if (scale != 1.0 || apply_log || apply_exp ||
       //       apply_power != 1.0 || apply_softmax_per_row) {
          Matrix<BaseFloat> mat(reader.Value());
		  std::string key(reader.Key());
     //     if (scale != 1.0) mat.Scale(scale);
       //   if (apply_log) {
         //   mat.ApplyFloor(1.0e-20);
        //    mat.ApplyLog();
      //    }
          if (apply_exp) mat.ApplyExp();
          //if (apply_softmax_per_row) ApplySoftMaxPerRow(&mat);
        //  if (apply_power != 1.0) mat.ApplyPow(apply_power);

		  std::vector<MatrixIndexT> min_seq;
		  std::vector<BaseFloat> min_val;
		  for(MatrixIndexT r=0; r<mat.NumRows(); ++r)
		  {
  			  SubVector<BaseFloat> subv(mat, r);
			  MatrixIndexT min_id = 0;
			  BaseFloat val = 0;
			  if(use_min == true)
  				  val = subv.Min(&min_id);
			  else
			  	  val = subv.Max(&min_id);
			  min_val.push_back(val);
			  min_seq.push_back(min_id);
		  }
		  MatrixIndexT prev_w = -1;
		  std::vector<MatrixIndexT> min_seq_final;
		  std::vector<BaseFloat> min_val_final;
		  for(ssize_t i = 0 ; i < min_seq.size(); ++i)
		  {
			  if(delete_repeat == true)
			  {
				  if(prev_w != min_seq[i])
				  {
					  prev_w = min_seq[i];
  				  }
 				  else
  				  {
					  continue;
  				  }
			  }
			  if(min_seq[i] == blank_id && delete_blank == true)
				  continue;
			  else
			  {
				  min_seq_final.push_back(min_seq[i]-wordid_offset);
				  min_val_final.push_back(min_val[i]);
			  }
		  }
		  writer_index.Write(key, min_seq_final);
		  writer_value.Write(key, min_val_final);
		  if(writer.IsOpen())
  			  writer.Write(key, mat);
       // } else {
         // writer.Write(reader.Key(), reader.Value());
       // }
      }
      KALDI_LOG << "Copied " << num_done << " matrices.";
      return (num_done != 0 ? 0 : 1);
    }
  } catch(const std::exception &e) {
    std::cerr << e.what();
    return -1;
  }
}

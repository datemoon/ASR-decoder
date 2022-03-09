// bin/latgen-faster-mapped.cc

// Copyright 2009-2012  Microsoft Corporation, Karel Vesely
//                2013  Johns Hopkins University (author: Daniel Povey)
//                2014  Guoguo Chen

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
#include "tree/context-dep.h"
#include "hmm/transition-model.h"
#include "fstext/fstext-lib.h"
#include "decoder/decoder-wrappers.h"
#include "decoder/decodable-matrix.h"
#include "base/timer.h"


int main(int argc, char *argv[]) {
  try {
    using namespace kaldi;
    typedef kaldi::int32 int32;
    using fst::SymbolTable;
    using fst::Fst;
    using fst::StdArc;

    const char *usage =
        "Generate lattices, reading log-likelihoods as matrices\n"
        " (model is needed only for the integer mappings in its transition-model)\n"
        "Usage: latgen-faster [options] (fst-in|fsts-rspecifier) loglikes-rspecifier"
        " lattice-wspecifier [ words-wspecifier [alignments-wspecifier] ]\n";
    ParseOptions po(usage);
    Timer timer;
    bool allow_partial = false;
    BaseFloat acoustic_scale = 0.1;
    LatticeFasterDecoderConfig config;

    std::string word_syms_filename;
    config.Register(&po);
    po.Register("acoustic-scale", &acoustic_scale, "Scaling factor for acoustic likelihoods");

    po.Register("word-symbol-table", &word_syms_filename, "Symbol table for words [for debug output]");
    po.Register("allow-partial", &allow_partial, "If true, produce output even if end state was not reached.");

    po.Read(argc, argv);

    if (po.NumArgs() < 4 || po.NumArgs() > 6) {
      po.PrintUsage();
      exit(1);
    }

    std::string fst_in_str = po.GetArg(1),
        feature_rspecifier = po.GetArg(2),
        lattice_wspecifier = po.GetArg(3),
        words_wspecifier = po.GetOptArg(4),
        alignment_wspecifier = po.GetOptArg(5);

   // TransitionModel trans_model;
  //  ReadKaldiObject(model_in_filename, &trans_model);

    //bool determinize = config.determinize_lattice;
    bool determinize = false;
    CompactLatticeWriter compact_lattice_writer;
    LatticeWriter lattice_writer;
    if (! (determinize ? compact_lattice_writer.Open(lattice_wspecifier)
           : lattice_writer.Open(lattice_wspecifier)))
      KALDI_ERR << "Could not open table for writing lattices: "
                 << lattice_wspecifier;

    Int32VectorWriter words_writer(words_wspecifier);

    Int32VectorWriter alignment_writer(alignment_wspecifier);

    fst::SymbolTable *word_syms = NULL;
    if (word_syms_filename != "")
      if (!(word_syms = fst::SymbolTable::ReadText(word_syms_filename)))
        KALDI_ERR << "Could not read symbol table from file "
                   << word_syms_filename;

    double tot_like = 0.0;
    kaldi::int64 frame_count = 0;
    int num_success = 0, num_fail = 0;

    if (ClassifyRspecifier(fst_in_str, NULL, NULL) == kNoRspecifier) {
      SequentialBaseFloatMatrixReader loglike_reader(feature_rspecifier);
      // Input FST is just one FST, not a table of FSTs.
      Fst<StdArc> *decode_fst = fst::ReadFstKaldiGeneric(fst_in_str);
      timer.Reset();

      {
        LatticeFasterDecoder decoder(*decode_fst, config);

        for (; !loglike_reader.Done(); loglike_reader.Next()) {
          std::string utt = loglike_reader.Key();
          Matrix<BaseFloat> loglikes (loglike_reader.Value());
          loglike_reader.FreeCurrent();
          if (loglikes.NumRows() == 0) {
            KALDI_WARN << "Zero-length utterance: " << utt;
            num_fail++;
            continue;
          }

          DecodableMatrixScaled decodable(loglikes, acoustic_scale);

		  if (!decoder.Decode(&decodable)) {
			  KALDI_WARN << "Failed to decode utterance with id " << utt;
			  return -1;
		  }
		  if (!decoder.ReachedFinal()) {
			  if (allow_partial) {
				  KALDI_WARN << "Outputting partial output for utterance " << utt
					  << " since no final-state reached\n";
			  } else {
				  KALDI_WARN << "Not producing output for utterance " << utt
					  << " since no final-state reached and "
					  << "--allow-partial=false.\n";
				  return -1;
			  }
		  }

		  double likelihood;
		  LatticeWeight weight;
		  int32 num_frames;
		  { // First do some stuff with word-level traceback...
			  fst::VectorFst<LatticeArc> decoded;
			  if (!decoder.GetBestPath(&decoded))
				  // Shouldn't really reach this point as already checked success.
				  KALDI_ERR << "Failed to get traceback for utterance " << utt;
			  std::vector<int32> alignment;
			  std::vector<int32> words;
			  GetLinearSymbolSequence(decoded, &alignment, &words, &weight);
			  num_frames = alignment.size();
			  if (words_writer.IsOpen())
				  words_writer.Write(utt, words);
			  if (alignment_writer.IsOpen())
				  alignment_writer.Write(utt, alignment);
			  if (word_syms != NULL) {
				  std::cerr << utt << ' ';
				  for (size_t i = 0; i < words.size(); i++) {
					  std::string s = word_syms->Find(words[i]);
					  if (s == "")
						  KALDI_ERR << "Word-id " << words[i] << " not in symbol table.";
					  std::cerr << s << ' ';
				  }
				  std::cerr << '\n';
			  }
			  likelihood = -(weight.Value1() + weight.Value2());
		  }
		  // Get lattice, and do determinization if requested.
		  Lattice lat;
		  decoder.GetRawLattice(&lat);
		  if (lat.NumStates() == 0)
			  KALDI_ERR << "Unexpected problem getting lattice for utterance " << utt;
		  fst::Connect(&lat);
		  if (determinize) {
			  CompactLattice clat;
			  //DeterminizeLatticePrunedOptions det_opts;
			  //det_opts.delta=decoder.GetOptions().det_opts.delta;
			  //det_opts.max_mem=decoder.GetOptions().det_opts.max_mem;
			  //if(!DeterminizeLatticePruned)
			  // We'll write the lattice without acoustic scaling.
			  if (acoustic_scale != 0.0)
				  fst::ScaleLattice(fst::AcousticLatticeScale(1.0 / acoustic_scale), &clat);
			  compact_lattice_writer.Write(utt, clat);
		  } else {
			  if (acoustic_scale != 0.0)
				  fst::ScaleLattice(fst::AcousticLatticeScale(1.0 / acoustic_scale), &lat);
			  lattice_writer.Write(utt, lat);
		  }
		  KALDI_LOG << "Log-like per frame for utterance " << utt << " is "
			  << (likelihood / num_frames) << " over "
			  << num_frames << " frames.";
		  KALDI_VLOG(2) << "Cost for utterance " << utt << " is "
			  << weight.Value1() << " + " << weight.Value2();

		  {
            tot_like += likelihood;
            frame_count += loglikes.NumRows();
            num_success++;
		  }
        }
      }
      delete decode_fst; // delete this only after decoder goes out of scope.
    }

    double elapsed = timer.Elapsed();
    KALDI_LOG << "Time taken "<< elapsed
              << "s: real-time factor assuming 100 frames/sec is "
              << (elapsed*100.0/frame_count);
    KALDI_LOG << "Done " << num_success << " utterances, failed for "
              << num_fail;
    KALDI_LOG << "Overall log-likelihood per frame is " << (tot_like/frame_count) << " over "
              << frame_count<<" frames.";

    delete word_syms;
    if (num_success != 0) return 0;
    else return 1;
  } catch(const std::exception &e) {
    std::cerr << e.what();
    return -1;
  }
}

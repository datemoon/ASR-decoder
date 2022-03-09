include ../../depend.mk

AM_CPPFLAGS = -I$(KALDI_INC) -I$(OPENFST_INC) -I$(CUDA_INC) -I$(MATRIX_INC)

AM_LDFLAGS = $(MATRIX_DEPLDFLAGS) $(KALDI_DEPLDFLAGS) $(CUDA_DEPLDFLAGS)

LDADD = $(KALDI_LIBS) \
		$(MATRIX_LIBS) $(CUDA_LIBS) \
		$(DL_LIBS)

noinst_PROGRAMS = latgen-faster matrix-row-min nbest-compute-wer gpu-forward \
				  lattice-best-path-score

lattice_best_path_score_SOURCES = lattice-best-path-score.cc

latgen_faster_SOURCES = latgen-faster.cc
matrix_row_min_SOURCES = matrix-row-min.cc
nbest_compute_wer_SOURCES = nbest-compute-wer.cc
gpu_forward_SOURCES=gpu-forward.cc

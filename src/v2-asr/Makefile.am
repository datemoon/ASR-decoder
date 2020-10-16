include ../depend.mk
AM_CPPFLAGS = -I$(CUDA_INC) \
			  -I$(KALDI_INC) \
			  -I$(MATRIX_INC) \
			  -I$(OPENFST_INC)

lib_LTLIBRARIES = libv2asr.la

libv2asr_la_SOURCES = v2-asr-work-thread.cc asr-client-task.cc

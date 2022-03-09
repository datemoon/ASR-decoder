include ../depend.mk
AM_CPPFLAGS = -I$(CUDA_INC) \
			  -I$(KALDI_INC) \
			  -I$(MATRIX_INC) \
			  -I$(OPENFST_INC)

AM_LDFLAGS = $(CUDA_DEPLDFLAGS) \
			 $(KALDI_DEPLDFLAGS) \
			 $(MATRIX_DEPLDFLAGS)

lib_LTLIBRARIES = libv1asr.la

libv1asr_la_SOURCES = asr-work-thread.cc asr-source.cc
libv1asr_la_LDFLAGS = -rpath '$(libdir)'

check_PROGRAMS = kaldi-v1-asr-online-test 

kaldi_v1_asr_online_test_SOURCES = kaldi-v1-asr-online-test.cc
kaldi_v1_asr_online_test_LDADD = $(KALDI_LIBS) \
								 $(MATRIX_LIBS) $(CUDA_LIBS) \
								 $(DL_LIBS)

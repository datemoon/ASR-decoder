include ../depend.mk
AM_CPPFLAGS = -I$(KALDI_INC) -I$(OPENFST_INC) -I$(CUDA_INC) -I$(MATRIX_INC)

lib_LTLIBRARIES = libonlinedecode.la

libonlinedecode_la_SOURCES = kaldi-online-nnet3-my-decoder.cc
libonlinedecode_la_LDFLAGS = -rpath '$(libdir)'

AM_LDFLAGS = $(MATRIX_DEPLDFLAGS) $(KALDI_DEPLDFLAGS) $(CUDA_DEPLDFLAGS)

LDADD = libonlinedecode.la \
	   	../my-decoder/liboptimizedecode.la \
		../newlm/liblm.la \
		../newfst/liblatfst.la \
	   	../util/libutil.la \
		$(KALDI_LIBS) \
		$(MATRIX_LIBS) $(CUDA_LIBS) \
		$(DL_LIBS)

check_PROGRAMS = kaldi-online-nnet3-my-decoder-test
kaldi_online_nnet3_my_decoder_test_SOURCES = kaldi-online-nnet3-my-decoder-test.cc


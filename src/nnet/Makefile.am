include ../depend.mk
AM_CPPFLAGS = -I$(MATRIX_INC) -DCBLAS
lib_LTLIBRARIES = libnnet.la

libnnet_la_SOURCES = nnet-layer.cc \
					 matrix.cc \
					 nnet-component.cc \
					 nnet-nnet.cc \
					 nnet-feature-api.cc \
					 lstm-layer.cc \
					 nnet-util.cc \
					 cudnn-lstm-layer.cc \
					 nnet-feature-pitch-api.cc \
					 lstm-projected-layer.cc \
					 nnet-simple-recurrent.cc \
				 	 tf-lstm-layer.cc

if HAVE_TEST_NNET
AM_LDFLAGS = $(MATRIX_DEPLDFLAGS) $(KALDI_DEPLDFLAGS) $(CUDA_DEPLDFLAGS)
LDADD = libnnet.la \
		../pitch/libpitch.la \
		../hmm/libhmm.la \
		../util/libutil.la \
		$(MATRIX_LIBS) $(FEATLIB) \
		-lm -lpthread -ldl


bin_PROGRAMS = nnetread \
			   nnet-feature-api-test \
			   forward-test \
			   kaldi-forward-test \
			   nnet-read-test \
			   pitch-forward-test \
			   stream-kaldi-forward-test \
			   kaldi-format-pitch-forward-test

nnetread_SOURCES = nnetread.cc
nnet_feature_api_test_SOURCES = nnet-feature-api-test.cc
forward_test_SOURCES = forward-test.cc
kaldi_forward_test_SOURCES = kaldi-forward-test.cc
nnet_read_test_SOURCES = nnet-read-test.cc
pitch_forward_test_SOURCES = pitch-forward-test.cc
stream_kaldi_forward_test_SOURCES = stream-kaldi-forward-test.cc
kaldi_format_pitch_forward_test_SOURCES = kaldi-format-pitch-forward-test.cc


endif

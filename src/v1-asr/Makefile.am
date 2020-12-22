include ../depend.mk
AM_CPPFLAGS = -I$(CUDA_INC) \
			  -I$(KALDI_INC) \
			  -I$(MATRIX_INC) \
			  -I$(OPENFST_INC)

lib_LTLIBRARIES = libv1asr.la

libv1asr_la_SOURCES = asr-work-thread.cc asr-source.cc
libv1asr_la_LDFLAGS = -rpath '$(libdir)'

check_PROGRAMS = online-nnet3-vad-test \
				 online-nnet3-vad-stream-test \
				 kaldi-v1-asr-online-test \
				 online-energy-vad-stream-test

online_nnet3_vad_test_SOURCES = online-nnet3-vad-test.cc
online_nnet3_vad_test_LDADD = $(KALDI_LIBS) \
								$(MATRIX_LIBS) $(CUDA_LIBS) \
								$(DL_LIBS)

online_nnet3_vad_stream_test_SOURCES = online-nnet3-vad-stream-test.cc
online_nnet3_vad_stream_test_LDADD = $(KALDI_LIBS) \
									   $(MATRIX_LIBS) $(CUDA_LIBS) \
									   $(DL_LIBS)

kaldi_v1_asr_online_test_SOURCES = kaldi-v1-asr-online-test.cc
kaldi_v1_asr_online_test_LDADD = $(KALDI_LIBS) \
								 $(MATRIX_LIBS) $(CUDA_LIBS) \
								 $(DL_LIBS)



online_energy_vad_stream_test_SOURCES = online-energy-vad-stream-test.cc
online_energy_vad_stream_test_LDADD = $(KALDI_LIBS) \
									  $(MATRIX_LIBS) $(CUDA_LIBS) \
									  $(DL_LIBS)



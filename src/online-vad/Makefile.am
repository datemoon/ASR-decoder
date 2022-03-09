include ../depend.mk
AM_CPPFLAGS = -I$(CUDA_INC) \
			  -I$(KALDI_INC) \
			  -I$(MATRIX_INC) \
			  -I$(OPENFST_INC)

AM_LDFLAGS = $(CUDA_DEPLDFLAGS) \
			 $(KALDI_DEPLDFLAGS) \
			 $(MATRIX_DEPLDFLAGS)

lib_LTLIBRARIES = 

check_PROGRAMS = online-nnet3-vad-test \
				 online-nnet3-vad-stream-test \
				 online-energy-vad-stream-test \
				 new-online-energy-vad-stream-test


online_nnet3_vad_test_SOURCES = online-nnet3-vad-test.cc
online_nnet3_vad_test_LDADD = $(KALDI_LIBS) \
								$(MATRIX_LIBS) $(CUDA_LIBS) \
								$(DL_LIBS)

online_nnet3_vad_stream_test_SOURCES = online-nnet3-vad-stream-test.cc
online_nnet3_vad_stream_test_LDADD = $(KALDI_LIBS) \
									   $(MATRIX_LIBS) $(CUDA_LIBS) \
									   $(DL_LIBS)

online_energy_vad_stream_test_SOURCES = online-energy-vad-stream-test.cc
online_energy_vad_stream_test_LDADD = $(KALDI_LIBS) \
									  $(MATRIX_LIBS) $(CUDA_LIBS) \
									  $(DL_LIBS)

new_online_energy_vad_stream_test_SOURCES = new-online-energy-vad-stream-test.cc
new_online_energy_vad_stream_test_LDADD = $(KALDI_LIBS) \
										  $(MATRIX_LIBS) $(CUDA_LIBS) \
										  $(DL_LIBS)


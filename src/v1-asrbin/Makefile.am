include ../depend.mk
AM_CPPFLAGS = -I$(CUDA_INC) \
			  -I$(KALDI_INC) \
			  -I$(MATRIX_INC) \
			  -I$(OPENFST_INC)

AM_LDFLAGS = $(CUDA_DEPLDFLAGS) \
			 $(KALDI_DEPLDFLAGS) \
			 $(MATRIX_DEPLDFLAGS)

LDADD = ../service2/libservice2.la \
		../util/libutil.la

LDADD += $(KALDI_LIBS) \
		 $(MATRIX_LIBS) \
		 $(CUDA_LIBS) \
		 -lm -lpthread $(DL_LIBS)

bin_PROGRAMS = v1-asr-service 

noinst_PROGRAMS = 

#thread_client_LDADD = ../service2/libservice2.la ../v2-asr/libv2asr.la ../util/libutil.la

#thread_client_PROGRAMS = thread-client

v1_asr_service_SOURCES = v1-asr-service.cc


include ../depend.mk
AM_CPPFLAGS = -I$(CUDA_INC) \
			  -I$(KALDI_INC) \
			  -I$(MATRIX_INC) \
			  -I$(OPENFST_INC)

AM_LDFLAGS = $(CUDA_DEPLDFLAGS) \
			 $(KALDI_DEPLDFLAGS) \
			 $(MATRIX_DEPLDFLAGS)

LDADD = ../v2-asr/libv2asr.la \
		../v1-asr/libv1asr.la \
		../kaldi-nnet3/libonlinedecode.la \
		../service2/libservice2.la \
		../decoder/liboptimizedecode.la \
		../newlm/liblm.la \
		../newfst/liblatfst.la \
		../util/libutil.la

LDADD += $(KALDI_LIBS) \
		 $(MATRIX_LIBS) \
		 $(CUDA_LIBS) \
		 -lm -lpthread $(DL_LIBS)

bin_PROGRAMS = asr-service \
			   asr-client \
			   asr-client-v1 \
			   asr-client-v2 \
			   v2-asr-service \
			   thread-client


asr_service_SOURCES = asr-service.cc
asr_client_SOURCES = asr-client.cc
asr_client_v1_SOURCES = asr-client-v1.cc
asr_client_v2_SOURCES = asr-client-v2.cc
v2_asr_service_SOURCES = v2-asr-service.cc
thread_client_SOURCES = thread-client.cc

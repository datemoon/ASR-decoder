include ../depend.mk
AM_CPPFLAGS = -I$(CUDA_INC) \
			  -I$(KALDI_INC) \
			  -I$(MATRIX_INC) \
			  -I$(OPENFST_INC)

AM_LDFLAGS = $(CUDA_DEPLDFLAGS) \
			 $(KALDI_DEPLDFLAGS) \
			 $(MATRIX_DEPLDFLAGS)

LDADD = ../v2-asr/libv2asr.la \
		../kaldi-nnet3/libonlinedecode.la \
		../service2/libservice2.la \
		../my-decoder/liboptimizedecode.la \
		../newlm/liblm.la \
		../newfst/liblatfst.la \
		../util/libutil.la

#		../newlm/liblm.la 

LDADD += $(KALDI_LIBS) \
		 $(MATRIX_LIBS) \
		 $(CUDA_LIBS) \
		 -lm -lpthread $(DL_LIBS)

if USA_CUDA_DECODER
bin_PROGRAMS =
noinst_PROGRAMS = v2-asr-service 

else
bin_PROGRAMS = v2-asr-service 

noinst_PROGRAMS = 

endif

v2_asr_service_SOURCES = v2-asr-service.cc

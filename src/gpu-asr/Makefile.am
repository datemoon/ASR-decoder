include ../depend.mk
AM_CPPFLAGS = -I$(CUDA_INC) \
			  -I$(KALDI_INC) \
			  -I$(MATRIX_INC) \
			  -I$(OPENFST_INC) 

if USECUDA
AM_CPPFLAGS += -DHAVE_CUDA
endif

AM_LDFLAGS = $(CUDA_DEPLDFLAGS) \
			 $(CUDA_DEPLDFLAGSCONST) \
			 $(KALDI_DEPLDFLAGS) \
			 $(MATRIX_DEPLDFLAGS)

LDADD = ../service2/libservice2.la \
		../util/libutil.la

# 添加cudadecoder相关库
LDADD += $(KALDI_ROOT)/cudadecoder/kaldi-cudadecoder.a \
		 $(KALDI_ROOT)/cudafeat/kaldi-cudafeat.a

LDADD += $(KALDI_LIBS) \
		 $(MATRIX_LIBS) \
		 $(CUDA_LIBS) \
		 $(CUDA_LIBSCONST) \
		 -lm -lpthread $(DL_LIBS)
if USECUDA
if USA_CUDA_DECODER
bin_PROGRAMS = v1-gpu-asr-service 
else
noinst_PROGRAMS = v1-gpu-asr-service
endif
endif

v1_gpu_asr_service_SOURCES = v1-gpu-asr-service.cc


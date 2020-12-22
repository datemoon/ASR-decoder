include ../depend.mk
AM_CPPFLAGS = -I$(CUDA_INC) \
			  -I$(KALDI_INC) \
			  -I$(MATRIX_INC) \
			  -I$(OPENFST_INC)

AM_LDFLAGS = $(CUDA_DEPLDFLAGS) \
			 $(KALDI_DEPLDFLAGS) \
			 $(MATRIX_DEPLDFLAGS)

LDADD = ../kaldi-nnet3/libonlinedecode.la \
		../decoder/liboptimizedecode.la \
		../newlm/liblm.la \
		../newfst/liblatfst.la \
		../util/libutil.la 

LDADD += $(KALDI_LIBS) \
		 $(MATRIX_LIBS) \
		 $(CUDA_LIBS)  \
		 $(DL_LIBS)


noinst_PROGRAMS = kaldi-online-nnet3-my-decoder-bin \
			   kaldi-my-decoder \
			   kaldi-hclg-my-decoder \
 			   kaldi-hclg-my-decoder-biglm 

kaldi_online_nnet3_my_decoder_bin_SOURCES = kaldi-online-nnet3-my-decoder-bin.cc

kaldi_my_decoder_SOURCES = kaldi-my-decoder.cc

kaldi_hclg_my_decoder_SOURCES = kaldi-hclg-my-decoder.cc

kaldi_hclg_my_decoder_biglm_SOURCES = kaldi-hclg-my-decoder-biglm.cc

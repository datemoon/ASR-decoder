include ../depend.mk

KALDI_THIS_LIBS=$(KALDI_ROOT)/lm/kaldi-lm.a \
				$(KALDI_ROOT)/lat/kaldi-lat.a \
				$(KALDI_ROOT)/fstext/kaldi-fstext.a \
	 			$(KALDI_ROOT)/hmm/kaldi-hmm.a \
				$(KALDI_ROOT)/tree/kaldi-tree.a \
	 			$(KALDI_ROOT)/util/kaldi-util.a \
	 			$(KALDI_ROOT)/matrix/kaldi-matrix.a \
	 			$(KALDI_ROOT)/base/kaldi-base.a \
	 			$(FST_LIBS) \
				$(MATRIX_LIBS) 

AM_CPPFLAGS = -I$(KALDI_INC) \
			  -I$(OPENFST_INC) \
			  -I$(MATRIX_INC) \
			  -DHAVE_CUDA 
			  
#-DDEBUG

AM_LDFLAGS = $(KALDI_DEPLDFLAGS) \
			 $(MATRIX_DEPLDFLAGS)
	

LDADD = ../service2/libservice2.la \
		../util/libutil.la

LDADD += $(KALDI_THIS_LIBS) \
		 -lm -lpthread $(DL_LIBS)

bin_PROGRAMS = asr-post-process-service

asr_post_process_service_SOURCES = asr-post-process-service.cc

noinst_PROGRAMS = client-asr-post-process-service

client_asr_post_process_service_SOURCES = client-asr-post-process-service.cc

check_PROGRAMS = post-package-test

post_package_test_SOURCES = post-package-test.cc
post_package_test_LDADD = ../service2/libservice2.la \
						  ../util/libutil.la \
						  $(KALDI_THIS_LIBS) \
						  -lm -lpthread $(DL_LIBS)



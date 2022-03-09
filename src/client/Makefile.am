include ../depend.mk

SUBDIRS = py-client

#AM_CPPFLAGS = -I$(CUDA_INC) \
#			  -I$(KALDI_INC) \
#			  -I$(MATRIX_INC) \
#			  -I$(OPENFST_INC)

lib_LTLIBRARIES = libclientv1.la

libclientv1_la_SOURCES = asr-client-task.cc
libclientv1_la_LDFLAGS = -rpath '$(libdir)'

include ../depend.mk

LDADD = ../client/libclientv1.la \
		../service2/libservice2.la \
		../util/libutil.la

LDADD += -lm -lpthread $(DL_LIBS)

bin_PROGRAMS = 
noinst_PROGRAMS = thread-client
#thread_client_LDADD = ../service2/libservice2.la ../v2-asr/libv2asr.la ../util/libutil.la

#thread_client_PROGRAMS = thread-client

thread_client_SOURCES = thread-client.cc

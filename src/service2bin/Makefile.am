include ../depend.mk

LDADD = ../service2/libservice2.la \
		../util/libutil.la

LDADD += -lm -lpthread $(DL_LIBS)

bin_PROGRAMS =
noinst_PROGRAMS = asr-client \
				  asr-client-v1 \
				  asr-client-v2 

asr_client_SOURCES = asr-client.cc
asr_client_v1_SOURCES = asr-client-v1.cc
asr_client_v2_SOURCES = asr-client-v2.cc

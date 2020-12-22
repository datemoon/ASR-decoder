AM_CPPFLAGS = 

lib_LTLIBRARIES = libclient.la

libclient_la_SOURCES = asr-client-api.cc 

libclient_la_LIBADD = ../service2/libservice2.la \
					 ../util/libutil.la
libclient_la_LDFLAGS = -rpath '$(libdir)'


check_PROGRAMS = asr-client-api-test

#AM_DEFAULT_SOURCE_EXT = .cc
asr_client_api_test_SOURCES = asr-client-api-test.cc

asr_client_api_test_LDADD = libclient.la \
		../service2/libservice2.la \
		../util/libutil.la 


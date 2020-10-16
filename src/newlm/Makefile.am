
lib_LTLIBRARIES = liblm.la

liblm_la_SOURCES = arpa2fsa.cc \
				   compose-arpalm.cc

if HAVE_BIN
LDADD = liblm.la  \
		../newfst/liblatfst.la \
		../util/libutil.la -lpthread

bin_PROGRAMS = arpa2fsa-bin \
			   lmscore \
			   compose-text \
			   lmrescale \
			   compose-diff-text \
			   lat-compose

arpa2fsa_bin_SOURCES = arpa2fsa-bin.cc
lmscore_SOURCES = lmscore.cc
compose_text_SOURCES = compose-text.cc
lmrescale_SOURCES = lmrescale.cc
compose_diff_text_SOURCES = compose-diff-text.cc
lat_compose_SOURCES = lat-compose.cc
endif

if HAVE_TEST

endif

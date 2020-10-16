

lib_LTLIBRARIES = libhmm.la

libhmm_la_SOURCES = hmm-topology.cc \
					transition-model.cc

if HAVE_TEST

LDADD = libhmm.la \
	   ../util/libutil.la

bin_PROGRAMS = test-hmm \
			   test-transition-model

test_hmm_SOURCES = test-hmm.cc

test_transition_model_SOURCES = test-transition-model.cc

endif

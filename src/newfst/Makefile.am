AM_CPPFLAGS = 
LDADD = ../newfst/liblatfst.la ../util/libutil.la 
lib_LTLIBRARIES = liblatfst.la

liblatfst_la_SOURCES = lattice-determinize-api.cc \
					   lattice-to-nbest.cc \
  					   connect-fst.cc lattice-fst.cc \
  					   reverse.cc \
  					   dfs-visit-fst.cc lattice-functions.cc \
  					   rmepsilon.cc topsort.cc \
  					   invert.cc 


if HAVE_TEST

bin_PROGRAMS = test-connect-fst \
			   lattice-test \
			   test-code \
			   test-heap \
			   const-fst-read

test_connect_fst_SOURCES = test-connect-fst.cc

lattice_test_SOURCES = lattice-test.cc

test_code_SOURCES = test-code.cc

test_heap_SOURCES = test-heap.cc

const_fst_read_SOURCES = const-fst-read.cc

endif

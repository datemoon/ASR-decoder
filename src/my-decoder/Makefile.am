include ../depend.mk
AM_CPPFLAGS = -I$(KALDI_INC) -I$(OPENFST_INC)
lib_LTLIBRARIES = liboptimizedecode.la

liboptimizedecode_la_SOURCES = mem-optimize-clg-lattice-faster-online-decoder.cc \
							   lattice-faster-decoder.cc \
							   mem-optimize-hclg-lattice-faster-online-decoder.cc

liboptimizedecode_la_LDFLAGS = -rpath '$(libdir)'


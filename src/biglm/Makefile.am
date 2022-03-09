include ../depend.mk
AM_CPPFLAGS = -I$(CUDA_INC) \
			  -I$(KALDI_INC) \
			  -I$(MATRIX_INC) \
			  -I$(OPENFST_INC)

AM_LDFLAGS = $(CUDA_DEPLDFLAGS) \
			 $(KALDI_DEPLDFLAGS) \
			 $(MATRIX_DEPLDFLAGS)

LDADD = 

LDADD += $(KALDI_LIBS) \
		 $(MATRIX_LIBS) \
		 $(CUDA_LIBS) \
		 -lm -lpthread $(DL_LIBS)

bin_PROGRAMS =

noinst_PROGRAMS = lattice-read-and-write

lattice_read_and_write_SOURCES = lattice-read-and-write.cc


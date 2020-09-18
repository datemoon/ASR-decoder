

include ../hubo.mk

BIN=

LIBS=../hmm/libhmm.a ../nnet/libnnet.a ../optimize-mem-decoder/liboptimizedecode.a \
	  $(CBLASLIB) ../align/libalign.a ../util/libutil.a ../newfst/liblatfst.a $(FEATLIB)

TESTFILES=optimize-clg-lattice-online-faster-decoder-kaldifeature-test

OBJ=mem-optimize-clg-lattice-faster-online-decoder.o lattice-faster-decoder.o mem-optimize-hclg-lattice-faster-online-decoder.o

all:liboptimizedecode.a $(BIN)


test:$(TESTFILES)

optimize-clg-lattice-online-faster-decoder-kaldifeature-test:optimize-clg-lattice-online-faster-decoder-kaldifeature-test.o
	$(CXX) $(CXXFLAGS) -o  $@ $^ $(LIBS) 

liboptimizedecode.a:$(OBJ)
	$(AR) -rcu $@ $^

%.o:%.c
	$(CXX) $(CXXFLAGS) -c  -o $@ $^

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c  -o $@ $^

%.o:%.cc
	$(CXX) $(CXXFLAGS) -c  -o $@ $^

.PHONY:

clean:
	rm -f *.o $(BIN) liboptimizedecode.a $(TESTFILES)

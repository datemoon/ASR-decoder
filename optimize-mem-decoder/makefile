

include ../hubo.mk

CXXFLAGS+= -I ../ -I ../nnet -I ../lib #-DPHONEQUEUE #-DDEBUGPRINT #-DTOKEN_ACTIVE_PRINT


BIN=

LIBS=../hmm/libhmm.a ../nnet/libnnet.a \
	  $(CBLASLIB) ../align/libalign.a ../util/libutil.a ../fst/liblatfst.a $(FEATLIB)

all:liboptimizedecode.a $(BIN)


liboptimizedecode.a:mem-optimize-clg-lattice-faster-online-decoder.o
	$(AR) -rcu $@ $^

%.o:%.c
	    $(CXX) $(CXXFLAGS) -c  -o $@ $^

%.o:%.cpp
	    $(CXX) $(CXXFLAGS) -c  -o $@ $^

%.o:%.cc
	    $(CXX) $(CXXFLAGS) -c  -o $@ $^
.PHONY:

clean:
	rm -f *.o $(BIN) liboptimizedecode.a

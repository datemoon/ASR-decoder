

include ../hubo.mk

CXXFLAGS+= -I. #-DPHONEQUEUE #-DDEBUGPRINT #-DTOKEN_ACTIVE_PRINT


TESTBIN=test-optimize-fst optimize-faster-decoder-kaldifeature-test \
	optimize-faster-decoder-test \
	optimize-ctc-faster-decoder-kaldifeature-test \
	optimize-ctc-lattice-faster-decoder-kaldifeature-test \
	optimize-faster-decoder-kaldifeat-test \
	optimize-clg-lattice-online-faster-decoder-kaldifeature-test

LIBS=../hmm/libhmm.a ../nnet/libnnet.a ../decoder/libdecode.a \
	  $(CBLASLIB) ../align/libalign.a ../util/libutil.a ../fst/liblatfst.a $(FEATLIB)

all:libdecode.a

test:$(TESTBIN)

test-optimize-fst:test-optimize-fst.o 
	$(CXX) $(CXXFLAGS) -o  $@ $^

libdecode.a:optimize-faster-decoder.o optimize-ctc-faster-decoder.o lattice-faster-decoder.o clg-lattice-faster-online-decoder.o
	$(AR) -rcu $@ $^

optimize-faster-decoder-test:optimize-faster-decoder-test.o
	$(CXX) $(CXXFLAGS) -o  $@ $^ $(LIBS)

optimize-faster-decoder-kaldifeat-test:optimize-faster-decoder-kaldifeat-test.o
	$(CXX) $(CXXFLAGS) -o  $@ $^ $(LIBS)

optimize-ctc-faster-decoder-kaldifeature-test:optimize-ctc-faster-decoder-kaldifeature-test.o
	$(CXX) $(CXXFLAGS) -o  $@ $^ $(LIBS)

optimize-faster-decoder-kaldifeature-test:optimize-faster-decoder-kaldifeature-test.o
	$(CXX) $(CXXFLAGS) -o  $@ $^ $(LIBS)

optimize-ctc-lattice-faster-decoder-kaldifeature-test:optimize-ctc-lattice-faster-decoder-kaldifeature-test.o
	$(CXX) $(CXXFLAGS)  -o  $@ $^ $(LIBS)

optimize-clg-lattice-online-faster-decoder-kaldifeature-test:optimize-clg-lattice-online-faster-decoder-kaldifeature-test.o
	$(CXX) $(CXXFLAGS)  -o  $@ $^ $(LIBS)

%.o:%.c
	    $(CXX) $(CXXFLAGS) -c  -o $@ $^

%.o:%.cpp
	    $(CXX) $(CXXFLAGS) -c  -o $@ $^

%.o:%.cc
	    $(CXX) $(CXXFLAGS) -c  -o $@ $^
.PHONY:

clean:
	rm -f *.o $(TESTBIN) libdecode.a

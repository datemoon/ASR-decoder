
include ../hubo.mk

LIBS=../decoder/liboptimizedecode.a ../newfst/liblatfst.a ../util/libutil.a

OBJ=kaldi-online-nnet3-my-decoder.o

LIB=libonlinedecode.a

TESTBIN=kaldi-online-nnet3-my-decoder-test

all:$(LIB) test

test:$(TESTBIN)

kaldi-online-nnet3-my-decoder-test:kaldi-online-nnet3-my-decoder-test.o
	$(GXX) $(CXXFLAGS) -o $@ $^ $(LIBS) $(KALDI_LIB_DYNAMIC) $(CBLAS_LIB)
	#$(CXX) $(CXXFLAGS) -o  $@ $^ $(LIBS) $(KALDI_LIB_STATIC) $(CBLAS_LIB) $(FST_LIB)

libonlinedecode.a:$(OBJ)
	$(AR) -rcu $@ $^

%.o:%.cc
	$(GXX) $(CXXFLAGS) -c -o $@ $< 

.PHONY:

clean:
	rm -f *.o $(TESTBIN) $(LIB)

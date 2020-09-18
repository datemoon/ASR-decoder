
include ../hubo.mk

LIBS=../decoder/liboptimizedecode.a ../newfst/liblatfst.a ../util/libutil.a

OBJ=kaldi-online-nnet3-my-decoder.o

LIB=libonlinedecode.a

TESTBIN=kaldi-online-nnet3-my-decoder-test

all:$(LIB)

test:$(TESTBIN)

kaldi-online-nnet3-my-decoder-test:kaldi-online-nnet3-my-decoder-test.o
	$(GXX) $(CXXFLAGS) -o $@ $^ $(LIBS) $(KALDI_LIBS) $(CBLAS_LIB)

libonlinedecode.a:$(OBJ)
	$(AR) -rcu $@ $^

%.o:%.cc
	$(GXX) $(CXXFLAGS) -c -o $@ $< 

.PHONY:

clean:
	rm -f *.o $(TESTBIN) $(LIB) 

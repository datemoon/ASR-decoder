
include ../hubo.mk

CXXFLAGS+=-I. -lm

LIB=../hmm/libhmm.a ../util/libutil.a 

TESTBIN=test-hmm test-transition-model

all:libhmm.a

test:$(TESTBIN)

libhmm.a:hmm-topology.o transition-model.o
	$(AR) -rc $@ $^

test-hmm:test-hmm.o 
	$(GXX) $(CXXFLAGS) -o $@ $^ $(LIB)

test-transition-model:test-transition-model.o
	$(GXX) $(CXXFLAGS) -o $@ $^ $(LIB)


.PHONY:


clean:
	rm -f *.o libhmm.a test-hmm test-transition-model


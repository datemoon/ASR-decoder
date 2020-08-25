
include ../hubo.mk

CXXFLAGS+=-I. -lm

OBJ=arpa2fsa.o compose-arpalm.o

LIB=liblm.a  ../fst/liblatfst.a ../util/libutil.a

BIN= arpa2fsa-bin lmscore compose-text lmrescale
all:liblm.a 

test:$(BIN)

liblm.a:$(OBJ)
	$(AR) -rc $@ $^

lmscore:lmscore.o
	$(GXX) $(CXXFLAGS) -o $@ $^ $(LIB)

arpa2fsa-bin:arpa2fsa-bin.o
	$(GXX) $(CXXFLAGS) -o $@ $^ $(LIB)

compose-text:compose-text.o
	$(GXX) $(CXXFLAGS) -o $@ $^ $(LIB)

lmrescale:lmrescale.o
	$(GXX) $(CXXFLAGS) -o $@ $^ $(LIB)

%.o:%.cpp
	$(GXX) $(CXXFLAGS) -o $@ -c $< $(LDLIBS)

%.o:%.cc
	$(GXX) $(CXXFLAGS) -o $@ -c $< $(LDLIBS)


.PHONY:


clean:
	rm -f *.o liblm.a $(BIN)


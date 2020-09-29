
include ../hubo.mk

CXXFLAGS+=-I. 

OBJ=arpa2fsa.o compose-arpalm.o

LIB=liblm.a  ../newfst/liblatfst.a ../util/libutil.a $(LDLIBS)

BIN= arpa2fsa-bin lmscore compose-text lmrescale compose-diff-text \
	 lat-compose

all:liblm.a 

test:$(BIN)

liblm.a:$(OBJ)
	$(AR) -rc $@ $^

lat-compose:lat-compose.o
	$(GXX) -o $@ $^ $(LIB)

compose-diff-text:compose-diff-text.o
	$(GXX) -o $@ $^ $(LIB)

lmscore:lmscore.o
	$(GXX) -o $@ $^ $(LIB)

arpa2fsa-bin:arpa2fsa-bin.o
	$(GXX) -o $@ $^ $(LIB)

compose-text:compose-text.o
	$(GXX) -o $@ $^ $(LIB)

lmrescale:lmrescale.o
	$(GXX) -o $@ $^ $(LIB)

%.o:%.cpp
	$(GXX) $(CXXFLAGS) -o $@ -c $< 

%.o:%.cc
	$(GXX) $(CXXFLAGS) -o $@ -c $< 


.PHONY:


clean:
	rm -f *.o liblm.a $(BIN)


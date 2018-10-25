
include ../hubo.mk

CXXFLAGS+=-I. -I.. -lm

OBJ=arpa2fsa.o compose-arpalm.o

LIB=../lm/liblm.a  ../fst/liblatfst.a ../util/libutil.a

BIN= arpa2fsa-bin lmscore compose-text
all:liblm.a $(BIN)

liblm.a:$(OBJ)
	$(AR) -rc $@ $^

lmscore:lmscore.o
	$(GXX) $(CXXFLAGS) -o $@ $^ $(LIB)

arpa2fsa-bin:arpa2fsa-bin.o
	$(GXX) $(CXXFLAGS) -o $@ $^ $(LIB)

compose-text:compose-text.o
	$(GXX) $(CXXFLAGS) -o $@ $^ $(LIB)

%.o:%.cpp
	$(GXX) $(CXXFLAGS) -o $@ -c $< $(LDLIBS)

%.o:%.cc
	$(GXX) $(CXXFLAGS) -o $@ -c $< $(LDLIBS)


.PHONY:


clean:
	rm -f *.o liblm.a $(BIN)


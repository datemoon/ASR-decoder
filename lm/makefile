
include ../hubo.mk

CXXFLAGS+=-I. -I.. -lm

OBJ=arpa2fsa.o

LIB=../lm/liblm.a 

all:liblm.a arpa2fsa-bin

liblm.a:$(OBJ)
	$(AR) -rc $@ $^

arpa2fsa-bin:arpa2fsa-bin.o
	$(GXX) $(CXXFLAGS) -o $@ $^ $(LIB)

%.o:%.cpp
	$(GXX) $(CXXFLAGS) -o $@ -c $< $(LDLIBS)

%.o:%.cc
	$(GXX) $(CXXFLAGS) -o $@ -c $< $(LDLIBS)


.PHONY:


clean:
	rm -f *.o liblm.a arpa2fsa-bin


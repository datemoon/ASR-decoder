
include ../hubo.mk

CXXFLAGS+=-I . 

LIB=../util/libutil.a

BIN=test-phone-to-word

all:test-phone-to-word libalign.a

test-phone-to-word:phone-to-word.o test-phone-to-word.o
	$(GXX) $(CXXFLAGS) -o $@ $^ $(LIB)

libalign.a:phone-to-word.o
	$(AR) -rc $@ $^

%.o:%.cpp
	$(GXX) $(CXXFLAGS) -c -o $@ $< 

.PHONY:

clean:
	rm -f *.o $(BIN) libalign.a

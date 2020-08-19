
include ../hubo.mk

CXXFLAGS+=-I . 

LIB=../util/libutil.a

TESTBIN=test-phone-to-word

all:libalign.a

test:$(TESTBIN)

test-phone-to-word:phone-to-word.o test-phone-to-word.o
	$(GXX) $(CXXFLAGS) -o $@ $^ $(LIB)

libalign.a:phone-to-word.o
	$(AR) -rc $@ $^

%.o:%.cpp
	$(GXX) $(CXXFLAGS) -c -o $@ $< 

.PHONY:

clean:
	rm -f *.o $(TESTBIN) libalign.a

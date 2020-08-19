
include ../hubo.mk

CXXFLAGS+=-I. 

LIBS=../util/libutil.a

TESTFILES=test-config-parse-options test-text-util hash-list-test log-message-test hash-key-test

OBJFILES=config-parse-options.o text-util.o io-funcs.o read-vector.o log-message.o

all:libutil.a 

test:$(LIBS) $(TESTFILES)

test-config-parse-options:test-config-parse-options.o 
	$(GXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

test-text-util:test-text-util.o 
	$(GXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

hash-list-test:hash-list-test.o
	$(GXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

log-message-test:log-message-test.o 
	$(GXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

hash-key-test:hash-key-test.o
	$(GXX) $(CXXFLAGS) -o $@ $^

%.o:%.cpp
	$(GXX) $(CXXFLAGS) -c -o $@ $<

libutil.a:$(OBJFILES)
	$(AR) -rc $@ $^

.PHONY:

clean:
	rm -f *.o $(TESTFILES) $(LIBS)

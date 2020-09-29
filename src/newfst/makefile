


include ../hubo.mk


CXXFLAGS+=

TESTFILES=test-connect-fst lattice-test test-code test-heap

LIBS=../newfst/liblatfst.a ../util/libutil.a

OBJ=lattice-fst.o lattice-functions.o lattice-to-nbest.o \
	dfs-visit-fst.o connect-fst.o reverse.o rmepsilon.o invert.o topsort.o \
	lattice-determinize-api.o

all:liblatfst.a 

test:$(TESTFILES)

test-connect-fst:test-connect-fst.o
	$(CXX) -o  $@ $^ $(LIBS)

lattice-test:lattice-test.o
	$(CXX) -o  $@ $^ $(LIBS)

test-code:test-code.o
	$(CXX) -o  $@ $^

test-heap:test-heap.o
	$(CXX) -o  $@ $^

liblatfst.a:$(OBJ)
	$(AR) -rcu $@ $^

%.o:%.c
	$(CXX) $(CXXFLAGS) -c  -o $@ $<

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c  -o $@ $<

%.o:%.cc
	$(CXX) $(CXXFLAGS) -c  -o $@ $<


.PHONY:

clean:
	rm -f *.o $(TESTFILES) liblatfst.a

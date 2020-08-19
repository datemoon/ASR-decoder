


include ../hubo.mk


CXXFLAGS+=-I../

TESTFILES=test-connect-fst lattice-test

LIBS=../fst/liblatfst.a ../util/libutil.a

OBJ=lattice-fst.o lattice-functions.o lattice-to-nbest.o \
	dfs-visit-fst.o connect-fst.o reverse.o rmepsilon.o invert.o topsort.o \
	compose-lat.o

all:liblatfst.a 

test:$(TESTFILES)

test-connect-fst:test-connect-fst.o
	$(CXX) $(CXXFLAGS) -o  $@ $^ $(LIBS)

lattice-test:lattice-test.o
	$(CXX) $(CXXFLAGS) -o  $@ $^ $(LIBS)

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

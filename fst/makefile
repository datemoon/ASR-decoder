


include ../hubo.mk


CXXFLAGS+=-I../

BIN=test-connect-fst

LIBS=../fst/liblatfst.a


OBJ=lattice-fst.o lattice-functions.o lattice-to-nbest.o \
	dfs-visit-fst.o connect-fst.o reverse.o rmepsilon.o invert.o topsort.o \
	compose-lat.o

all:liblatfst.a $(BIN)


test-connect-fst:test-connect-fst.o
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
	rm -f *.o $(BIN) liblatfst.a




include ../hubo.mk


CXXFLAGS+=-I../

TESTFILES=service client

LIBS=../util/libutil.a ../service/libservice.a

OBJ=thread-pool.o

all:libservice.a 

test:$(TESTFILES)

service:service.o
	$(CXX) $(CXXFLAGS) -o  $@ $^ $(LIBS)

client:client.o
	$(CXX) $(CXXFLAGS) -o  $@ $^ $(LIBS)

libservice.a:$(OBJ)
	$(AR) -rcu $@ $^

%.o:%.c
	$(CXX) $(CXXFLAGS) -c  -o $@ $<

%.o:%.cc
	$(CXX) $(CXXFLAGS) -c  -o $@ $<


.PHONY:

clean:
	rm -f *.o $(TESTFILES) libservice.a

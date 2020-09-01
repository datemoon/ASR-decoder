include ../hubo.mk


TESTFILES=testv1-service test-client test-service net-data-package-test 

LIBS=../util/libutil.a ../service2/libservice2.a

OBJ=thread-class.o net-data-package.o \
	pthread-util.o

LIBNAME=libservice2.a

all:$(LIBNAME)

test:$(TESTFILES)

testv1-service:testv1-service.o
	$(CXX) $(CXXFLAGS) -o  $@ $^ $(LIBS)

test-service:test-service.o
	$(CXX) $(CXXFLAGS) -o  $@ $^ $(LIBS)

test-client:test-client.o
	$(CXX) $(CXXFLAGS) -o  $@ $^ $(LIBS)

net-data-package-test:net-data-package-test.o
	$(CXX) $(CXXFLAGS) -o  $@ $^ $(LIBS)

$(LIBNAME):$(OBJ)
	$(AR) -rcu $@ $^

%.o:%.c
	$(CXX) $(CXXFLAGS) -c  -o $@ $<

%.o:%.cc
	$(CXX) $(CXXFLAGS) -c  -o $@ $<

.PHONY:

clean:
	rm -f *.o $(TESTFILES) $(LIBNAME)

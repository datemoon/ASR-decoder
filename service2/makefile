


include ../hubo.mk


CXXFLAGS+=-I../

TESTFILES=service client service1 net-data-package-test

LIBS=../util/libutil.a ../service2/libservice2.a

OBJ=thread-class.o work-thread.o net-data-package.o

#thread-pool.o 

LIBNAME=libservice2.a
all:$(LIBNAME)

test:$(TESTFILES)

service:service.o
	$(CXX) $(CXXFLAGS) -o  $@ $^ $(LIBS)

service1:service1.o
	$(CXX) $(CXXFLAGS) -o  $@ $^ $(LIBS)

client:client.o
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

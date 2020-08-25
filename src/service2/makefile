include ../hubo.mk


TESTFILES=testv1-service client test-service net-data-package-test \
		  asr-service asr-client asr-client-v1 asr-client-v2

LIBS=../util/libutil.a ../service2/libservice2.a

OBJ=thread-class.o test-work-thread.o net-data-package.o \
	asr-work-thread.o asr-source.o
#thread-pool.o 

LIBNAME=libservice2.a
all:$(LIBNAME)

test:$(TESTFILES)

testv1-service:testv1-service.o
	$(CXX) $(CXXFLAGS) -o  $@ $^ $(LIBS)

test-service:test-service.o
	$(CXX) $(CXXFLAGS) -o  $@ $^ $(LIBS)

client:client.o
	$(CXX) $(CXXFLAGS) -o  $@ $^ $(LIBS)

asr-client:asr-client.o
	$(CXX) $(CXXFLAGS) -o  $@ $^ $(LIBS)

asr-client-v1:asr-client-v1.o
	$(CXX) $(CXXFLAGS) -o  $@ $^ $(LIBS)

asr-client-v2:asr-client-v2.o
	$(CXX) $(CXXFLAGS) -o  $@ $^ $(LIBS)

net-data-package-test:net-data-package-test.o
	$(CXX) $(CXXFLAGS) -o  $@ $^ $(LIBS)

asr-service:asr-service.o
	$(CXX) $(CXXFLAGS) -o  $@ $^ $(LIBS) $(KALDI_LIB_DYNAMIC) $(FST_LIB) $(CBLAS_LIB)
	#$(CXX) $(CXXFLAGS) -o  $@ $^ $(LIBS) $(KALDI_LIB_STATIC) $(FST_LIB) $(CBLAS_LIB)

$(LIBNAME):$(OBJ)
	$(AR) -rcu $@ $^

%.o:%.c
	$(CXX) $(CXXFLAGS) -c  -o $@ $<

%.o:%.cc
	$(CXX) $(CXXFLAGS) -c  -o $@ $<

.PHONY:

clean:
	rm -f *.o $(TESTFILES) $(LIBNAME)

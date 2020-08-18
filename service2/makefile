


include ../hubo.mk

KALDI_ROOT=/home/hubo/git/github-online/github/kaldi/src
OPENFSTINC = /home/hubo/git/github-online/github/kaldi/tools/openfst-1.6.7/include
ATLASINC=/home/hubo/git/github-online/github/kaldi/tools/ATLAS_headers/include
CXXFLAGS+=-I../ -I$(KALDI_ROOT) -I$(OPENFSTINC) -I$(ATLASINC)

TESTFILES=service client test-service net-data-package-test

LIBS=../util/libutil.a ../service2/libservice2.a

OBJ=thread-class.o test-work-thread.o net-data-package.o \
	asr-work-thread.o 

#thread-pool.o 

LIBNAME=libservice2.a
all:$(LIBNAME)

test:$(TESTFILES)

service:service.o
	$(CXX) $(CXXFLAGS) -o  $@ $^ $(LIBS)

test-service:test-service.o
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

include ../hubo.mk

KALDI_ROOT=/home/hubo/git/github-online/github/kaldi/src
OPENFSTINC = /home/hubo/git/github-online/github/kaldi/tools/openfst-1.6.7/include
ATLASINC=/home/hubo/git/github-online/github/kaldi/tools/ATLAS_headers/include

CXXFLAGS+= -I$(KALDI_ROOT) -I$(OPENFSTINC) -I$(ATLASINC)  -Wl,-rpath=/home/hubo/git/github-online/github/kaldi/tools/openfst-1.6.7/lib

FST_LIB=/home/hubo/git/github-online/github/kaldi/tools/openfst-1.6.7/lib/libfst.a
CBLAS_LIB=/usr/lib64/atlas/libsatlas.so.3 /usr/lib64/atlas/libtatlas.so.3 -Wl,-rpath=/usr/lib64/atlas

KALDI_LIB_DYNAMIC=$(KALDI_ROOT)/online2/libkaldi-online2.so  $(KALDI_ROOT)/ivector/libkaldi-ivector.so \
		  $(KALDI_ROOT)/nnet3/libkaldi-nnet3.so  $(KALDI_ROOT)/chain/libkaldi-chain.so \
		  $(KALDI_ROOT)/nnet2/libkaldi-nnet2.so  $(KALDI_ROOT)/cudamatrix/libkaldi-cudamatrix.so \
		  $(KALDI_ROOT)/decoder/libkaldi-decoder.so  $(KALDI_ROOT)/lat/libkaldi-lat.so \
		  $(KALDI_ROOT)/fstext/libkaldi-fstext.so  $(KALDI_ROOT)/hmm/libkaldi-hmm.so \
		  $(KALDI_ROOT)/feat/libkaldi-feat.so  $(KALDI_ROOT)/transform/libkaldi-transform.so \
		  $(KALDI_ROOT)/gmm/libkaldi-gmm.so  $(KALDI_ROOT)/tree/libkaldi-tree.so \
		  $(KALDI_ROOT)/util/libkaldi-util.so  $(KALDI_ROOT)/matrix/libkaldi-matrix.so \
		  $(KALDI_ROOT)/base/libkaldi-base.so
KALDI_LIB_STATIC=$(KALDI_ROOT)/online2/kaldi-online2.a $(KALDI_ROOT)/ivector/kaldi-ivector.a \
		  $(KALDI_ROOT)/nnet3/kaldi-nnet3.a $(KALDI_ROOT)/chain/kaldi-chain.a $(KALDI_ROOT)/nnet2/kaldi-nnet2.a \
		  $(KALDI_ROOT)/cudamatrix/kaldi-cudamatrix.a $(KALDI_ROOT)/decoder/kaldi-decoder.a \
		  $(KALDI_ROOT)/lat/kaldi-lat.a $(KALDI_ROOT)/fstext/kaldi-fstext.a $(KALDI_ROOT)/hmm/kaldi-hmm.a \
		  $(KALDI_ROOT)/feat/kaldi-feat.a $(KALDI_ROOT)/transform/kaldi-transform.a \
		  $(KALDI_ROOT)/gmm/kaldi-gmm.a $(KALDI_ROOT)/tree/kaldi-tree.a $(KALDI_ROOT)/util/kaldi-util.a \
		  $(KALDI_ROOT)/matrix/kaldi-matrix.a $(KALDI_ROOT)/base/kaldi-base.a

TESTFILES=testv1-service client test-service net-data-package-test \
		  asr-service

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

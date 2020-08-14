
include ../hubo.mk

CXXFLAGS+=-I ../ \
		 -DCBLAS  #-DDEBUGDNNOUT #-DDEBUGFEAT #-DDEBUGDNNOUT #-DDEBUGPITCH #-DDEBUG #-DDEBUGGRAPH

CXXFLAGS += -fPIC


LDFLAGS = -rdynamic

LDLIBS = -lm -lpthread -ldl


LIBS = ../pitch/libpitch.a ../nnet/libnnet.a ../hmm/libhmm.a ../util/libutil.a \
	   $(CBLASLIB) $(FEATLIB)

OBJ=nnet-layer.o matrix.o nnet-component.o nnet-nnet.o \
	nnet-feature-api.o lstm-layer.o nnet-util.o \
	cudnn-lstm-layer.o nnet-feature-pitch-api.o \
	lstm-projected-layer.o nnet-simple-recurrent.o \
	tf-lstm-layer.o

BIN = nnetread nnet-feature-api-test forward-test \
	  kaldi-forward-test nnet-read-test pitch-forward-test \
	  stream-kaldi-forward-test kaldi-format-pitch-forward-test


all: libnnet.a $(BIN)

nnetread:nnetread.o 
	$(GXX) -g -o $@ $^ $(LIBS) $(LDLIBS)

nnet-feature-api-test:nnet-feature-api-test.o nnet-feature-api.o nnet-util.o
	$(GXX) -g -o $@ $^ $(FEATLIB)

pitch-forward-test:pitch-forward-test.o
	$(GXX) -g -o $@ $^ $(LIBS) $(LDLIBS)

kaldi-format-pitch-forward-test:kaldi-format-pitch-forward-test.o
	$(GXX) -g -o $@ $^ $(LIBS) $(LDLIBS)

forward-test:forward-test.o 
	$(GXX) -g -o $@ $^ $(LIBS) $(LDLIBS)

nnet-read-test:nnet-read-test.o
	 $(GXX) -g -o $@ $^ $(LIBS) $(LDLIBS)

#kaldi-forward-test:kaldi-forward-test.o \
#	nnet-layer.o matrix.o nnet-component.o nnet-nnet.o \
#	nnet-feature-api.o lstm-layer.o nnet-util.o cudnn-lstm-layer.o

kaldi-forward-test:kaldi-forward-test.o 
	$(GXX) -g -o $@ $^ $(LIBS) $(LDLIBS)

stream-kaldi-forward-test:stream-kaldi-forward-test.o
	$(GXX) -g -o $@ $^ $(LIBS) $(LDLIBS)

libnnet.a:$(OBJ)
	$(AR) -cru $@ $^

%.o:%.cpp
	$(GXX) $(CXXFLAGS) -o $@ -c $< $(LDLIBS)

%.o:%.cc
	$(GXX) $(CXXFLAGS) -o $@ -c $< $(LDLIBS)


.PHONY:

clean:
	rm -f *.o $(BIN) libnnet.a

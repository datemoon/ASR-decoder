
include ../hubo.mk

LIBS=../kaldi-nnet3/libonlinedecode.a \
	 ../decoder/liboptimizedecode.a ../newfst/liblatfst.a ../util/libutil.a

OBJ=kaldi-online-nnet3-my-decoder-bin.o

BIN=kaldi-online-nnet3-my-decoder-bin

all:$(BIN)

kaldi-online-nnet3-my-decoder-bin:kaldi-online-nnet3-my-decoder-bin.o
	$(GXX) $(CXXFLAGS) -o $@ $^ $(LIB) $(LIBS) $(KALDI_LIB_DYNAMIC) $(CBLAS_LIB) 
%.o:%.cc
	$(GXX) $(CXXFLAGS) -c -o $@ $< 

.PHONY:

clean:
	rm -f *.o kaldi-online-nnet3-my-decoder-bin

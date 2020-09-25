
include ../hubo.mk

LIBS=../kaldi-nnet3/libonlinedecode.a \
	 ../decoder/liboptimizedecode.a \
	 ../newlm/liblm.a \
	 ../newfst/liblatfst.a \
	 ../util/libutil.a 

OBJ=kaldi-online-nnet3-my-decoder-bin.o

BIN=kaldi-online-nnet3-my-decoder-bin kaldi-my-decoder kaldi-hclg-my-decoder \
	kaldi-hclg-my-decoder-biglm

all:$(BIN)

kaldi-hclg-my-decoder-biglm:kaldi-hclg-my-decoder-biglm.o
	$(GXX) $(CXXFLAGS) -o $@ $^ $(LIB) $(LIBS) $(KALDI_LIBS) $(CBLAS_LIB)

kaldi-online-nnet3-my-decoder-bin:kaldi-online-nnet3-my-decoder-bin.o
	$(GXX) $(CXXFLAGS) -o $@ $^ $(LIB) $(LIBS) $(KALDI_LIBS) $(CBLAS_LIB)

kaldi-my-decoder:kaldi-my-decoder.o
	$(GXX) $(CXXFLAGS) -o $@ $^ $(LIB) $(LIBS)  $(KALDI_LIBS) $(CBLAS_LIB)

kaldi-hclg-my-decoder:kaldi-hclg-my-decoder.o
	$(GXX) $(CXXFLAGS) -o $@ $^ $(LIB) $(LIBS)  $(KALDI_LIBS) $(CBLAS_LIB)

%.o:%.cc
	$(GXX) $(CXXFLAGS) -c -o $@ $< 

.PHONY:

clean:
	rm -f *.o $(BIN)


include ../hubo.mk

CXXFLAGS+=

OBJ=kaldi-vector.o    pitch-functions.o \
	online-feature.o  resample.o kaldi-matrix.o kaldi-common.o

BIN=compute-and-process-kaldi-pitch-feats stream-compute-and-process-kaldi-pitch-feats \
	hb-stream-compute-and-process-kaldi-pitch-feats

LIB=libpitch.a ../util/libutil.a \
	$(CBLASLIB)

all:libpitch.a 

test:$(BIN)


libpitch.a:$(OBJ)
	$(AR) -rc $@ $^

compute-and-process-kaldi-pitch-feats:compute-and-process-kaldi-pitch-feats.o
	$(GXX) $(CXXFLAGS) -o $@ $^ $(LIB) -lm -pthread -ldl

stream-compute-and-process-kaldi-pitch-feats:stream-compute-and-process-kaldi-pitch-feats.o
	$(GXX) $(CXXFLAGS) -o $@ $^ $(LIB) -lm -pthread -ldl

hb-stream-compute-and-process-kaldi-pitch-feats:hb-stream-compute-and-process-kaldi-pitch-feats.o
	$(GXX) $(CXXFLAGS) -o $@ $^ $(LIB) -lm -pthread -ldl

%.o:%.cc
	$(GXX) $(CXXFLAGS) -o $@ -c  $<

.PHONY:
clean:
	rm -rf *.o libpitch.a $(BIN)

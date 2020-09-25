include ../hubo.mk

BIN=asr-service asr-client asr-client-v1 asr-client-v2 \
	v2-asr-service thread-client

LIBS=../v2-asr/libv2asr.a ../v1-asr/libv1asr.a \
	 ../kaldi-nnet3/libonlinedecode.a \
	 ../service2/libservice2.a \
	 ../decoder/liboptimizedecode.a \
	 ../newlm/liblm.a \
	 ../newfst/liblatfst.a \
	 ../util/libutil.a

OBJ=

LIBNAME=libservice2.a
all:$(BIN)


asr-client:asr-client.o
	$(CXX) $(CXXFLAGS) -o  $@ $^ $(LIBS)

thread-client:thread-client.o
	$(CXX) $(CXXFLAGS) -o  $@ $^ $(LIBS)

asr-client-v1:asr-client-v1.o
	$(CXX) $(CXXFLAGS) -o  $@ $^ $(LIBS)

asr-client-v2:asr-client-v2.o
	$(CXX) $(CXXFLAGS) -o  $@ $^ $(LIBS)

asr-service:asr-service.o
	$(CXX) $(CXXFLAGS) -o  $@ $^ $(LIBS) $(KALDI_LIBS) $(CBLAS_LIB)

v2-asr-service:v2-asr-service.o
	$(CXX) $(CXXFLAGS) -o  $@ $^ $(LIBS) $(KALDI_LIBS) $(CBLAS_LIB)

%.o:%.c
	$(CXX) $(CXXFLAGS) -c  -o $@ $<

%.o:%.cc
	$(CXX) $(CXXFLAGS) -c  -o $@ $<

.PHONY:
clean:
	rm -f *.o $(BIN) $(LIBNAME)

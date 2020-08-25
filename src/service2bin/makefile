include ../hubo.mk


BIN=asr-service asr-client asr-client-v1 asr-client-v2

LIBS=../util/libutil.a ../service2/libservice2.a

OBJ=

LIBNAME=libservice2.a
all:$(BIN)

asr-client:asr-client.o
	$(CXX) $(CXXFLAGS) -o  $@ $^ $(LIBS)

asr-client-v1:asr-client-v1.o
	$(CXX) $(CXXFLAGS) -o  $@ $^ $(LIBS)

asr-client-v2:asr-client-v2.o
	$(CXX) $(CXXFLAGS) -o  $@ $^ $(LIBS)

asr-service:asr-service.o
	$(CXX) $(CXXFLAGS) -o  $@ $^ $(LIBS) $(KALDI_LIB_DYNAMIC) $(FST_LIB) $(CBLAS_LIB)
	#$(CXX) $(CXXFLAGS) -o  $@ $^ $(LIBS) $(KALDI_LIB_STATIC) $(FST_LIB) $(CBLAS_LIB)

%.o:%.c
	$(CXX) $(CXXFLAGS) -c  -o $@ $<

%.o:%.cc
	$(CXX) $(CXXFLAGS) -c  -o $@ $<

.PHONY:

clean:
	rm -f *.o $(BIN) $(LIBNAME)

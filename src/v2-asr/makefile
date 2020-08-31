include ../hubo.mk


TESTFILES=

LIBS=../util/libutil.a ../service2/libservice2.a

OBJ=v2-asr-work-thread.o asr-client-task.o

LIBNAME=libv2asr.a

all:$(LIBNAME)

test:$(TESTFILES)

$(LIBNAME):$(OBJ)
	$(AR) -rcu $@ $^

%.o:%.c
	$(CXX) $(CXXFLAGS) -c  -o $@ $<

%.o:%.cc
	$(CXX) $(CXXFLAGS) -c  -o $@ $<

.PHONY:

clean:
	rm -f *.o $(TESTFILES) $(LIBNAME)

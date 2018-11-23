


include ../hubo.mk

#GCC=gcc

#CFLAGS=-g

all:main readhmm

main: read_fst.o main.o write_fst.o
	$(GCC) $(CFLAGS) -o $@ $^

readhmm:readhmm.o read_fst.o write_fst.o
	$(GCC) $(CFLAGS) -o $@ $^

%.o:%.c
	$(GCC) $(CFLAGS) -c $^

.PHONY:clean

clean:
	rm -f *.o main readhmm

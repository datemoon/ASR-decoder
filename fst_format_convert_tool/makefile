


include ../hubo.mk

#GCC=gcc

#CFLAGS=-g

all:main

main: read_fst.o main.o write_fst.o
	$(GCC) $(CFLAGS) -o $@ $^

%.o:%.c
	$(GCC) $(CFLAGS) -c $^

.PHONY:clean

clean:
	rm -f *.o main

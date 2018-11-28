


CXXFLAGS=-msse -msse2 -Wall -I.. -pthread \
		 -std=c++0x -fpermissive  -Wunused-but-set-variable \
		 -O3 -DNDEBUG #-gstabs+ #-DDEBUGTOKEN -g #-pg #-DDEBUG #-DDEBUGGRAPH

CXXFLAGS += -fPIC


LDFLAGS = -rdynamic

CBLASLIB=../lib/libcblas_linux.a \
		 ../lib/libatlas_linux.a

FEATLIB=../lib/libfrontend.a

LDLIBS = -lm -lpthread -ldl


GCC=gcc
CC = g++
CXX = g++
GXX = g++
AR = ar
AS = as
RANLIB = ranlib

ifeq ($(ARM),true)
include ../arm.mk
endif


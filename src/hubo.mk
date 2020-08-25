
CXXFLAGS=-std=c++11 -Wall -Wno-sign-compare \
		 -Wno-unused-local-typedefs -Wno-deprecated-declarations \
		 -Winit-self \
		 -msse -msse2 \
		 -pthread \
		 -O2 #-gdwarf-2 #-gstabs+ #-DDEBUGTOKEN -g #-pg #-DDEBUG #-DDEBUGGRAPH

CXXFLAGS += -fPIC -I../..
CXXFLAGS += -DNAMESPACE

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

ifeq ($(KALDI),true)
include ../kaldi.mk
endif

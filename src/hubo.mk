
CXXFLAGS=-Wall -Wno-sign-compare \
		 -Wno-unused-local-typedefs -Wno-deprecated-declarations \
		 -Winit-self \
		 -msse -msse2 \
		 -pthread \
		 -std=c++11 \
		 -O3 #-gdwarf-2 #-gstabs+ #-DDEBUGTOKEN -g #-pg #-DDEBUG #-DDEBUGGRAPH

CXXFLAGS += -fPIC -I../..
CXXFLAGS += -DNAMESPACE
#CXXFLAGS += -DUSE_CLG

LDFLAGS = -rdynamic

CBLASLIB=../lib/libcblas_linux.a \
		 ../lib/libatlas_linux.a

FEATLIB=../lib/libfrontend.a

LDLIBS = -lm -lpthread -ldl

GCC=gcc
CC = g++
CXX = $(CC)
GXX = $(CC)
AR = ar
AS = as
RANLIB = ranlib

ifeq ($(ARM),true)
include ../arm.mk
endif

ifeq ($(KALDI),true)
include ../kaldi.mk
endif


WORK_DIR=`pwd`
#KALDI_DIR_NAME=kaldi-arm
#ATLAS_PATH=$WORK_DIR/$KALDI_DIR_NAME/tools/ATLAS/build_pic_fix2/lib

PATH=/opt/android-toolchain-5-arm/bin:$PATH

CROSS=arm-linux-androideabi
CC_=$(CROSS)-gcc
CXX_=$(CROSS)-g++
AR_=$(CROSS)-ar
RANLIB_=$(CROSS)-ranlib
STRIP_=$(CROSS)-strip
LD_=$(CROSS)-ld

CC=$(CROSS)-g++
CXX=$(CROSS)-g++
AR=$(CROSS)-ar
RANLIB=$(CROSS)-ranlib
STRIP=$(CROSS)-strip
LD=$(CROSS)-ld
GCC=$(CROSS)-gcc
GXX=$(CROSS)-g++

CBLASLIB=../lib/libcblas_arm.a \
		 ../lib/libatlas_arm.a

FEATLIB=../lib/libfrontend_arm.a

CXXFLAGS=-I.. -fpermissive -Wall -DKALDI_DOUBLEPRECISION=0 -DHAVE_MEMALIGN -Wno-sign-compare -Winit-self -rdynamic -DHAVE_CXXABI_H -DHAVE_ATLAS -O3 -mcpu=cortex-a8 -mtune=cortex-a8 -mfpu=neon -mfloat-abi=softfp -fno-expensive-optimizations -std=c++11 -fpermissive


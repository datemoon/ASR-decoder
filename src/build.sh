#!/bin/bash
set -x

# 版本代码建议下载kaldi代码commit(90d44fe3a1bce06b60600e68078565e2c0fe393e)
# kaldi依赖的安装文件，可以在项目的kaldi-master.tar的tools目录下的download目录下找到
# 如果想安装gpu版本的识别服务，使用命令
# ./build.sh true
# 使用cpu版本的识别服务
# ./build.sh
# 如果想要编译torch版本的nnet，g++必须支持c++14，需要gcc 5以上版本，GLIBC_2.23以后。
#
# 如果你想编译音频转码目录请添加 --with-ffmpeg 选项到make_opts
#
source ../bashrc

use_gpu=false
add_ffmpeg=false

if [ $# == 1 ]
then
	use_gpu=$1
	echo use gpu decoder
fi

if [ $# == 2 ]
then
	KALDI_ROOT=$2
fi

if [ $use_gpu == true ]
then
	kaldi_opts="--static --use-cuda=yes"
	make_opts="--enable-usecuda=yes --with-cuda=/usr/local/cuda/ --with-atlas=/usr/lib64/atlas --with-kaldi=${KALDI_ROOT}/src "
else
	kaldi_opts="--static --use-cuda=no"
	make_opts="--with-atlas=/usr/lib64/atlas --with-kaldi=${KALDI_ROOT}/src"
fi

if [ $add_ffmpeg == true ]
then
	make_opts+=" --with-ffmpeg=${FFMPEG_ROOT}"
fi

# first build kaldi
#if [ ! -d ../kaldi-master ]
#then
#	cd ../
#	#rm -rf kaldi-master
#	tar -xvf kaldi-master.tar || exit 1;
#	cd kaldi-master/tools
#	make -j 30
#	cd ../src
#	./configure $kaldi_opts
#	sed -i "s:-g$:-O2:g" kaldi.mk
#	make depend -j 30
#	make -j 30
#	cd ../../src
#fi
# this build shell
#sed -i "s:ROOTDIR=.*$:ROOTDIR=\"$PWD/\.\./\":g" configure.ac
autoreconf --install

if [ $use_gpu == true ]
then
	rm -rf build-gpu
	mkdir build-gpu
	cd build-gpu
	mkdir -p $PWD/install

else
	rm -rf build
	mkdir build
	cd build
	mkdir -p $PWD/install
fi
# if you kaldi use cuda and mkl , you must be
../configure --prefix=$PWD/install $make_opts 

make -j 2

make install

tar -cvf install.tar install


#!/bin/bash
set -x

# first build kaldi
if [ ! -d ../kaldi-master ]
then
	cd ../
	#rm -rf kaldi-master
	tar -xvf kaldi-master.tar
	cd kaldi-master/tools
	make -j 30
	cd ../src
	./configure --static --use-cuda=no
	sed -i "s:-g$:-O2:g" kaldi.mk
	make depend -j 30
	make -j 30
	cd ../../src
fi
# this build shell
sed -i "s:KALDI_ROOT=.*$:KALDI_ROOT=$PWD/../kaldi-master/src:g" depend.mk
sed -i "s:ROOTDIR=.*$:ROOTDIR=\"$PWD/\.\./\":g" configure.ac
autoreconf --install

rm -rf build
mkdir build
cd build
mkdir -p $PWD/install
# if you kaldi use cuda and mkl , you must be
../configure --prefix=$PWD/install --disable-usecblasnotmkl 

make -j 20

make install

tar -cvf install.tar install


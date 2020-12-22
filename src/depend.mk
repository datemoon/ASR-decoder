#KALDI_ROOT=/home/hu.bo/git/kaldi/src
KALDI_ROOT=/home/hubo/git/github-online/github/kaldi/src

# use cblas or mkl
if USECBLAS
MATRIX_DEPLDFLAGS = -Wl,-rpath=/usr/lib64/atlas
MATRIX_INC = $(KALDI_ROOT)/../tools/ATLAS_headers/include
MATRIX_LIBS = /usr/lib64/atlas/libsatlas.so.3 \
			  /usr/lib64/atlas/libtatlas.so.3 
else
MATRIX_INC = /opt/intel/mkl/include
MATRIX_DEPLDFLAGS = -Wl,-rpath=/opt/intel/mkl/lib/intel64 

#MATRIX_LIBS =  -Wl,--start-group /opt/intel/mkl/lib/intel64/libmkl_intel_lp64.a /opt/intel/mkl/lib/intel64/libmkl_core.a /opt/intel/mkl/lib/intel64/libmkl_sequential.a -Wl,--end-group    -ldl -lpthread -lm
AM_LIBTOOLFLAGS = --preserve-dup-deps 
MATRIX_LIBS = /opt/intel/mkl/lib/intel64/libmkl_intel_lp64.a \
			  /opt/intel/mkl/lib/intel64/libmkl_core.a \
			  /opt/intel/mkl/lib/intel64/libmkl_sequential.a \
			  /opt/intel/mkl/lib/intel64/libmkl_intel_lp64.a \
			  /opt/intel/mkl/lib/intel64/libmkl_core.a \
			  /opt/intel/mkl/lib/intel64/libmkl_sequential.a \
			  /opt/intel/mkl/lib/intel64/libmkl_intel_lp64.a \
			  /opt/intel/mkl/lib/intel64/libmkl_core.a \
			  /opt/intel/mkl/lib/intel64/libmkl_sequential.a \
			  -ldl -lpthread -lm

#MATRIX_LIBS = -L/opt/intel/mkl/lib/intel64 \
#			  -lmkl_intel_lp64  -lmkl_core  -lmkl_sequential  

endif

# use cuda
CUDA_INC = /usr/local/cuda/include
if USECUDA
CUDA_DEPLDFLAGS = -L/usr/local/cuda/lib64 -Wl,-rpath,/usr/local/cuda/lib64 
CUDA_LIBS = -lcublas -lcusparse -lcudart -lcurand -lcufft -lnvToolsExt  -lcusolver

else
CUDA_DEPLDFLAGS = 
CUDA_LIBS =
endif

# use kaldi
OPENFST_INC = $(KALDI_ROOT)/../tools/openfst-1.6.7/include
KALDI_INC = $(KALDI_ROOT)

if USEKALDISTATIC
FST_LIBS=$(KALDI_ROOT)/../tools/openfst-1.6.7/lib/libfst.a
KALDI_LIBS=$(KALDI_ROOT)/online2/kaldi-online2.a $(KALDI_ROOT)/ivector/kaldi-ivector.a \
		   $(KALDI_ROOT)/nnet3/kaldi-nnet3.a $(KALDI_ROOT)/chain/kaldi-chain.a \
		   $(KALDI_ROOT)/nnet2/kaldi-nnet2.a $(KALDI_ROOT)/cudamatrix/kaldi-cudamatrix.a \
		   $(KALDI_ROOT)/decoder/kaldi-decoder.a $(KALDI_ROOT)/lat/kaldi-lat.a \
		   $(KALDI_ROOT)/fstext/kaldi-fstext.a $(KALDI_ROOT)/hmm/kaldi-hmm.a \
		   $(KALDI_ROOT)/feat/kaldi-feat.a $(KALDI_ROOT)/transform/kaldi-transform.a \
		   $(KALDI_ROOT)/gmm/kaldi-gmm.a $(KALDI_ROOT)/tree/kaldi-tree.a \
		   $(KALDI_ROOT)/util/kaldi-util.a $(KALDI_ROOT)/matrix/kaldi-matrix.a \
		   $(KALDI_ROOT)/base/kaldi-base.a \
		   $(FST_LIBS)
else
FST_LIBS=$(KALDI_ROOT)/../tools/openfst-1.6.7/lib/libfst.so

KALDI_LIBS = $(KALDI_ROOT)/online2/libkaldi-online2.so  $(KALDI_ROOT)/ivector/libkaldi-ivector.so \
			 $(KALDI_ROOT)/nnet3/libkaldi-nnet3.so  $(KALDI_ROOT)/chain/libkaldi-chain.so \
			 $(KALDI_ROOT)/nnet2/libkaldi-nnet2.so  $(KALDI_ROOT)/cudamatrix/libkaldi-cudamatrix.so \
			 $(KALDI_ROOT)/decoder/libkaldi-decoder.so  $(KALDI_ROOT)/lat/libkaldi-lat.so \
			 $(KALDI_ROOT)/fstext/libkaldi-fstext.so  $(KALDI_ROOT)/hmm/libkaldi-hmm.so \
			 $(KADI_ROOT)/feat/libkaldi-feat.so  $(KALDI_ROOT)/transform/libkaldi-transform.so \
			 $(KALDI_ROOT)/gmm/libkaldi-gmm.so  $(KALDI_ROOT)/tree/libkaldi-tree.so \
			 $(KALDI_ROOT)/util/libkaldi-util.so  $(KALDI_ROOT)/matrix/libkaldi-matrix.so \
			 $(KALDI_ROOT)/base/libkaldi-base.so \
			 $(FST_LIBS) 

KALDI_DEPLDFLAGS = -Wl,-rpath=$(KALDI_ROOT)/lib \
   				   -Wl,-rpath=$(KALDI_ROOT)/../tools/openfst-1.6.7/lib 
endif

KALDI_ROOT=/home/hubo/git/github-online/github/kaldi/src
OPENFSTINC = $(KALDI_ROOT)/../tools/openfst-1.6.7/include

FST_LIB_STATIC=$(KALDI_ROOT)/../tools/openfst-1.6.7/lib/libfst.a
FST_LIB_DYNAMIC=$(KALDI_ROOT)/../tools/openfst-1.6.7/lib/libfst.so

ifeq ($(CUDA),true)
ATLASINC=/opt/intel/mkl/include
CUDAINC=/usr/local/cuda/include
CBLAS_LIB=-L/usr/local/cuda/lib64 -Wl,-rpath,/usr/local/cuda/lib64 \
		  -L/opt/intel/mkl/lib/intel64 \
		  -Wl,-rpath=/opt/intel/mkl/lib/intel64 \
		  -lmkl_intel_lp64  -lmkl_core  -lmkl_sequential  \
		  -ldl -lpthread -lm  -lm -lpthread -ldl -lcublas \
		  -lcusparse -lcudart -lcurand -lcufft -lnvToolsExt  -lcusolver

CXXFLAGS+=-I$(CUDAINC)
else
ATLASINC=$(KALDI_ROOT)/../tools/ATLAS_headers/include
CBLAS_LIB=/usr/lib64/atlas/libsatlas.so.3 \
		  /usr/lib64/atlas/libtatlas.so.3 \
		  -Wl,-rpath=/usr/lib64/atlas 
#		  -L/opt/intel/mkl/lib/intel64 \
#		  -Wl,-rpath=/opt/intel/mkl/lib/intel64 \
#		  -lmkl_intel_lp64  -lmkl_core  -lmkl_sequential -ldl
endif


KALDI_LIB_DYNAMIC=$(KALDI_ROOT)/online2/libkaldi-online2.so  $(KALDI_ROOT)/ivector/libkaldi-ivector.so \
				  $(KALDI_ROOT)/nnet3/libkaldi-nnet3.so  $(KALDI_ROOT)/chain/libkaldi-chain.so \
				  $(KALDI_ROOT)/nnet2/libkaldi-nnet2.so  $(KALDI_ROOT)/cudamatrix/libkaldi-cudamatrix.so \
				  $(KALDI_ROOT)/decoder/libkaldi-decoder.so  $(KALDI_ROOT)/lat/libkaldi-lat.so \
				  $(KALDI_ROOT)/fstext/libkaldi-fstext.so  $(KALDI_ROOT)/hmm/libkaldi-hmm.so \
				  $(KALDI_ROOT)/feat/libkaldi-feat.so  $(KALDI_ROOT)/transform/libkaldi-transform.so \
				  $(KALDI_ROOT)/gmm/libkaldi-gmm.so  $(KALDI_ROOT)/tree/libkaldi-tree.so \
				  $(KALDI_ROOT)/util/libkaldi-util.so  $(KALDI_ROOT)/matrix/libkaldi-matrix.so \
				  $(KALDI_ROOT)/base/libkaldi-base.so

KALDI_LIB_STATIC=$(KALDI_ROOT)/online2/kaldi-online2.a $(KALDI_ROOT)/ivector/kaldi-ivector.a \
				 $(KALDI_ROOT)/nnet3/kaldi-nnet3.a $(KALDI_ROOT)/chain/kaldi-chain.a $(KALDI_ROOT)/nnet2/kaldi-nnet2.a \
				 $(KALDI_ROOT)/cudamatrix/kaldi-cudamatrix.a $(KALDI_ROOT)/decoder/kaldi-decoder.a \
				 $(KALDI_ROOT)/lat/kaldi-lat.a $(KALDI_ROOT)/fstext/kaldi-fstext.a $(KALDI_ROOT)/hmm/kaldi-hmm.a \
				 $(KALDI_ROOT)/feat/kaldi-feat.a $(KALDI_ROOT)/transform/kaldi-transform.a \
				 $(KALDI_ROOT)/gmm/kaldi-gmm.a $(KALDI_ROOT)/tree/kaldi-tree.a $(KALDI_ROOT)/util/kaldi-util.a \
				 $(KALDI_ROOT)/matrix/kaldi-matrix.a $(KALDI_ROOT)/base/kaldi-base.a

KALDI_LIBS=$(KALDI_LIB_DYNAMIC) $(FST_LIB_DYNAMIC)

#KALDI_LIBS=$(KALDI_LIB_STATIC) $(FST_LIB_STATIC)

CXXFLAGS+= -I$(KALDI_ROOT) -I$(OPENFSTINC) -I$(ATLASINC) \
		   -Wl,-rpath=$(KALDI_ROOT)/../tools/openfst-1.6.7/lib \
		   -Wl,-rpath=$(KALDI_ROOT)/lib 

CXXFLAGS+= -DKALDI


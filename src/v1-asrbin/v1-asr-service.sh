#!/bin/bash

set -x

#./bin/v1-asr-service --config=conf/conf.txt \
#	--use-energy-vad=false --energy-vad-config=conf/energy-vad.conf \
#	--use-model-vad=true --nnet-vad-config=conf/nnet3-vad.conf \
#	--vad-model-filename=vad-nnet3-model/final.mdl \
#	--fbank-config=conf/fbank.80.conf \
#	--config-socket=conf/socket.conf \
#	final.mdl HCLG.fst words.txt


# 使用模型vad
./v1-asr-service --config=conf/conf.txt \
	--use-model-vad=false --nnet-vad-config=conf/nnet3-vad.conf \
	--vad-model-filename=vad-nnet3-model/final.mdl \
	--fbank-config=conf/fbank.80.conf \
	--config-socket=conf/socket.conf \
	--nthread=60 \
	source/final.mdl source/HCLG.fst source/words.txt

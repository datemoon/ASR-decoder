#!/bin/bash


set -x
if [ $# != 1 ]
then
	echo $0 scpfile
	exit 1;
fi

./new-online-energy-vad-stream-test --use-realtime-vad=false \
	--print-vad-ali-info=true \
	--max-nosil-frames=-1 \
	--verbose=3 \
	scp:$1

feat_conf=fbank.80.conf
model=nnet.model
scp=scp

./online-nnet3-vad-stream-test --vad-nnet.acoustic-scale=1.0 --apply-exp=true --use-priors=false  \
	--verbose=3 --fbank-config=$feat_conf  --feature-type=fbank \
	$model scp:$scp

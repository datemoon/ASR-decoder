#!/bin/bash

set -x

if [ $# != 1 ]
then
	echo $0 modeldir
	exit 1;
fi
#am=tdnnf_fbank80_train_5kh_aishell_add_10epoch/am/final.mdl
#hclg=tdnnf_fbank80_train_5kh_aishell_add_10epoch/HCLG-1e-9-modefy-lex-v7-cvte-manual-v2/HCLG.fst
dir=$1
am=$dir/final.mdl
hclg=$dir/HCLG.fst
words=$dir/words.txt
export LD_LIBRARY_PATH=$PWD/lib
export CUDA_DEVICE_ORDER=PCI_BUS_ID
CUDA_VISIBLE_DEVICES="0" ./bin/v1-gpu-asr-service --config=conf/config.txt --word-symbol-table=$words \
	$am $hclg

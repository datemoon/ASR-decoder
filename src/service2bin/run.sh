#!/bin/bash

set -x 
export LD_LIBRARY_PATH=$PWD/lib

./bin/asr-service --config=conf/conf.txt final.mdl HCLG.fst words.txt

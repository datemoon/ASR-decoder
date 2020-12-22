SUBDIRS = fst_format_convert_tool \
		  util align \
		  newfst newlm \
		  pitch hmm service nnet \
		  decoder \
		  service2 \
		  kaldi-nnet3 kaldi-nnet3bin \
		  v1-asr v2-asr \
		  service2bin \
		  client \
		  vad \
		  v1-asrbin

lib : util align newfst newlm decoder service2 kaldi-nnet3 v1-asr v2-asr vad client \
	fst_format_convert_tool pitch hmm service nnet

kaldi-nnet3bin service2bin v1-asrbin : lib

include_HEADERS = client/asr-client-api.h
confdir=$(prefix)/conf
dist_conf_DATA = v1-asrbin/conf/conf.txt \
				 v1-asrbin/conf/decoder.conf \
				 v1-asrbin/conf/fbank.80.conf \
				 v1-asrbin/conf/socket.conf \
				 v1-asrbin/conf/nnet3-vad.conf \
				 v1-asrbin/conf/energy-vad.conf

#service2bin/conf/conf.txt \
#			   	service2bin/conf/socket.conf \
#				service2bin/conf/decoder.conf \
#			   	service2bin/conf/v2-conf.txt \
#			   	service2bin/conf/fbank.80.conf \
#

installdir=$(prefix)
dist_install_DATA= service2bin/run.sh \
				   v1-asrbin/v1-asr-service.sh v1-asrbin/README

ACLOCAL_AMFLAGS = -I m4





SUBDIRS=util hmm align lib pitch nnet \
	newfst newlm \
	decoder fst_format_convert_tool \
	service2 kaldi-nnet3 v2-asr v1-asr

SUBDIRSBIN=service2bin kaldi-nnet3bin 

all:ex exbin

ex:
	-for x in $(SUBDIRS) ; do make -C $$x; done

exbin:
	-for x in $(SUBDIRSBIN) ; do make -C $$x; done

.PHONY:
clean:
	-for x in $(SUBDIRS) $(SUBDIRSBIN) ; do make clean -C $$x; done





SUBDIRS=util hmm align lib pitch nnet \
	newfst newlm \
	decoder fst_format_convert_tool \
	service2 kaldi-nnet3

all:ex

ex:
	-for x in $(SUBDIRS) ; do make -C $$x; done

.PHONY:
clean:
	-for x in $(SUBDIRS) ; do make clean -C $$x; done



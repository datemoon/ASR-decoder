



SUBDIRS=util hmm align lib pitch nnet \
	fst lm \
	decoder fst_format_convert_tool \
	service2

all:ex

ex:
	-for x in $(SUBDIRS) ; do make -C $$x; done

.PHONY:
clean:
	-for x in $(SUBDIRS) ; do make clean -C $$x; done



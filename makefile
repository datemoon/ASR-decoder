



SUBDIRS=util hmm align lib pitch nnet \
	fst decoder fst_format_convert_tool 

all:ex

ex:
	-for x in $(SUBDIRS) ; do make -C $$x; done

.PHONY:
clean:
	-for x in $(SUBDIRS) ; do make clean -C $$x; done



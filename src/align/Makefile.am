
lib_LTLIBRARIES = libalign.la

libalign_la_SOURCES = phone-to-word.cc

if HAVE_TEST

LDADD = ../align/libalign.la ../util/libutil.la 

bin_PROGRAMS = test-phone-to-word

test_phone_to_word_SOURCES = test-phone-to-word.cc

endif

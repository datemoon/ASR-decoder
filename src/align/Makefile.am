
#lib_LTLIBRARIES = libalign.la
noinst_LTLIBRARIES = libalign.la

libalign_la_SOURCES = phone-to-word.cc

check_PROGRAMS = test-phone-to-word test-align-info

test_phone_to_word_LDADD = ../align/libalign.la ../util/libutil.la 
test_phone_to_word_SOURCES = test-phone-to-word.cc

test_align_info_LDADD = ../util/libutil.la
test_align_info_SOURCES = test-align-info.cc

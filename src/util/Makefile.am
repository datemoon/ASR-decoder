

AM_CPPFLAGS = 

lib_LTLIBRARIES = libutil.la

libutil_la_SOURCES = config-parse-options.cc \
					 text-util.cc \
					 io-funcs.cc \
					 read-vector.cc \
					 log-message.cc

libutil_la_LDFLAGS = -rpath '$(libdir)'

LDADD = ../util/libutil.la

check_PROGRAMS = test-config-parse-options \
			   test-text-util \
			   hash-list-test \
			   log-message-test \
			   hash-key-test

test_config_parse_options_SOURCES = test-config-parse-options.cc
test_text_util_SOURCES = test-text-util.cc
hash_list_test_SOURCES = hash-list-test.cc
log_message_test_SOURCES = log-message-test.cc
hash_key_test_SOURCES = hash-key-test.cc

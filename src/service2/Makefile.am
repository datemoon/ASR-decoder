
lib_LTLIBRARIES = libservice2.la
libservice2_la_SOURCES = thread-class.cc \
						 net-data-package.cc \
						 pthread-util.cc


if HAVE_TEST

LDADD = libservice2.la ../util/libutil.la -lpthread

bin_PROGRAMS = testv1-service \
			   test-client \
			   test-service \
			   net-data-package-test

testv1_service_SOURCES = testv1-service.cc
test_client_SOURCES = test-client.cc
test_service_SOURCES = test-service.cc
net_data_package_test_SOURCES = net-data-package-test.cc

endif

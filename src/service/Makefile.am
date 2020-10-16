
lib_LTLIBRARIES = libservice.la

libservice_la_SOURCES = thread-pool.cc

if HAVE_TEST
LDADD = libservice.la ../util/libutil.la -lpthread
bin_PROGRAMS = service \
			   client

service_SOURCES = service.cc
client_SOURCES = client.cc

endif

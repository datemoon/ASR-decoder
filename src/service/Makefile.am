
lib_LTLIBRARIES = libservice.la

libservice_la_SOURCES = thread-pool.cc

LDADD = libservice.la ../util/libutil.la -lpthread
check_PROGRAMS = service \
			   client

service_SOURCES = service.cc
client_SOURCES = client.cc


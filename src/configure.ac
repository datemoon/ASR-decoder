#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([ASR-decoder], [0.1], [1057277483@qq.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
#AC_CONFIG_HEADERS([config.h])
AM_PROG_AR

CXXFLAGS="-Wall -Wno-sign-compare -Wno-unused-local-typedefs -Wno-deprecated-declarations -Winit-self -msse -msse2 -pthread"
CXXFLAGS="$CXXFLAGS -std=c++11 -O2 -I../.."

# Checks for programs.
AC_PROG_CXX
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile
				 fst_format_convert_tool/Makefile
				 util/Makefile
				 align/Makefile
				 newfst/Makefile
				 newlm/Makefile
				 pitch/Makefile
				 hmm/Makefile
				 decoder/Makefile
				 service/Makefile
				 service2/Makefile
				 nnet/Makefile
				 kaldi-nnet3/Makefile
				 kaldi-nnet3bin/Makefile
				 v1-asr/Makefile
				 v2-asr/Makefile
				 service2bin/Makefile])

AC_CONFIG_MACRO_DIR([m4])

# --enable-namespace set namespace
AC_ARG_ENABLE([namespace],
			  [AS_HELP_STRING([--enable-namespace],[enable add namespcae])],
			  [],
			  [enable_namespace=yes])

# it must be define KALDI and NAMESPACE
if test "x$enable_namespace" != xno ;
then
	CXXFLAGS="$CXXFLAGS -DNAMESPACE -DKALDI"
fi

# --enable-testbin complie test bin.
AC_ARG_ENABLE([testbin],
			  [AS_HELP_STRING([--enable-testbin],[enable test bin])],
			  [],
			  [enable_testbin=yes])

AM_CONDITIONAL([HAVE_TEST], [test "x$enable_testbin" != xno])

AC_ARG_ENABLE([testnnetbin],
			  [AS_HELP_STRING([--enable-testnnetbin],[enable test nnet bin])],
			  [],
			  [enable_testnnetbin=no])

AM_CONDITIONAL([HAVE_TEST_NNET], [test "x$enable_testnnetbin" != xno])

AC_ARG_ENABLE([bin],
			  [AS_HELP_STRING([--enable-bin],[enable command-line binaries])],
			  [],
			  [enable_bin=yes])

AM_CONDITIONAL([HAVE_BIN], [test "x$enable_bin" != xno])

# check kaldi lib
#AC_SUBST([KALDI_LIBS],[""])

# check cblas lib
#AC_SUBST([CBLASLIB],["../lib/libcblas_linux.a ../lib/libatlas_linux.a"])

# check cuda
AC_ARG_ENABLE([usecuda],
			  [AS_HELP_STRING([--enable-usecuda],[enable use cuda])],
			  [],
			  [enable_usecuda=no])

AM_CONDITIONAL([USECUDA], [test "x$enable_usecuda" != xno])

# check cblas lib or mkl lib
AC_ARG_ENABLE([usecblasnotmkl],
			  [AS_HELP_STRING([--enable-usecblasnotmkl],[enable use cblas not mkl])],
			  [],
			  [enable_usecblasnotmkl=yes])

AM_CONDITIONAL([USECBLAS], [test "x$enable_usecblasnotmkl" != xno])

# check kaldi static or dynamic
AC_ARG_ENABLE([usekaldistatic],
			  [AS_HELP_STRING([--enable-usekaldistatic],[use kaldi static libs])],
			  [],
			  [enable_usekaldistatic=yes])

AM_CONDITIONAL([USEKALDISTATIC], [test "x$enable_usekaldistatic" != xno])

AC_CHECK_LIB([dl], dlopen, [DL_LIBS=-ldl])
AC_SUBST([DL_LIBS])

AC_OUTPUT

#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
m4_define([ASR_VERSION], [1.0.0])
AC_INIT([asr-decoder], m4_defn([ASR_VERSION]), [1057277483@qq.com], [asr-decoder]m4_defn([ASR_VERSION]))
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
#AC_CONFIG_HEADERS([config.h])
AM_PROG_AR

ROOTDIR="$ASR_PROJECT_INSTALL_ROOT"
CXXFLAGS="-Wall -Wno-sign-compare -Wno-unused-local-typedefs -Wno-deprecated-declarations -Winit-self -msse -msse2 -pthread"
CXXFLAGS="$CXXFLAGS -std=c++11 -O2 -I$ROOTDIR"

# Checks for programs.
AC_PROG_CXX
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

#CXX=g++
#CC=gcc
# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

#AC_CHECK_LIB()
#AC_SEARCH_LIBS()

# checks for system services
AC_CONFIG_MACRO_DIR([m4])
##################################
AC_ARG_ENABLE([debug],
			  [AS_HELP_STRING([--enable-debug], [Turn on debugging@<:@default=false@:>@])],
			  [case "${enableval}" in
			     yes) debug=true ;;
			     no)  debug=false;;
			     *)   AC_MSG_ERROR("bad value ${enableval} for --enable-debug");;
			     esac],
			  [debug=false])

AM_CONDITIONAL(DEBUG, test "x$debug" != xfalse)
if test "x$debug" != xfalse ;
then
	AC_MSG_CHECKING("Add debug choise.")
	CXXFLAGS="$CXXFLAGS -DDEBUG"
fi

# --enable-namespace set namespace
AC_ARG_ENABLE([namespace],
			  [AS_HELP_STRING([--enable-namespace],[enable add namespcae. @<:@default=yes@:>@])],
			  [case "${enableval}" in
			     yes) enable_namespace=yes;;
				 no) enable_namespace=no;;
				 *)   AC_MSG_ERROR("bad value ${enableval} for --enable-namespace");;
			   esac],
			  [enable_namespace=yes])

# it must be define KALDI and NAMESPACE
if test "x$enable_namespace" != xno ;
then
	CXXFLAGS="$CXXFLAGS -DNAMESPACE -DKALDI"
fi

###################


# set KALDI_ROOT and no check ;
AC_ARG_WITH([kaldi],
			[AS_HELP_STRING([--with-kaldi], [depend kaldi and you must be install kaldi. @<:@default=../kaldi-master@:>@])],
			[AC_SUBST([KALDI_ROOT], ["${withval}"])],
			[AC_SUBST([KALDI_ROOT], ["../kaldi-master"]]))

# check kaldi static or dynamic
AC_ARG_ENABLE([usekaldistatic],
			  [AS_HELP_STRING([--enable-usekaldistatic],[use kaldi static libs. @<:@default=yes@:>@])],
			  [case "${enableval}" in
			     yes) usekaldistatic=yes;;
				 no) usekaldistatic=no;;
				 *)   AC_MSG_ERROR("bad value ${enableval} for --enable-usekaldistatic");;
			   esac],
			  [usekaldistatic=yes])

AM_CONDITIONAL([USEKALDISTATIC], [test "x$usekaldistatic" != xno])
###################


# set cuda and no check;
AC_ARG_ENABLE([usecuda],
			  [AS_HELP_STRING([--enable-usecuda],[enable use cuda @<:@default=yes@:>@])],
			  [case "${enableval}" in
			     yes) usecuda=yes;;
				 no)  usecuda=no;;
				 *)   AC_MSG_ERROR("bad value ${enableval} for --enable-usecuda");;
			   esac],
			  [usecuda=no])

AM_CONDITIONAL([USECUDA], [test "x$usecuda" != xno])
CUDA_ROOT=

if test "x$usecuda" != xno ;
then
	AC_ARG_WITH([cuda],
				[AS_HELP_STRING([--with-cuda], [depend cuda. @<:@default=/usr/local/cuda/@:>@])],
				[AC_SUBST([CUDA_ROOT], ["${withval}"])],
				[AC_SUBST([CUDA_ROOT], ["/usr/local/cuda/"])])
fi
####################
#check cuda_decoder
AC_ARG_ENABLE([usecudadecoder],
			  [AS_HELP_STRING([--enable-usecudadecoder], [enable use cuda decoder @<:@default=no@:>@])],
			  [case "${enableval}" in
			     yes) usecudadecoder=yes;;
				 no) usecudadecoder=no;;
				 *)   AC_MSG_ERROR("bad value ${enableval} for --enable-usecudadecoder");;
			   esac],
			  [usecudadecoder=no])

AM_CONDITIONAL([USA_CUDA_DECODER], [test "x$usecudadecoder" != xno])


####################
# check cblas lib or mkl lib
ATLAS_ROOT=
AC_ARG_WITH([atlas],
			[AS_HELP_STRING([--with-atlas],[math library use atlas. @<:@default=""@:>@])],
			[AC_SUBST([ATLAS_ROOT], ["${withval}"])],
			[AC_SUBST([ATLAS_ROOT], [""])])

MKL_ROOT=
AC_ARG_WITH([mkl],
			[AS_HELP_STRING([--with-mkl],[math library use mkl. @<:@default=/opt/intel/mkl@:>@])],
			[AC_SUBST([MKL_ROOT], ["${withval}"])],
			[AC_SUBST([MKL_ROOT], ["/opt/intel/mkl"])])

AM_CONDITIONAL(USEATLAS, test x$ATLAS_ROOT != x"")

###################
# set ffmpeg

FFMPEG_ROOT=
AC_ARG_WITH([ffmpeg],
			[AS_HELP_STRING([--with-ffmpeg],[audio library use ffmpeg. @<:@default=""@:>@])],
			[AC_SUBST([FFMPEG_ROOT], ["${withval}"])],
			[AC_SUBST([FFMPEG_ROOT], [""])])

AM_CONDITIONAL(HAVE_FFMPEG, test x$FFMPEG_ROOT != x"")

####################

AC_CHECK_LIB([dl], dlopen, [DL_LIBS=-ldl])
AC_SUBST([DL_LIBS])

AC_CONFIG_FILES([Makefile
				 fst_format_convert_tool/Makefile
				 util/Makefile
				 align/Makefile
				 newfst/Makefile
				 newlm/Makefile
				 pitch/Makefile
				 hmm/Makefile
				 my-decoder/Makefile
				 service/Makefile
				 service2/Makefile
				 service2bin/Makefile
				 nnet/Makefile
				 kaldi-nnet3/Makefile
				 kaldi-nnet3bin/Makefile
				 v1-asr/Makefile
				 v2-asr/Makefile
				 v1-asrbin/Makefile
				 v2-asrbin/Makefile
				 vad/Makefile
				 online-vad/Makefile
				 client/Makefile
				 client/py-client/Makefile
				 clientbin/Makefile
				 kaldi-bin/Makefile
				 kaldi-bin/bin/Makefile
				 post-processing-service/Makefile
				 biglm/Makefile
				 gpu-asr/Makefile])

AC_OUTPUT

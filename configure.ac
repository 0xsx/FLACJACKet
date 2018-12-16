AC_INIT([flacjacket], [0.1.0])

AC_PREREQ([2.59c])
AC_CANONICAL_SYSTEM
AC_CONFIG_SRCDIR([src/flacjacket.c])
AC_CONFIG_HEADERS([src/flacjacket-config.h])

AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_MAINTAINER_MODE


AC_PROG_CC_C99
AC_PROG_GCC_TRADITIONAL
AM_PROG_CC_C_O



AC_CHECK_LIB([m],[ceil],[],[AC_MSG_ERROR([libm not found])])
AC_CHECK_LIB([uuid],[uuid_generate],[],[AC_MSG_ERROR([libuuid not found])])
AC_CHECK_LIB([pthread],[pthread_create],[],[AC_MSG_ERROR([libpthread not found])])
AC_CHECK_LIB([jack],[jack_client_open],[],[AC_MSG_ERROR([libjack not found])])
AC_CHECK_LIB([FLAC],[FLAC__stream_encoder_new],[],[AC_MSG_ERROR([libFLAC not found])])


AC_ARG_ENABLE([debug],
    AS_HELP_STRING([--enable-debug], [Enable debug build]))

AS_IF([test "x$enable_debug" = "xyes"], [
  AC_DEFINE(DEBUG_BUILD, 1, [Define to 1 if debugging enabled.])
])


AS_IF([test x$have_win32 = xtrue], [
  AC_DEFINE(HAVE_WINDOWS, 1, [Define to 1 if building on Windows.])
])


AS_IF([test "x$OS" = "xWindows_NT"], [
  AC_DEFINE(HAVE_MINGW, 1, [Define to 1 if building with MINGW.])
])



AC_CONFIG_FILES([
	Makefile
	])
AC_OUTPUT

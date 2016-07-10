AC_INIT([sync_corpus], 0.0)
AM_INIT_AUTOMAKE([foreign])
AC_PROG_CXX

m4_define([required_lttoolbox_version], [3.3.3])
m4_define([required_apertium_version], [3.4.0])
PKG_CHECK_MODULES(APERTIUM, [
  lttoolbox >= required_lttoolbox_version
  apertium >= required_apertium_version],
  CPPFLAGS="$CPPFLAGS $APERTIUM_CFLAGS"; LIBS="$LIBS $APERTIUM_LIBS")

AC_CONFIG_FILES(Makefile)
AC_OUTPUT

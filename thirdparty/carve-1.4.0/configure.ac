AC_INIT(README)
AC_CONFIG_AUX_DIR(build/autoconf)
AC_SUBST(ac_aux_dir)

AC_CANONICAL_SYSTEM

AC_CONFIG_SRCDIR([src])
AM_CONFIG_HEADER(include/carve_config.h include/carve/config.h)
AM_INIT_AUTOMAKE(carve, 1.4.0, tobias.sargeant@gmail.com)
AC_DEFINE_UNQUOTED(CARVE_VERSION, ["${VERSION}"], [Carve version number])

AC_PROG_CC
AC_PROG_CPP
AM_PROG_LIBTOOL
AC_PROG_CXX

AM_MAINTAINER_MODE

AC_HEADER_STDC
AC_CHECK_HEADERS([stdint.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_HEADER_STDBOOL
AC_C_BIGENDIAN
AC_TYPE_SIZE_T

AC_LANG_CPLUSPLUS

# Checks for library functions.
# AC_CHECK_FUNCS([floor getcwd getservbyname getaddrinfo gethostbyname gettimeofday memset select setlocale socket sqrt strerror strtoul inet_ntop inet_pton])
AC_CHECK_FUNCS([isatty])

AC_ARG_WITH(tr1-collections,
              AC_HELP_STRING([--with-tr1-collections],              [use tr1 collections, where available]),
              with_TR1="$withval",
              with_TR1="yes")
AC_ARG_WITH(boost-collections,
              AC_HELP_STRING([--with-boost-collections],            [use boost collections]),
              with_BOOST="$withval",
              with_BOOST="no")
AC_ARG_ENABLE(debug,
              AC_HELP_STRING([--enable-debug],                      [compile in debug hooks [default=no]]))
AC_ARG_ENABLE(intersect-glu-triangulator,
              AC_HELP_STRING([--enable-intersect-glu-triangulator], [compile GLU-based triangulation code into intersect [default=yes]]),
              enable_GLU_tri="$enableval",
              enable_GLU_tri="yes")
AC_ARG_WITH(gui,
              AC_HELP_STRING([--with-gui],                          [compile gui code]),
              with_GUI="$withval",
              with_GUI="yes")


AC_MSG_CHECKING([for std collections])
AC_TRY_COMPILE([
#include <unordered_set>
#include <unordered_map>
], [
const std::unordered_map<int,int> a;
const std::unordered_set<int> b;
a.find(1);
b.find(1);
], [
  AC_MSG_RESULT([yes])
  AC_DEFINE([HAVE_STD_UNORDERED_COLLECTIONS], [], [Define if std::unordered_map and std::unordered_set are supported by your compiler.])
  with_TR1=no
], [
  AC_MSG_RESULT([no])
])


AC_MSG_CHECKING([for gnu libstdc++ collections])
AC_TRY_COMPILE([
#include <ext/hash_set>
#include <ext/hash_map>
], [
const __gnu_cxx::hash_map<int,int> a;
const __gnu_cxx::hash_set<int> b;
a.find(1);
b.find(1);
], [
  AC_MSG_RESULT([yes])
  AC_DEFINE([HAVE_LIBSTDCPP_UNORDERED_COLLECTIONS], [], [Define if using gnu libstdc++.])
  with_TR1=no
], [
  AC_MSG_RESULT([no])
])


if test "x$with_TR1" = "xyes"; then
  AC_MSG_CHECKING([for TR1 collections])
  AC_TRY_COMPILE([
#include <tr1/unordered_set>
#include <tr1/unordered_map>
],[
std::tr1::unordered_map<int,int> a;
std::tr1::unordered_set<int> b;
], [
  AC_TRY_COMPILE([
#include <tr1/unordered_set>
#include <tr1/unordered_map>
],[
const std::tr1::unordered_map<int,int> a;
const std::tr1::unordered_set<int> b;
a.find(1);
b.find(1);
], [
  AC_MSG_RESULT([yes])
  with_TR1=yes
], [
  AC_MSG_RESULT([yes, but unusable (const issues)])
  with_TR1=no
])], [
  AC_MSG_RESULT([no])
  with_TR1=no
])
fi


if test "x$with_TR1" = "xyes"; then
  AC_DEFINE([HAVE_TR1_UNORDERED_COLLECTIONS], [], [Define if TR1 collections are supportted by your compiler.])
fi


if test "x$with_BOOST" != "xno"; then
  AC_DEFINE([HAVE_BOOST_UNORDERED_COLLECTIONS], [], [Define if using boost collections.])
  CARVE_save_CPPFLAGS=$CPPFLAGS
  CPPFLAGS="${CPPFLAGS} -I$ac_confdir/include"
  AC_MSG_CHECKING([for boost collections])
  AC_TRY_COMPILE([
#include <carve/collection/external/boost/unordered_set.hpp>
#include <carve/collection/external/boost/unordered_map.hpp>
],[
const boost::unordered_map<int,int> a;
const boost::unordered_set<int> b;
a.find(1);
b.find(1);
], [
  AC_MSG_RESULT([yes])
  with_BOOST=yes
], [
  AC_MSG_RESULT([no])
  with_BOOST=no
])
  CPPFLAGS=$CARVE_save_CPPFLAGS
fi


if test x$enable_debug = xyes; then
  CPPFLAGS="${CPPFLAGS} -DDEBUG"
fi


CARVE_CHECK_OPENGL
if test x"$have_GL" = xno; then AC_MSG_NOTICE([missing GL]); with_GUI=no; fi
if test x"$have_GLU" = xno; then AC_MSG_NOTICE([missing GLU]); with_GUI=no; fi
if test x"$have_glut" = xno; then AC_MSG_NOTICE([missing glut]); with_GUI=no; fi

if test x"$have_GLU" = xyes; then
  AC_MSG_CHECKING([glu callback prototype])
  AC_TRY_COMPILE([
#if defined(__APPLE__)
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif
],[
  typedef GLvoid (*GLUTessCallback)(...);
  gluTessCallback((GLUtesselator *)NULL, GLU_TESS_END_DATA, (GLUTessCallback)NULL);
], [
  AC_MSG_RESULT([varargs])
  AC_DEFINE([GLU_TESS_CALLBACK_VARARGS], [], [Define if gluTessCallback callback prototype needs (...).])
], [
  AC_MSG_RESULT([no varargs])
])
fi

AM_CONDITIONAL(enable_GLU_tri,  test "x$enable_GLU_tri" = "xyes")
AM_CONDITIONAL(with_GUI, test "x$with_GUI" = "xyes")
AM_CONDITIONAL(with_BOOST, test "x$with_BOOST" != "xno")

AC_SUBST(GL_CFLAGS)
AC_SUBST(GL_LIBS)
AC_SUBST(GLUT_CFLAGS)
AC_SUBST(GLUT_LIBS)

AC_OUTPUT([
	Makefile lib/Makefile common/Makefile src/Makefile
        tests/Makefile examples/Makefile
        external/Makefile
	external/GLEW/Makefile external/GLOOP/Makefile
	external/GLUI/Makefile include/Makefile include/carve/Makefile
	include/carve/collection/Makefile
	include/carve/collection/unordered/Makefile
])

AC_DEFUN([CARVE_CHECK_OPENGL],[
  have_GL=no
  have_GLU=no
  have_glut=no

  case "$host" in
    *darwin*)
    if test x"${with_x}" != xyes; then
      AC_MSG_NOTICE([Using OSX frameworks for GL])
      GL_CFLAGS=""
      GL_LIBS="-framework OpenGL"

      GLUT_CFLAGS=""
      GLUT_LIBS="-framework GLUT"

      have_GL=yes
      have_GLU=yes
      have_glut=yes
    else
      AC_MSG_NOTICE([Not using OSX frameworks for GL])
    fi
    ;;
  esac

  if test x"$have_GL" = xno; then
    AC_PATH_XTRA

    AC_LANG_SAVE
    AC_LANG_C

    GL_CFLAGS=""
    GL_LIBS=""

    GLUT_CFLAGS=""
    GLUT_LIBS=""

    GL_X_LIBS=""

    if test x"$no_x" != xyes; then
      GL_CFLAGS="$X_CFLAGS"
      GL_X_LIBS="$X_PRE_LIBS $X_LIBS -lX11 -lXext -lXmu -lXt -lXi $X_EXTRA_LIBS"
    fi

    GL_save_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$GL_CFLAGS"

    GL_save_LIBS="$LIBS"
    LIBS="$GL_X_LIBS"

    AC_CHECK_LIB([GL],   [glAccum],       have_GL=yes,   have_GL=no)
    AC_CHECK_LIB([GLU],  [gluBeginCurve], have_GLU=yes,  have_GLU=no)
    AC_CHECK_LIB([glut], [glutInit],      have_glut=yes, have_glut=no)

    if test x"$have_GL" = xno; then
      GL_LIBS="-lGL"
    fi

    if test x"$have_GLU" = xyes; then
      GL_LIBS="$GL_LIBS -lGLU"
    fi

    if test x"$have_glut" = xyes; then
      GLUT_LIBS="-lglut"
    fi

    GL_X_LIBS=""

    LIBS="$GL_save_LIBS"
    CPPFLAGS="$GL_save_CPPFLAGS"

    AC_LANG_RESTORE
  fi
])

AC_DEFUN([CARVE_CHECK_GLUI],[
AC_ARG_WITH(glui-prefix,  [  --with-glui-prefix=PFX      Prefix where GLUI is installed (optional)],
            glui_prefix="$withval", glui_prefix="")
AC_ARG_WITH(glui-includes,[  --with-glui-includes=PATH   Path to GLUI includes (optional)],
            glui_includes="$withval", glui_includes="")
AC_ARG_WITH(glui-libs,    [  --with-glui-libs=PATH       Path to GLUI libs (optional)],
            glui_libs="$withval", glui_libs="")

  if test x"$glui_prefix" != x""; then
    glui_libs="$glui_prefix/lib"
    glui_includes="$glui_prefix/include"
  fi

  GLUI_CFLAGS=""
  if test x"$glui_includes" != x""; then
    GLUI_CFLAGS="-I$glui_includes"
  fi

  GLUI_LDFLAGS=""
  if test x"$glui_libs" != x""; then
    GLUI_LDFLAGS="-L$glui_libs"
  fi

  GLUI_LIBS="-lglui"


  AC_LANG_SAVE
  AC_LANG_CPLUSPLUS

  GLUI_save_CPPFLAGS="$CPPFLAGS"
  CPPFLAGS="$CPPFLAGS $GLUI_CFLAGS"

  GLUI_save_LIBS="$LIBS"
  LIBS="$LIBS $GLUI_LIBS"

  GLUI_save_LDFLAGS="$LDFLAGS"
  LDFLAGS="$LDFLAGS $GLUI_LDFLAGS"

  have_GLUI=no
  AC_CHECK_LIB(glui, main, [AC_CHECK_HEADER(GL/glui.h, [have_GLUI=yes])])

  LDFLAGS="$GLUI_save_LDFLAGS"
  LIBS="$GLUI_save_LIBS"
  CPPFLAGS="$GLUI_save_CPPFLAGS"

  AC_LANG_RESTORE
])

AC_DEFUN([CARVE_CHECK_GLEW],[
  AC_ARG_WITH(glew-prefix,  [  --with-glew-prefix=PFX      Prefix where GLEW is installed (optional)],
              glew_prefix="$withval", glew_prefix="")
  AC_ARG_WITH(glew-includes,[  --with-glew-includes=PATH   Path to GLEW includes (optional)],
              glew_includes="$withval", glew_includes="")
  AC_ARG_WITH(glew-libs,    [  --with-glew-libs=PATH       Path to GLEW libs (optional)],
              glew_libs="$withval", glew_libs="")

  if test x"$glew_prefix" != x""; then
    glew_libs="$glew_prefix/lib"
    glew_includes="$glew_prefix/include"
  fi

  GLEW_CFLAGS=""
  if test x"$glew_includes" != x""; then
    GLEW_CFLAGS="-I$glew_includes"
  fi

  GLEW_LDFLAGS=""
  if test x"$glew_libs" != x""; then
    GLEW_LDFLAGS="-L$glew_libs"
  fi

  GLEW_LIBS="-lglew"

  AC_LANG_SAVE
  AC_LANG_C

  GLEW_save_CPPFLAGS="$CPPFLAGS"
  CPPFLAGS="$CPPFLAGS $GLEW_CFLAGS"

  GLEW_save_LIBS="$LIBS"
  LIBS="$LIBS $GLEW_LIBS"

  GLEW_save_LDFLAGS="$LDFLAGS"
  LDFLAGS="$LDFLAGS $GLEW_LDFLAGS"

  have_GLEW=no
  AC_CHECK_LIB(glew, glewInit, [AC_CHECK_HEADER(GL/glew.h, [have_GLEW=yes])])

  LDFLAGS="$GLEW_save_LDFLAGS"
  LIBS="$GLEW_save_LIBS"
  CPPFLAGS="$GLEW_save_CPPFLAGS"

  AC_LANG_RESTORE
])

AC_DEFUN([CARVE_CHECK_GLOOP],[
  AC_ARG_WITH(gloop-prefix,  [  --with-gloop-prefix=PFX     Prefix where GLOOP is installed (optional)],
              gloop_prefix="$withval", gloop_prefix="")
  AC_ARG_WITH(gloop-includes,[  --with-gloop-includes=PATH  Path to GLOOP includes (optional)],
              gloop_includes="$withval", gloop_includes="")
  AC_ARG_WITH(gloop-libs,    [  --with-gloop-libs=PATH      Path to GLOOP libs (optional)],
              gloop_libs="$withval", gloop_libs="")

  if test x"$gloop_prefix" != x""; then
    gloop_libs="$gloop_prefix/lib"
    gloop_includes="$gloop_prefix/include"
  fi

  GLOOP_CFLAGS=""
  if test x"$gloop_includes" != x""; then
    GLOOP_CFLAGS="-I$gloop_includes"
  fi

  GLOOP_LDFLAGS=""
  if test x"$gloop_libs" != x""; then
    GLOOP_LDFLAGS="-L$gloop_libs"
  fi

  GLOOP_LIBS="-lgloop"
  GLOOP_MODEL_LIBS="-lgloop-model"

  AC_LANG_SAVE
  AC_LANG_CPLUSPLUS

  GLOOP_save_CPPFLAGS="$CPPFLAGS"
  CPPFLAGS="$CPPFLAGS $GLOOP_CFLAGS"

  GLOOP_save_LIBS="$LIBS"
  LIBS="$LIBS $GLOOP_LIBS $GLOOP_MODEL_LIBS"

  GLOOP_save_LDFLAGS="$LDFLAGS"
  LDFLAGS="$LDFLAGS $GLOOP_LDFLAGS"

  have_gloop=no
  AC_CHECK_HEADERS(gloop/gloop.hpp, [AC_CHECK_LIB(gloop, main, have_gloop=yes)])

  LDFLAGS="$GLOOP_save_LDFLAGS"
  LIBS="$GLOOP_save_LIBS"
  CPPFLAGS="$GLOOP_save_CPPFLAGS"

  AC_LANG_RESTORE
])

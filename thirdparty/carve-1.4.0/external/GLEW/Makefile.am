if with_GUI
  noinst_LTLIBRARIES = libglew.la
  noinst_HEADERS = include/GL/glew.h
  libglew_la_CPPFLAGS=@CPPFLAGS@ -Iinclude 
  libglew_la_CFLAGS=@CFLAGS@ @GL_CFLAGS@
  libglew_la_LIBADD=
  libglew_la_SOURCES=src/glew.c
endif

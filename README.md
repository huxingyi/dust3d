WIP...

Build
--------
Mac
```
$ qmake -spec macx-xcode
```

Windows
```
From Start Menu, Open Visual Studio 2017 Tools Command Prompt:
C:\Program Files\Microsoft Visual Studio\2017\Community>cd C:\Users\IEUser\Desktop\dust3d
C:\Users\IEUser\Desktop\dust3d>qmake DEFINES+=BOOST_INCLUDEDIR=C:\dev\boost_1_55_0\boost_1_55_0 DEFINES+=CGAL_DIR=C:\dev\CGAL-4.11.1
C:\Users\IEUser\Desktop\dust3d>nmake -f Makefile.Release
```
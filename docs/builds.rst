Building Dust3D
-------------------

Windows
=======

.. code-block:: none

    From Start Menu, Open Visual Studio 2017 Tools Command Prompt:
    C:\Program Files\Microsoft Visual Studio\2017\Community>cd C:\Users\IEUser\Desktop\dust3d
    C:\Users\IEUser\Desktop\dust3d>qmake DEFINES+=BOOST_INCLUDEDIR=C:\dev\boost_1_55_0\boost_1_55_0 DEFINES+=CGAL_DIR=C:\dev\CGAL-4.11.1
    C:\Users\IEUser\Desktop\dust3d>nmake -f Makefile.Release

Mac
=======

.. code-block:: sh

    $ cd /Users/jeremy/Desktop
    $ git clone https://github.com/huxingyi/meshlite.git
    $ cd meshlite
    $ cargo build --release
    $ cp include/meshlite.h /Users/jeremy/Repositories/dust3d/thirdparty/meshlite/meshlite.h
    $ cp target/release/libmeshlite.dylib /Users/jeremy/Repositories/dust3d/thirdparty/meshlite/libmeshlite.dylib
    
    $ cd /Users/jeremy/Repositories/dust3d
    $ qmake -spec macx-xcode
    Open dust3d.xcodeproj in Xcode and build
"""""""""""""""""
Building Dust3D
"""""""""""""""""

...........
Overview
...........

The UI of Dust3D is built on Qt5, the third party dependencies which may need to be compiled separately are the CGAL and Instant Meshes, however, CGAL will introduce some new dependencies, such as boost and gmp library.

===================
Prerequisites
===================

* Qt

    https://www.qt.io

* CGAL

    https://www.cgal.org

* Instant Meshes

    https://github.com/wjakob/instant-meshes

* Boost

    https://www.boost.org

* CMake

    https://cmake.org/

===================
Building
===================

Here are the snapshots of the build commands on different platforms, you may use customized defines on your system. If you encounter build issues, please follow the ci files step by step,

    Windows:
    https://github.com/huxingyi/dust3d/blob/master/appveyor.yml

    Mac and Linux:
    https://github.com/huxingyi/dust3d/blob/master/.travis.yml

----------------------
Windows
----------------------
**The following steps are for windows 64 bit, please refer to the appveyor.yml file for windows 32 bit**

Install `Qt 5.13.2`_

.. _Qt 5.13.2: http://download.qt.io/official_releases/qt/5.13/5.13.2/qt-opensource-windows-x86-5.13.2.exe

Install ``Visual Studio 2017`` Community edition

Install ``CMake``, during installation, choose ``Add CMake to the system PATH for the current user``

Download `boost 1.66`_ and extract to ``C:\Libraries`` make sure you got your boost path on ``C:\Libraries\boost_1_66_0``

.. _boost 1.66: https://www.boost.org/users/history/version_1_66_0.html

Download `Dust3D Source Repository`_ and extract to your desktop as ``C:\Users\IEUser\Desktop\dust3d``, please note the ``IEUser`` should be replaced with your windows user name.

.. _Dust3D Source Repository: https://github.com/huxingyi/dust3d/archive/master.zip

Build ``CGAL`` as the following steps:

.. code-block:: none

    From Start Menu, Open x64 Native Tools Command Prompt for VS 2017:
    C:\Program Files (x86)\Microsoft Visual Studio\2017\Community>cd C:\Users\IEUser\Desktop\dust3d
    C:\Users\IEUser\Desktop\dust3d>cd thirdparty\cgal\CGAL-4.13
    C:\Users\IEUser\Desktop\dust3d\thirdparty\cgal\CGAL-4.13>mkdir build
    C:\Users\IEUser\Desktop\dust3d\thirdparty\cgal\CGAL-4.13>cd build
    C:\Users\IEUser\Desktop\dust3d\thirdparty\cgal\CGAL-4.13\build>cmake -G "Visual Studio 15 2017" -A x64 ../ -DBOOST_INCLUDEDIR=C:\Libraries\boost_1_66_0
    C:\Users\IEUser\Desktop\dust3d\thirdparty\cgal\CGAL-4.13\build>msbuild /p:Configuration=Release ALL_BUILD.vcxproj

Build ``Instant Meshes`` as the following steps:

.. code-block:: none

    From Start Menu, Open x64 Native Tools Command Prompt for VS 2017:
    C:\Program Files (x86)\Microsoft Visual Studio\2017\Community>cd C:\Users\IEUser\Desktop\dust3d
    C:\Users\IEUser\Desktop\dust3d>cd thirdparty\instant-meshes
    C:\Users\IEUser\Desktop\dust3d\thirdparty\instant-meshes>mkdir build
    C:\Users\IEUser\Desktop\dust3d\thirdparty\instant-meshes>cd build
    C:\Users\IEUser\Desktop\dust3d\thirdparty\instant-meshes\build>cmake -G "Visual Studio 15 2017" -A x64 ../
    C:\Users\IEUser\Desktop\dust3d\thirdparty\instant-meshes\build>msbuild /p:Configuration=RelWithDebInfo ALL_BUILD.vcxproj

Build ``Dust3D`` as the following steps:

.. code-block:: none

    From Start Menu, Open x64 Native Tools Command Prompt for VS 2017:
    C:\Program Files (x86)\Microsoft Visual Studio\2017\Community>cd C:\Users\IEUser\Desktop\dust3d
    C:\Users\IEUser\Desktop\dust3d>set PATH=%PATH%;C:\Qt\Qt5.13.2\5.13.2\msvc2017_64\bin
    C:\Users\IEUser\Desktop\dust3d>qmake "BOOST_INCLUDEDIR=C:\Libraries\boost_1_66_0" "CGAL_DIR=C:\Users\IEUser\Desktop\dust3d\thirdparty\cgal\CGAL-4.13"
    C:\Users\IEUser\Desktop\dust3d>nmake -f Makefile.Release
    After dust3d.exe is been built in C:\Users\IEUser\Desktop\dust3d\release, copy the following files to release folder:
      C:\Users\IEUser\Desktop\dust3d\thirdparty\instant-meshes\build\RelWithDebInfo\instant-meshes.dll
      C:\Users\IEUser\Desktop\dust3d\thirdparty\instant-meshes\build\ext_build\tbb\RelWithDebInfo\tbb.dll
      C:\Users\IEUser\Desktop\dust3d\thirdparty\cgal\CGAL-4.13\build\bin\Release\CGAL-vc140-mt-4.13.dll
      C:\Users\IEUser\Desktop\dust3d\thirdparty\cgal\CGAL-4.13\auxiliary\gmp\lib\libgmp-10.dll
      C:\Users\IEUser\Desktop\dust3d\thirdparty\cgal\CGAL-4.13\auxiliary\gmp\lib\libmpfr-4.dll
      C:\Qt\Qt5.13.2\5.13.2\msvc2017_64\bin\Qt5Widgets.dll
      C:\Qt\Qt5.13.2\5.13.2\msvc2017_64\bin\Qt5Gui.dll
      C:\Qt\Qt5.13.2\5.13.2\msvc2017_64\bin\Qt5Core.dll
      C:\Qt\Qt5.13.2\5.13.2\msvc2017_64\bin\Qt5Network.dll
      C:\Qt\Qt5.13.2\5.13.2\msvc2017_64\bin\opengl32sw.dll
    Now run dust3d.exe

----------------------
Mac
----------------------

**Outdated, help needed**

.. code-block:: sh

    $ cd /Users/jeremy/Repositories/dust3d
    $ qmake -spec macx-xcode
    Open dust3d.xcodeproj in Xcode and build

----------------------
Ubuntu
----------------------

**Outdated, help needed**

.. code-block:: sh

    ;Install Qt5
    $ sudo apt-get install --reinstall qtchooser
    $ sudo apt-get install qtbase5-dev

    ;Prepare compile environment for CGAL-4.13
    $ sudo apt-get install libcgal-dev	; This is not the latest version, will encounter compiler error when build the Dust3D with this version, but helps resolve internal dependencies of CGAL for you
    $ sudo apt install cmake

    ;Install CGAL-4.13
    $ wget https://github.com/CGAL/cgal/releases/download/releases/CGAL-4.13/CGAL-4.13.zip
    $ unzip CGAL-4.13.zip
    $ cd CGAL-4.13
    $ mkdir build
    $ cd build
    $ cmake ../
    $ make
    $ sudo make install

    ;Clone the Main project
    $ cd ~/Documents
    $ git clone https://github.com/huxingyi/dust3d.git

    ;Compile Dust3D
    $ cd ~/Documents/dust3d
    $ qmake -qt=5 -makefile
    $ make
    $ ./dust3d

----------------------
Fedora
----------------------

**Outdated, help needed**

.. code-block:: sh

    $ sudo dnf install qt5-qtbase-devel CGAL-devel
    $ git clone https://github.com/huxingyi/dust3d.git
    $ cd dust3d
    $ qmake-qt5 -makefile
    $ make
    $ ./dust3d

Building Dust3D
-------------------

Overview
==========
The core mesh algorithms of Dust3D written in Rust language, located in meshlite repository,

    https://github.com/huxingyi/meshlite

The UI of Dust3D built in Qt5, the only thirdparty dependency which should been compiled separately is the CGAL library, however, CGAL will introduce some new dependencies, such as boost and gmp library.

Prerequisites
===============
* CGAL

    https://www.cgal.org/

* Rust

    https://www.rust-lang.org/en-US/install.html


Building
==========

Here is the snapshot of the command line of one build, you may use different defines on your system. If you encounter build issues, please follow the ci files step by step,

    https://github.com/huxingyi/dust3d/blob/master/appveyor.yml
    https://github.com/huxingyi/dust3d/blob/master/.travis.yml

* Windows

.. code-block:: none

    From Start Menu, Open Visual Studio 2017 Tools Command Prompt:
    C:\Program Files\Microsoft Visual Studio\2017\Community>cd C:\Users\IEUser\Desktop\dust3d
    C:\Users\IEUser\Desktop\dust3d>qmake DEFINES+=BOOST_INCLUDEDIR=C:\dev\boost_1_55_0\boost_1_55_0 DEFINES+=CGAL_DIR=C:\dev\CGAL-4.11.1
    C:\Users\IEUser\Desktop\dust3d>nmake -f Makefile.Release

* Mac

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

* Ubuntu

.. code-block:: sh

    ;Install Rust
    $ curl https://sh.rustup.rs -sSf | sh         ;Add ~/.cargo/bin to PATH after finishing install

    ;Install Qt5
    $ sudo apt-get install --reinstall qtchooser
    $ sudo apt-get install qtbase5-dev

    ;Prepare compile environment for CGAL-4.11.1
    $ sudo apt-get install libcgal-dev	; This is not the latest version, will encounter compiler error when build the Dust3D with this version, but helps resolve internal dependencies of CGAL for you
    $ sudo apt install cmake

    ;Install CGAL-4.11.1, other versions of CGAL haven't test with Dust3D
    $ wget https://github.com/CGAL/cgal/releases/download/releases/CGAL-4.11.1/CGAL-4.11.1.zip
    $ unzip CGAL-4.11.1.zip
    $ cd CGAL-4.11.1
    $ mkdir build
    $ cd build
    $ cmake ../
    $ make
    $ sudo make install

    ;Clone the Main project
    $ cd ~/Documents
    $ git clone https://github.com/huxingyi/dust3d.git

    ;Compile the internal dependency
    $ cd ~/Desktop
    $ git clone https://github.com/huxingyi/meshlite.git
    $ cd meshlite
    $ cargo build --release
    $ cp ~/Desktop/meshlite/include/meshlite.h ~/Documents/dust3d/thirdparty/meshlite/meshlite.h
    $ cp ~/Desktop/meshlite/target/release/libmeshlite.so ~/Documents/dust3d/thirdparty/meshlite/libmeshlite.so

    ;Compile Dust3D
    $ cd ~/Documents/dust3d
    $ qmake -qt=5 -makefile
    $ make
    $ export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/Documents/dust3d/thirdparty/meshlite
    $ ./dust3d

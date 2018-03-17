#!/bin/sh
aclocal -I m4
autoheader
libtoolize -f -c --automake
automake --foreign -c -a
autoconf

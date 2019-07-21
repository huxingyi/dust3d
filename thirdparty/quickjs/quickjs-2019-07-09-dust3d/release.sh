#!/bin/sh
# Release the QuickJS source code

set -e

version=`cat VERSION`

if [ "$1" = "-h" ] ; then
    echo "release.sh [all]"
    echo ""
    echo "all: build all the archives. Otherwise only build the quickjs source archive."
    exit 1
fi

    
if [ "$1" = "all" ] ; then

#################################################"
# unicode data

d="quickjs-${version}"
name="quickjs-unicode-data-${version}"
outdir="/tmp/${d}"

rm -rf $outdir
mkdir -p $outdir $outdir/unicode

cp unicode/* $outdir/unicode

( cd /tmp && tar Jcvf /tmp/${name}.tar.xz ${d} )

#################################################"
# all tests

d="quickjs-${version}"
name="quickjs-tests-${version}"
outdir="/tmp/${d}"

rm -rf $outdir
mkdir -p $outdir $outdir/test262o $outdir/test262 $outdir/tests 

cp -a test262o/test $outdir/test262o

cp -a test262/test test262/harness $outdir/test262

cp -a tests/bench-v8 $outdir/tests

( cd /tmp && tar Jcvf /tmp/${name}.tar.xz ${d} )

fi # all

#################################################"
# quickjs

make build_doc

d="quickjs-${version}"
outdir="/tmp/${d}"

rm -rf $outdir
mkdir -p $outdir $outdir/doc $outdir/tests $outdir/examples

cp Makefile VERSION TODO readme.txt release.sh \
   qjs.c qjsc.c qjscalc.js repl.js \
   quickjs.c quickjs.h quickjs-atom.h \
   quickjs-libc.c quickjs-libc.h quickjs-opcode.h \
   cutils.c cutils.h list.h \
   libregexp.c libregexp.h libregexp-opcode.h \
   libunicode.c libunicode.h libunicode-table.h \
   libbf.c libbf.h \
   jscompress.c unicode_gen.c unicode_gen_def.h \
   bjson.c \
   run-test262.c test262o.conf test262.conf test262bn.conf \
   test262o_errors.txt test262_errors.txt test262bn_errors.txt \
   $outdir

cp tests/*.js tests/*.patch $outdir/tests

cp examples/*.js examples/*.c $outdir/examples

cp doc/quickjs.texi doc/quickjs.pdf doc/quickjs.html \
   doc/jsbignum.texi doc/jsbignum.html doc/jsbignum.pdf \
   $outdir/doc 

( cd /tmp && tar Jcvf /tmp/${d}.tar.xz ${d} )

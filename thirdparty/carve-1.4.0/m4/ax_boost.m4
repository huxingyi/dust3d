##### http://autoconf-archive.cryp.to/ax_boost.html
#
# SYNOPSIS
#
#   AX_BOOST([MINIMUM-VERSION], [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
#
# DESCRIPTION
#
#   Test for the Boost C++ libraries of a particular version (or newer)
#
#   If no path to the installed boost library is given the macro
#   searchs under /usr, /usr/local, and /opt, and evaluates the
#   $BOOST_ROOT environment variable. Further documentation is
#   available at <http://randspringer.de/boost/index.html>.
#
#   This macro calls:
#
#     AC_SUBST(BOOST_CPPFLAGS) / AC_SUBST(BOOST_LDFLAGS)
#     AC_SUBST(BOOST_FILESYSTEM_LIB)
#     AC_SUBST(BOOST_PROGRAM_OPTIONS_LIB)
#     AC_SUBST(BOOST_THREAD_LIB)
#     AC_SUBST(BOOST_IOSTREAMS_LIB)
#     AC_SUBST(BOOST_SERIALIZATION_LIB)
#     AC_SUBST(BOOST_WSERIALIZATION_LIB)
#     AC_SUBST(BOOST_SIGNALS_LIB)
#     AC_SUBST(BOOST_DATE_TIME_LIB)
#     AC_SUBST(BOOST_REGEX_LIB)
#     AC_SUBST(BOOST_UNIT_TEST_FRAMEWORK_LIB)
#
#   And sets:
#
#     HAVE_BOOST
#     HAVE_BOOST_FILESYSTEM
#     HAVE_BOOST_PROGRAM_OPTIONS
#     HAVE_BOOST_THREAD
#     HAVE_BOOST_IOSTREAMS
#     HAVE_BOOST_SERIALIZATION
#     HAVE_BOOST_SIGNALS
#     HAVE_BOOST_DATE_TIME
#     HAVE_BOOST_REGEX
#     HAVE_BOOST_UNIT_TEST_FRAMEWORK
#
# LAST MODIFICATION
#
#   2006-12-28
#
# COPYLEFT
#
#   Copyright (c) 2006 Thomas Porschberg <thomas@randspringer.de>
#
#   Copying and distribution of this file, with or without
#   modification, are permitted in any medium without royalty provided
#   the copyright notice and this notice are preserved.

AC_DEFUN([AX_BOOST],
[
    AC_ARG_WITH([boost],
                AS_HELP_STRING([--with-boost=DIR],
                [use boost (default is NO) specify the root directory for boost library (optional)]),
                [
                if test "$withval" = "no"; then
		            want_boost="no"
                elif test "$withval" = "yes"; then
                    want_boost="yes"
                    ac_boost_path=""
                else
			        want_boost="yes"
            		ac_boost_path="$withval"
		        fi
            	],
                [want_boost="yes"])

    AC_CANONICAL_BUILD
	if test "x$want_boost" = "xyes"; then
        AC_REQUIRE([AC_PROG_CC])
		boost_lib_version_req=ifelse([$1], ,1.20.0,$1)
		boost_lib_version_req_shorten=`expr $boost_lib_version_req : '\([[0-9]]*\.[[0-9]]*\)'`
		boost_lib_version_req_major=`expr $boost_lib_version_req : '\([[0-9]]*\)'`
		boost_lib_version_req_minor=`expr $boost_lib_version_req : '[[0-9]]*\.\([[0-9]]*\)'`
		boost_lib_version_req_sub_minor=`expr $boost_lib_version_req : '[[0-9]]*\.[[0-9]]*\.\([[0-9]]*\)'`
		if test "x$boost_lib_version_req_sub_minor" = "x" ; then
			boost_lib_version_req_sub_minor="0"
    	fi
		WANT_BOOST_VERSION=`expr $boost_lib_version_req_major \* 100000 \+  $boost_lib_version_req_minor \* 100 \+ $boost_lib_version_req_sub_minor`
		AC_MSG_CHECKING(for boostlib >= $boost_lib_version_req)
		succeeded=no

		dnl first we check the system location for boost libraries
		dnl this location ist chosen if boost libraries are installed with the --layout=system option
		dnl or if you install boost with RPM
		if test "$ac_boost_path" != ""; then
			BOOST_LDFLAGS="-L$ac_boost_path/lib"
			BOOST_CPPFLAGS="-I$ac_boost_path/include"
		else
			for ac_boost_path_tmp in /usr /usr/local /opt ; do
				if test -d "$ac_boost_path_tmp/include/boost" && test -r "$ac_boost_path_tmp/include/boost"; then
					BOOST_LDFLAGS="-L$ac_boost_path_tmp/lib"
					BOOST_CPPFLAGS="-I$ac_boost_path_tmp/include"
					break;
				fi
			done
		fi

		CPPFLAGS_SAVED="$CPPFLAGS"
		CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
		export CPPFLAGS

		LDFLAGS_SAVED="$LDFLAGS"
		LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"
		export LDFLAGS

	AC_LANG_PUSH(C++)
     	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
@%:@include <boost/version.hpp>
]],
       [[
#if BOOST_VERSION >= $WANT_BOOST_VERSION
// Everything is okay
#else
#  error Boost version is too old
#endif

		]])],
    	[
         AC_MSG_RESULT(yes)
		 succeeded=yes
		 found_system=yes
         ifelse([$2], , :, [$2])
       ],
       [
       ])
       AC_LANG_POP([C++])
		dnl if we found no boost with system layout we search for boost libraries
		dnl built and installed without the --layout=system option or for a staged(not installed) version
		if test "x$succeeded" != "xyes"; then
			_version=0
			if test "$ac_boost_path" != ""; then
                BOOST_LDFLAGS="-L$ac_boost_path/lib"
				if test -d "$ac_boost_path" && test -r "$ac_boost_path"; then
					for i in `ls -d $ac_boost_path/include/boost-* 2>/dev/null`; do
						_version_tmp=`echo $i | sed "s#$ac_boost_path##" | sed 's/\/include\/boost-//' | sed 's/_/./'`
						V_CHECK=`expr $_version_tmp \> $_version`
						if test "$V_CHECK" = "1" ; then
							_version=$_version_tmp
						fi
						VERSION_UNDERSCORE=`echo $_version | sed 's/\./_/'`
						BOOST_CPPFLAGS="-I$ac_boost_path/include/boost-$VERSION_UNDERSCORE"
					done
				fi
			else
				for ac_boost_path in /usr /usr/local /opt ; do
					if test -d "$ac_boost_path" && test -r "$ac_boost_path"; then
						for i in `ls -d $ac_boost_path/include/boost-* 2>/dev/null`; do
							_version_tmp=`echo $i | sed "s#$ac_boost_path##" | sed 's/\/include\/boost-//' | sed 's/_/./'`
							V_CHECK=`expr $_version_tmp \> $_version`
							if test "$V_CHECK" = "1" ; then
								_version=$_version_tmp
								best_path=$ac_boost_path
							fi
						done
					fi
				done

				VERSION_UNDERSCORE=`echo $_version | sed 's/\./_/'`
				BOOST_CPPFLAGS="-I$best_path/include/boost-$VERSION_UNDERSCORE"
				BOOST_LDFLAGS="-L$best_path/lib"

	    		if test "x$BOOST_ROOT" != "x"; then
                    if test -d "$BOOST_ROOT" && test -r "$BOOST_ROOT" && test -d "$BOOST_ROOT/stage/lib" && test -r "$BOOST_ROOT/stage/lib"; then
						version_dir=`expr //$BOOST_ROOT : '.*/\(.*\)'`
						stage_version=`echo $version_dir | sed 's/boost_//' | sed 's/_/./g'`
						stage_version_shorten=`expr $stage_version : '\([[0-9]]*\.[[0-9]]*\)'`
						V_CHECK=`expr $stage_version_shorten \>\= $_version`
						if test "$V_CHECK" = "1" ; then
							AC_MSG_NOTICE(We will use a staged boost library from $BOOST_ROOT)
							BOOST_CPPFLAGS="-I$BOOST_ROOT"
							BOOST_LDFLAGS="-L$BOOST_ROOT/stage/lib"
						fi
					fi
	    		fi
			fi

			CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
			export CPPFLAGS
			LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"
			export LDFLAGS

            AC_LANG_PUSH(C++)
            AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
@%:@include <boost/version.hpp>
]],
       [[
#if BOOST_VERSION >= $WANT_BOOST_VERSION
// Everything is okay
#else
#  error Boost version is too old
#endif

		]])],
    	[
         AC_MSG_RESULT(yes ($_version))
		 succeeded=yes
         ifelse([$2], , :, [$2])
       ],
       [
         AC_MSG_RESULT(no ($_version))
         ifelse([$3], , :, [$3])
       ])
    	AC_LANG_POP([C++])
		fi

		if test "$succeeded" != "yes" ; then
			if test "$_version" = "0" ; then
				AC_MSG_ERROR([[We could not detect the boost libraries (version $boost_lib_version_req_shorten or higher). If you have a staged boost library (still not installed) please specify \$BOOST_ROOT in your environment and do not give a PATH to --with-boost option.  If you are sure you have boost installed, then check your version number looking in <boost/version.hpp>. See http://randspringer.de/boost for more documentation.]])
			else
				AC_MSG_ERROR('Your boost libraries seems to old (version $_version).  We need at least $boost_lib_version_shorten')
			fi
		else
			AC_SUBST(BOOST_CPPFLAGS)
			AC_SUBST(BOOST_LDFLAGS)
			AC_DEFINE(HAVE_BOOST,,[define if the Boost library is available])

			AC_CACHE_CHECK([whether the Boost::Filesystem library is available],
						   ax_cv_boost_filesystem,
						[AC_LANG_PUSH([C++])
			 AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[@%:@include <boost/filesystem/path.hpp>]],
                                   [[using namespace boost::filesystem;
                                   path my_path( "foo/bar/data.txt" );
                                   return 0;]]),
            				       ax_cv_boost_filesystem=yes, ax_cv_boost_filesystem=no)
                                   AC_LANG_POP([C++])
			])
			if test "$ax_cv_boost_filesystem" = "yes"; then
				AC_DEFINE(HAVE_BOOST_FILESYSTEM,,[define if the Boost::FILESYSTEM library is available])
				BN=boost_filesystem
				for ax_lib in $BN $BN-$CC $BN-$CC-mt $BN-$CC-mt-s $BN-$CC-s \
                              lib$BN lib$BN-$CC lib$BN-$CC-mt lib$BN-$CC-mt-s lib$BN-$CC-s \
                              $BN-mgw $BN-mgw $BN-mgw-mt $BN-mgw-mt-s $BN-mgw-s ; do
				    AC_CHECK_LIB($ax_lib, main,
                                 [BOOST_FILESYSTEM_LIB="-l$ax_lib" AC_SUBST(BOOST_FILESYSTEM_LIB) link_filesystem="yes" break],
                                 [link_filesystem="no"])
  				done
				if test "x$link_filesystem" = "xno"; then
					AC_MSG_NOTICE(Could not link against $ax_lib !)
				fi
			fi

			AC_CACHE_CHECK([whether the Boost::Program_Options library is available],
						   ax_cv_boost_program_options,
						   [AC_LANG_PUSH([C++])
			               AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[@%:@include <boost/program_options.hpp>]],
                                   [[boost::program_options::options_description generic("Generic options");
                                   return 0;]]),
                           ax_cv_boost_program_options=yes, ax_cv_boost_program_options=no)
                           AC_LANG_POP([C++])
			])
			if test "$ax_cv_boost_program_options" = yes; then
				AC_DEFINE(HAVE_BOOST_PROGRAM_OPTIONS,,[define if the Boost::PROGRAM_OPTIONS library is available])
				BN=boost_program_options
				for ax_lib in $BN $BN-$CC $BN-$CC-mt $BN-$CC-mt-s $BN-$CC-s \
                              lib$BN lib$BN-$CC lib$BN-$CC-mt lib$BN-$CC-mt-s lib$BN-$CC-s \
                              $BN-mgw $BN-mgw $BN-mgw-mt $BN-mgw-mt-s $BN-mgw-s ; do
				    AC_CHECK_LIB($ax_lib, main,
                                 [BOOST_PROGRAM_OPTIONS_LIB="-l$ax_lib" AC_SUBST(BOOST_PROGRAM_OPTIONS_LIB) link_program_options="yes" break],
                                 [link_program_options="no"])
  				done
				if test "x$link_program_options="no"" = "xno"; then
					AC_MSG_NOTICE(Could not link against $ax_lib !)
				fi
			fi

			AC_CACHE_CHECK(whether the Boost::Thread library is available,
						   ax_cv_boost_thread,
						[AC_LANG_PUSH([C++])
			 CXXFLAGS_SAVE=$CXXFLAGS

			 if test "x$build_os" = "xsolaris" ; then
  				 CXXFLAGS="-pthreads $CXXFLAGS"
			 elif test "x$build_os" = "xming32" ; then
				 CXXFLAGS="-mthreads $CXXFLAGS"
			 else
				CXXFLAGS="-pthread $CXXFLAGS"
			 fi
			 AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[@%:@include <boost/thread/thread.hpp>]],
                                   [[boost::thread_group thrds;
                                   return 0;]]),
                   ax_cv_boost_thread=yes, ax_cv_boost_thread=no)
			 CXXFLAGS=$CXXFLAGS_SAVE
             AC_LANG_POP([C++])
			])
			if test "x$ax_cv_boost_thread" = "xyes"; then
               if test "x$build_os" = "xsolaris" ; then
 				  BOOST_CPPFLAGS="-pthreads $BOOST_CPPFLAGS"
			   elif test "x$build_os" = "xming32" ; then
 				  BOOST_CPPFLAGS="-mthreads $BOOST_CPPFLAGS"
			   else
				  BOOST_CPPFLAGS="-pthread $BOOST_CPPFLAGS"
			   fi

				AC_SUBST(BOOST_CPPFLAGS)
				AC_DEFINE(HAVE_BOOST_THREAD,,[define if the Boost::THREAD library is available])
				BN=boost_thread
     			LDFLAGS_SAVE=$LDFLAGS
                        case "x$build_os" in
                          *bsd* )
                               LDFLAGS="-pthread $LDFLAGS"
                          break;
                          ;;
                        esac

				for ax_lib in $BN $BN-$CC $BN-$CC-mt $BN-$CC-mt-s $BN-$CC-s \
                              lib$BN lib$BN-$CC lib$BN-$CC-mt lib$BN-$CC-mt-s lib$BN-$CC-s \
                              $BN-mgw $BN-mgw $BN-mgw-mt $BN-mgw-mt-s $BN-mgw-s ; do
				    AC_CHECK_LIB($ax_lib, main, [BOOST_THREAD_LIB="-l$ax_lib" AC_SUBST(BOOST_THREAD_LIB) link_thread="yes" break],
                                 [link_thread="no"])
  				done
				if test "x$link_thread" = "xno"; then
					AC_MSG_NOTICE(Could not link against $ax_lib !)
                else
                    case "x$build_os" in
                       *bsd* )
                       BOOST_LDFLAGS="-pthread $BOOST_LDFLAGS"
                       break;
                       ;;
                    esac
				fi
			fi

			AC_CACHE_CHECK(whether the Boost::IOStreams library is available,
						   ax_cv_boost_iostreams,
						[AC_LANG_PUSH([C++])
			 AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[@%:@include <boost/iostreams/filtering_stream.hpp>
												 @%:@include <boost/range/iterator_range.hpp>
												]],
                                   [[std::string  input = "Hello World!";
									 namespace io = boost::iostreams;
									 io::filtering_istream  in(boost::make_iterator_range(input));
									 return 0;
                                   ]]),
                   ax_cv_boost_iostreams=yes, ax_cv_boost_iostreams=no)
			 AC_LANG_POP([C++])
			])
			if test "x$ax_cv_boost_iostreams" = "xyes"; then
				AC_DEFINE(HAVE_BOOST_IOSTREAMS,,[define if the Boost::IOStreams library is available])
				BN=boost_iostreams
				for ax_lib in $BN $BN-$CC $BN-$CC-mt $BN-$CC-mt-s $BN-$CC-s \
                              lib$BN lib$BN-$CC lib$BN-$CC-mt lib$BN-$CC-mt-s lib$BN-$CC-s \
                              $BN-mgw $BN-mgw $BN-mgw-mt $BN-mgw-mt-s $BN-mgw-s ; do
				    AC_CHECK_LIB($ax_lib, main, [BOOST_IOSTREAMS_LIB="-l$ax_lib" AC_SUBST(BOOST_IOSTREAMS_LIB) link_thread="yes" break],
                                 [link_thread="no"])
  				done
				if test "x$link_thread" = "xno"; then
					AC_MSG_NOTICE(Could not link against $ax_lib !)
				fi
			fi

			AC_CACHE_CHECK(whether the Boost::Serialization library is available,
						   ax_cv_boost_serialization,
						[AC_LANG_PUSH([C++])
			 AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[@%:@include <fstream>
												 @%:@include <boost/archive/text_oarchive.hpp>
                                                 @%:@include <boost/archive/text_iarchive.hpp>
												]],
                                   [[std::ofstream ofs("filename");
									boost::archive::text_oarchive oa(ofs);
									 return 0;
                                   ]]),
                   ax_cv_boost_serialization=yes, ax_cv_boost_serialization=no)
			 AC_LANG_POP([C++])
			])
			if test "x$ax_cv_boost_serialization" = "xyes"; then
				AC_DEFINE(HAVE_BOOST_SERIALIZATION,,[define if the Boost::Serialization library is available])
				BN=boost_serialization
				for ax_lib in $BN $BN-$CC $BN-$CC-mt $BN-$CC-mt-s $BN-$CC-s \
                              lib$BN lib$BN-$CC lib$BN-$CC-mt lib$BN-$CC-mt-s lib$BN-$CC-s \
                              $BN-mgw $BN-mgw $BN-mgw-mt $BN-mgw-mt-s $BN-mgw-s ; do
				    AC_CHECK_LIB($ax_lib, main,
                                 [BOOST_SERIALIZATION_LIB="-l$ax_lib" AC_SUBST(BOOST_SERIALIZATION_LIB) link_serialization="yes" break],
                                 [link_serialization="no"])
  				done
				if test "x$link_serialization" = "xno"; then
					AC_MSG_NOTICE(Could not link against $ax_lib !)
				fi

				BN=boost_wserialization
				for ax_lib in $BN $BN-$CC $BN-$CC-mt $BN-$CC-mt-s $BN-$CC-s \
                              lib$BN lib$BN-$CC lib$BN-$CC-mt lib$BN-$CC-mt-s lib$BN-$CC-s \
                              $BN-mgw $BN-mgw $BN-mgw-mt $BN-mgw-mt-s $BN-mgw-s ; do
				    AC_CHECK_LIB($ax_lib, main,
                                 [BOOST_WSERIALIZATION_LIB="-l$ax_lib" AC_SUBST(BOOST_WSERIALIZATION_LIB) link_wserialization="yes" break],
                                 [link_wserialization="no"])
  				done
				if test "x$link_wserialization" = "xno"; then
					AC_MSG_NOTICE(Could not link against $ax_lib !)
				fi
			fi

			AC_CACHE_CHECK(whether the Boost::Signals library is available,
						   ax_cv_boost_signals,
						[AC_LANG_PUSH([C++])
			 AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[@%:@include <boost/signal.hpp>
												]],
                                   [[boost::signal<void ()> sig;
                                     return 0;
                                   ]]),
                   ax_cv_boost_signals=yes, ax_cv_boost_signals=no)
			 AC_LANG_POP([C++])
			])
			if test "x$ax_cv_boost_signals" = "xyes"; then
				AC_DEFINE(HAVE_BOOST_SIGNALS,,[define if the Boost::Signals library is available])
				BN=boost_signals
				for ax_lib in $BN $BN-$CC $BN-$CC-mt $BN-$CC-mt-s $BN-$CC-s \
                              lib$BN lib$BN-$CC lib$BN-$CC-mt lib$BN-$CC-mt-s lib$BN-$CC-s \
                              $BN-mgw $BN-mgw $BN-mgw-mt $BN-mgw-mt-s $BN-mgw-s ; do
				    AC_CHECK_LIB($ax_lib, main, [BOOST_SIGNALS_LIB="-l$ax_lib" AC_SUBST(BOOST_SIGNALS_LIB) link_signals="yes" break],
                                 [link_signals="no"])
  				done
				if test "x$link_signals" = "xno"; then
					AC_MSG_NOTICE(Could not link against $ax_lib !)
				fi
			fi

			AC_CACHE_CHECK(whether the Boost::Date_Time library is available,
						   ax_cv_boost_date_time,
						[AC_LANG_PUSH([C++])
			 AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[@%:@include <boost/date_time/gregorian/gregorian_types.hpp>
												]],
                                   [[using namespace boost::gregorian; date d(2002,Jan,10);
                                     return 0;
                                   ]]),
                   ax_cv_boost_date_time=yes, ax_cv_boost_date_time=no)
			 AC_LANG_POP([C++])
			])
			if test "x$ax_cv_boost_date_time" = "xyes"; then
				AC_DEFINE(HAVE_BOOST_DATE_TIME,,[define if the Boost::Date_Time library is available])
				BN=boost_date_time
				for ax_lib in $BN $BN-$CC $BN-$CC-mt $BN-$CC-mt-s $BN-$CC-s \
                              lib$BN lib$BN-$CC lib$BN-$CC-mt lib$BN-$CC-mt-s lib$BN-$CC-s \
                              $BN-mgw $BN-mgw $BN-mgw-mt $BN-mgw-mt-s $BN-mgw-s ; do
				    AC_CHECK_LIB($ax_lib, main, [BOOST_DATE_TIME_LIB="-l$ax_lib" AC_SUBST(BOOST_DATE_TIME_LIB) link_thread="yes" break],
                                 [link_thread="no"])
  				done
				if test "x$link_thread"="no" = "xno"; then
					AC_MSG_NOTICE(Could not link against $ax_lib !)
				fi
			fi

			AC_CACHE_CHECK(whether the Boost::Regex library is available,
						   ax_cv_boost_regex,
						[AC_LANG_PUSH([C++])
			 AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[@%:@include <boost/regex.hpp>
												]],
                                   [[boost::regex r(); return 0;]]),
                   ax_cv_boost_regex=yes, ax_cv_boost_regex=no)
			 AC_LANG_POP([C++])
			])
			if test "x$ax_cv_boost_regex" = "xyes"; then
				AC_DEFINE(HAVE_BOOST_REGEX,,[define if the Boost::Regex library is available])
				BN=boost_regex
				for ax_lib in $BN $BN-$CC $BN-$CC-mt $BN-$CC-mt-s $BN-$CC-s \
                              lib$BN lib$BN-$CC lib$BN-$CC-mt lib$BN-$CC-mt-s lib$BN-$CC-s \
                              $BN-mgw $BN-mgw $BN-mgw-mt $BN-mgw-mt-s $BN-mgw-s ; do
				    AC_CHECK_LIB($ax_lib, main, [BOOST_REGEX_LIB="-l$ax_lib" AC_SUBST(BOOST_REGEX_LIB) link_regex="yes" break],
                                 [link_regex="no"])
  				done
				if test "x$link_regex" = "xno"; then
					AC_MSG_NOTICE(Could not link against $ax_lib !)
				fi
			fi

			AC_CACHE_CHECK(whether the Boost::UnitTestFramework library is available,
						   ax_cv_boost_unit_test_framework,
						[AC_LANG_PUSH([C++])
			 AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[@%:@include <boost/test/unit_test.hpp>]],
                                    [[using boost::unit_test::test_suite;
					                 test_suite* test= BOOST_TEST_SUITE( "Unit test example 1" ); return 0;]]),
                   ax_cv_boost_unit_test_framework=yes, ax_cv_boost_unit_test_framework=no)
			 AC_LANG_POP([C++])
			])
			if test "x$ax_cv_boost_unit_test_framework" = "xyes"; then
    		AC_DEFINE(HAVE_BOOST_UNIT_TEST_FRAMEWORK,,[define if the Boost::Unit_test_framework library is available])
			BN=boost_unit_test_framework
    		saved_ldflags="${LDFLAGS}"
			for ax_lib in $BN $BN-$CC $BN-$CC-mt $BN-$CC-mt-s $BN-$CC-s \
                          lib$BN lib$BN-$CC lib$BN-$CC-mt lib$BN-$CC-mt-s lib$BN-$CC-s \
                          $BN-mgw $BN-mgw $BN-mgw-mt $BN-mgw-mt-s $BN-mgw-s ; do
                LDFLAGS="${LDFLAGS} -l$ax_lib"
    			AC_CACHE_CHECK(the name of the Boost::UnitTestFramework library,
	      					   ax_cv_boost_unit_test_framework_link,
						[AC_LANG_PUSH([C++])
                   AC_LINK_IFELSE([AC_LANG_PROGRAM([[@%:@include <boost/test/unit_test.hpp>
                                                     using boost::unit_test::test_suite;
                                                     test_suite* init_unit_test_suite( int argc, char * argv[] ) {
                                                     test_suite* test= BOOST_TEST_SUITE( "Unit test example 1" );
                                                     return test;
                                                     }
                                                   ]],
                                 [[ return 0;]])],
                                 link_unit_test_framework="yes",link_unit_test_framework="no")
			 AC_LANG_POP([C++])
               ])
                LDFLAGS="${saved_ldflags}"
			    if test "x$link_unit_test_framework" = "xyes"; then
                    BOOST_UNIT_TEST_FRAMEWORK_LIB="-l$ax_lib"
                    AC_SUBST(BOOST_UNIT_TEST_FRAMEWORK_LIB)
					break
				fi
              done
			    if test "x$link_unit_test_framework" = "xno"; then
				   AC_MSG_NOTICE(Could not link against $ax_lib !)
				fi
			fi
		fi
        CPPFLAGS="$CPPFLAGS_SAVED"
        LDFLAGS="$LDFLAGS_SAVED"
	fi
])

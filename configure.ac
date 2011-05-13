# -*- autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT(configure.ac)
AM_INIT_AUTOMAKE(petri-foo, 0.0.2)
AM_CONFIG_HEADER(src/config.h)

# compilation
with_debug="no"
AC_ARG_ENABLE([debug], 
    [AC_HELP_STRING([--enable-debug],
        [enable debugging information, accepting a performance penalty (default is NO)])],
    [if test x$enable_debug = xyes; then with_debug=yes ; fi])

if test x$with_debug = xno; then
    CFLAGS="-O3 -std=gnu99"
else
    CFLAGS="-O0 -std=gnu99 -ggdb -Wextra -DGTK_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGSEAL_ENABLE"
    AC_DEFINE(DEBUG, 1, [[whether to display debugging output or not]])
fi

# disable lash support?
with_lash="no"
AC_ARG_ENABLE([lash], 
    [AC_HELP_STRING([--disable-lash],
        [disable lash support (default is YES)])],
    [if test x$enable_lash = xno; then with_lash=no ; fi])

echo with_lash:$with_lash

AM_CONDITIONAL(WITH_LASH, [test "$with_lash" = "yes"])

# standard autoconf checks
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CPP
AC_PROG_RANLIB
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h sys/time.h unistd.h])
AC_C_CONST
AC_C_INLINE
AC_HEADER_TIME
AC_FUNC_MALLOC
AC_CHECK_FUNCS([floor gettimeofday pow strchr strdup])

# pthreads
ACX_PTHREAD

PETRI_FOO_CFLAGS="-Wall"
AC_SUBST(PETRI_FOO_CFLAGS)
CFLAGS="$PETRI_FOO_CFLAGS $CFLAGS"

config_error="no"

# gtk
PKG_CHECK_MODULES(GTK, gtk+-2.0, HAVE_GTK="yes", config_error="yes")
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

# libxml
PKG_CHECK_MODULES(LIBXML, libxml-2.0, HAVE_LIBXML="yes", config_error="yes")
AC_SUBST(LIBXML_CFLAGS)
AC_SUBST(LIBXML_LIBS)

# libgnomecanvas
PKG_CHECK_MODULES(LIBGNOMECANVAS, libgnomecanvas-2.0, HAVE_LIBGNOMECANVAS="yes", config_error="yes")
AC_SUBST(LIBGNOMECANVAS_CFLAGS)
AC_SUBST(LIBGNOMECANVAS_LIBS)

# alsa
PKG_CHECK_MODULES(ALSA, alsa, HAVE_ALSA="yes", config_error="yes")
AC_SUBST(ALSA_CFLAGS)
AC_SUBST(ALSA_LIBS)

# libsndfile
PKG_CHECK_MODULES(LIBSNDFILE, sndfile, HAVE_LIBSNDFILE="yes", config_error="yes")
AC_SUBST(LIBSNDFILE_CFLAGS)
AC_SUBST(LIBSNDFILE_LIBS)

# libsamplerate
PKG_CHECK_MODULES(LIBSAMPLERATE, samplerate, HAVE_LIBSAMPLERATE="yes", config_error="yes")
AC_SUBST(LIBSAMPLERATE_CFLAGS)
AC_SUBST(LIBSAMPLERATE_LIBS)

# jack
PKG_CHECK_MODULES(JACK, jack >= 0.116.0, HAVE_JACK="yes", config_error="yes")
if test "x$HAVE_JACK" = xyes ; then
  AC_CHECK_HEADER( jack/session.h, HAVE_JACK_SESSION="yes", HAVE_JACK_SESSION="no" )
  if test "x$HAVE_JACK_SESSION" = xyes; then
    AC_DEFINE([HAVE_JACK_SESSION], [], [Define if we have jack session support.])
  fi
fi
AC_SUBST(JACK_CFLAGS)
AC_SUBST(JACK_LIBS)

# phat
PKG_CHECK_MODULES(PHAT, phat >= 0.3.1, HAVE_PHAT="yes", config_error="yes")
AC_SUBST(PHAT_CFLAGS)
AC_SUBST(PHAT_LIBS)

if test x$with_lash = xyes; then

have_lash=no
PKG_CHECK_MODULES(LASH, lash-1.0 >= 0.5.0, have_lash="yes", config_error="no")
if test x$have_lash = xyes ; then
  AC_DEFINE([HAVE_LASH], [], [Define if we have lash.])
  AC_SUBST(LASH_CFLAGS)
  AC_SUBST(LASH_LIBS)
  else
    AC_MSG_WARN([LASH not found, session support will not be built.])
  fi
fi

CC="$PTHREAD_CC"

# print build summary
AC_CONFIG_COMMANDS_POST([
echo
echo    "                   BUILD SUMMARY"
echo    "                   ============="
echo    "    Compiler full flags: $CFLAGS"
echo -n "             Build type: "
if test x$with_debug = xyes; then
    echo "debugging"
else
    echo "optimized"
fi
echo -n "      Jack midi support: "
if test "x$HAVE_JACK" = xyes ; then
    echo "yes with jack >= 0.116.0"
fi
echo -n "   Jack session support: "
if test "x$HAVE_JACK_SESSION" = xyes ; then
    echo "yes"
else
    echo "not found"
fi
echo -n "           LASH support: "
if test "x$with_lash" = xyes; then
    echo "yes with lash-1.0 >= 0.5.0"
else
    echo "disabled or not found"
fi

echo
echo
])

# are we good to go?

if test x$config_error = "xyes"; then

AC_MSG_RESULT([
*** ERROR: the following required packages are missing ***
])

if test "x$HAVE_GTK" != xyes ; then
  AC_MSG_RESULT([
*** GTK+ version 2.2.x or greater, available from :
		http://www.gtk.org/
])
fi

if test "x$HAVE_LIBGNOMECANVAS" != xyes ; then
  AC_MSG_RESULT([
*** libgnomecanvas, available from :
		http://ftp.gnome.org/pub/gnome/sources/libgnomecanvas/
])
fi

if test "x$HAVE_PHAT" != xyes ; then
  AC_MSG_RESULT([
*** Phat 0.4.0 or greater, available from:
		http://phat.berlios.de/
])
fi

if test "x$HAVE_JACK" != xyes ; then
  AC_MSG_RESULT([
*** JACK 0.99.0 or greater, available from:
		http://www.jackaudio.org/
])
fi

if test "x$HAVE_LIBSAMPLERATE" != xyes ; then
  AC_MSG_RESULT([
*** libsamplerate, available from:
		http://www.mega-nerd.com/SRC/
])
fi

if test "x$HAVE_LIBSNDFILE" != xyes ; then
  AC_MSG_RESULT([
*** libsndfile, available from:
		http://www.mega-nerd.com/libsndfile/
])
fi

if test "x$HAVE_LIBXML" != xyes ; then
  AC_MSG_RESULT([
*** libxml2, available from:
		http://ftp.gnome.org/pub/gnome/sources/libxml2/
])
fi

if test x$enable_lash = xyes; then
	if test "x$HAVE_LASH" != xyes ; then
  	AC_MSG_RESULT([
*** LASH,  available from:
		http://savannah.nongnu.org/projects/lash
	Note: you may get this message if jack is not installed since lash
    requires jack.	

    To disable lash support, add --disable-lash to
    your configure options.])
	fi
fi

AC_MSG_RESULT([
Please ensure that all the above software is properly installed
before running configure again. To do this, use your package
manager to install the correct versions of the binary AND 
development (dev) packages or, download them from the 
above link(s) and build and install them from source manually.
])

AC_MSG_ERROR([
*************************************************
see ./config.log for full details.
])
else
AC_OUTPUT([
Makefile
pixmaps/Makefile
src/Makefile
src/gui/Makefile
src/patch_private/Makefile
petri-foo.spec
])
fi

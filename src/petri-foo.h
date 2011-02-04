#ifndef __SPECIMEN_H__
#define __SPECIMEN_H__

#include <config.h>
#include <stdio.h>
#include <signal.h>

#ifndef DEBUG
# define DEBUG 0
#endif

enum
{
    FUBAR = -69
};

#define DEFAULT_VOLUME 0.7 /* default volume stuff is set to, from 0 to 1 */

#ifndef PIXMAPSDIR
# define PIXMAPSDIR INSTALLDIR"/petri-foo/pixmaps/"
#endif

#define errmsg(...) {fprintf(stderr, "%20s:%5d\t%30s", __FILE__, __LINE__, __FUNCTION__); fprintf(stderr, ": "); fprintf(stderr, __VA_ARGS__);}

#if DEBUG
# define debug(...) errmsg(__VA_ARGS__)
#else
# define debug(...)
#endif

typedef sig_atomic_t Atomic;

#endif /* __SPECIMEN_H__ */

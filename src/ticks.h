#ifndef __TICKS_H__
#define __TICKS_H__

#include <glib.h>

/* frame count type */
#ifdef G_HAVE_GINT64

typedef guint32 Tick;

#else

typedef guint32 Tick;

#endif /* G_HAVE_GINT64 */

/* functions */
Tick  ticks_get_ticks     ( );
Tick  ticks_secs_to_ticks (float secs);
float ticks_ticks_to_secs (Tick tick);

/* call me sometime, OK? */
void ticks_set_samplerate (int rate);

#endif /* __TICKS_H__ */

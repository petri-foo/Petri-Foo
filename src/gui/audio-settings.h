#ifndef __AUDIO_SETTINGS_H__
#define __AUDIO_SETTINGS_H__

#include <gtk/gtk.h>

#include "config.h"

#ifdef HAVE_JACK_SESSION
#include <jack/session.h>
#endif


void audio_settings_init(GtkWidget* parent);
void audio_settings_show(void);


#ifdef HAVE_JACK_SESSION
void audio_settings_session_cb(jack_session_event_t *event, void *arg);
#endif


#endif /* __AUDIO_SETTINGS_H__ */

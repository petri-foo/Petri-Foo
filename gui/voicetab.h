/*  Petri-Foo is a fork of the Specimen audio sampler.

    Original Specimen author Pete Bessman
    Copyright 2005 Pete Bessman
    Copyright 2011 James W. Morris

    This file is part of Petri-Foo.

    Petri-Foo is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.

    Petri-Foo is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Petri-Foo.  If not, see <http://www.gnu.org/licenses/>.

    This file is a derivative of a Specimen original, modified 2011
*/


#ifndef __VOICE_TAB__
#define __VOICE_TAB__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define VOICE_TAB_TYPE \
    (voice_tab_get_type())

#define VOICE_TAB(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VOICE_TAB_TYPE, VoiceTab))

#define IS_VOICE_TAB(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VOICE_TAB_TYPE))

#define VOICE_TAB_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), VOICE_TAB_TYPE, VoiceTabClass))

#define IS_VOICE_TAB_CLASS(klass) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((klass), VOICE_TAB_TYPE))


typedef struct _VoiceTabClass VoiceTabClass;
typedef struct _VoiceTab      VoiceTab;


struct _VoiceTab
{
    GtkVBox parent;
};

struct _VoiceTabClass
{
    GtkVBoxClass parent_class;
};

GType       voice_tab_get_type(void);
GtkWidget*  voice_tab_new(void);
void        voice_tab_set_patch(VoiceTab* self, int patch);


G_END_DECLS


#endif /* __VOICE_TAB__ */

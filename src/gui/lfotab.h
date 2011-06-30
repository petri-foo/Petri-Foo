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


#ifndef __LFO_TAB__
#define __LFO_TAB__

#include <gtk/gtk.h>


G_BEGIN_DECLS


#define LFO_TAB_TYPE            (lfo_tab_get_type())
#define LFO_TAB(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                LFO_TAB_TYPE, LfoTab))

#define IS_LFO_TAB(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                LFO_TAB_TYPE))

#define LFO_TAB_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                LFO_TAB_TYPE, LfoTabClass))

#define IS_LFO_TAB_CLASS(klass) (G_TYPE_CHECK_INSTANCE_TYPE ((klass),   \
                                LFO_TAB_TYPE))


typedef struct _LfoTabClass LfoTabClass;
typedef struct _LfoTab LfoTab;


struct _LfoTab
{
    GtkHBox parent;
};

struct _LfoTabClass
{
    GtkHBoxClass parent_class;
};


GType       lfo_tab_get_type(void);
GtkWidget*  lfo_tab_new(void);

void        lfo_tab_set_patch(LfoTab* self, int patch);


G_END_DECLS


#endif /* __LFO_TAB__ */

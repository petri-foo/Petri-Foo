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


#ifndef __SAMPLE_TAB__
#define __SAMPLE_TAB__

#include <gtk/gtk.h>

G_BEGIN_DECLS


#define SAMPLE_TAB_TYPE \
    (sample_tab_get_type())

#define SAMPLE_TAB(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SAMPLE_TAB_TYPE, SampleTab))

#define SAMPLE_TAB_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), SAMPLE_TAB_TYPE, SampleTabClass))

#define IS_SAMPLE_TAB(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SAMPLE_TAB_TYPE))

#define IS_SAMPLE_TAB_CLASS(klass) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((klass), SAMPLE_TAB_TYPE))


typedef struct _SampleTabClass  SampleTabClass;
typedef struct _SampleTab       SampleTab;


struct _SampleTab
{
    GtkVBox parent;
};


struct _SampleTabClass
{
    GtkVBoxClass parent_class;
};


GType       sample_tab_get_type(void);

GtkWidget*  sample_tab_new(void);
void        sample_tab_set_patch(SampleTab*, int patch);
void        sample_tab_update_waveforms(SampleTab*);


G_END_DECLS


#endif /* __SAMPLE_TAB__ */

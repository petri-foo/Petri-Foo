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


#ifndef __CHANNEL_SECTION__
#define __CHANNEL_SECTION__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define CHANNEL_SECTION_TYPE \
    (channel_section_get_type())

#define CHANNEL_SECTION(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CHANNEL_SECTION_TYPE, \
                                        ChannelSection))

#define IS_CHANNEL_SECTION(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CHANNEL_SECTION_TYPE))

#define CHANNEL_SECTION_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CHANNEL_SECTION_TYPE, \
                                       ChannelSectionClass))

#define IS_CHANNEL_SECTION_CLASS(klass) \
    (G_TYPE_CHECK_INSTANCE_TYPE((klass), CHANNEL_SECTION_TYPE))


typedef struct _ChannelSectionClass ChannelSectionClass;
typedef struct _ChannelSection      ChannelSection;


struct _ChannelSection
{
    GtkVBox parent;

    /* <private> */
    int patch;
    GtkWidget* chan_sb;
    GtkWidget* lower_vel_sb;
    GtkWidget* upper_vel_sb;
};


struct _ChannelSectionClass
{
    GtkVBoxClass parent_class;
};


GType       channel_section_get_type(void);

GtkWidget*  channel_section_new(void);

void        channel_section_set_patch(ChannelSection* self, int patch);
int         channel_section_get_channel(ChannelSection* self);

G_END_DECLS

#endif /* __CHANNEL_SECTION__ */

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


#ifndef __MIDI_SECTION__
#define __MIDI_SECTION__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MIDI_SECTION_TYPE           (midi_section_get_type())
#define MIDI_SECTION(obj)           (G_TYPE_CHECK_INSTANCE_CAST((obj),  \
                                    MIDI_SECTION_TYPE, MidiSection))

#define IS_MIDI_SECTION(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj),  \
                                    MIDI_SECTION_TYPE))

#define MIDI_SECTION_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),   \
                                    MIDI_SECTION_TYPE, MidiSectionClass))

#define IS_MIDI_SECTION_CLASS(klass)(G_TYPE_CHECK_INSTANCE_TYPE((klass),\
                                    MIDI_SECTION_TYPE))


typedef struct _MidiSectionClass MidiSectionClass;
typedef struct _MidiSection MidiSection;


struct _MidiSection
{
    GtkVBox parent_instance;
};


struct _MidiSectionClass
{
    GtkVBoxClass parent_class;
};


GType       midi_section_get_type(void);
GtkWidget*  midi_section_new(void);

void        midi_section_set_patch(MidiSection* self, int patch);


G_END_DECLS


#endif /* __MIDI_SECTION__ */

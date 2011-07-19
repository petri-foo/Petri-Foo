/*  Petri-Foo is a fork of the Specimen audio sampler.

    Copyright 2011 James W. Morris

    This file is part of Petri-Foo.

    Petri-Foo is free software: you can redistribute it and/or Boolify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.

    Petri-Foo is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Petri-Foo.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef __FLOAT_SECTION__
#define __FLOAT_SECTION__

#include <gtk/gtk.h>


#include "patch.h"


G_BEGIN_DECLS

#define FLOAT_SECTION_TYPE      (float_section_get_type())
#define FLOAT_SECTION(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                FLOAT_SECTION_TYPE, FloatSection))

#define IS_FLOAT_SECTION(obj)   (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                FLOAT_SECTION_TYPE))

#define FLOAT_SECTION_CLASS(klass)(G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                FLOAT_SECTION_TYPE, FloatSectionClass))

#define IS_FLOAT_SECTION_CLASS(klass) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((klass), FLOAT_SECTION_TYPE))


typedef struct _FloatSectionClass FloatSectionClass;
typedef struct _FloatSection      FloatSection;


struct _FloatSection
{
    GtkVBox parent_instance;
};


struct _FloatSectionClass
{
    GtkVBoxClass parent_class;
     /* <private> */
    void (*set_toggled)(FloatSection*);
};


GType       float_section_get_type(void);

GtkWidget*  float_section_new(void);

void        float_section_set_float(FloatSection*, PatchFloatType);
void        float_section_set_patch(FloatSection*, int patch_id);


G_END_DECLS


#endif /* __FLOAT_SECTION__ */

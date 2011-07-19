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


#ifndef __BOOL_SECTION__
#define __BOOL_SECTION__

#include <gtk/gtk.h>


#include "patch.h"


G_BEGIN_DECLS

#define BOOL_SECTION_TYPE       (bool_section_get_type())
#define BOOL_SECTION(obj)       (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                BOOL_SECTION_TYPE, BoolSection))

#define IS_BOOL_SECTION(obj)    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                BOOL_SECTION_TYPE))

#define BOOL_SECTION_CLASS(klass)(G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                BOOL_SECTION_TYPE, BoolSectionClass))

#define IS_BOOL_SECTION_CLASS(klass) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((klass), BOOL_SECTION_TYPE))


typedef struct _BoolSectionClass BoolSectionClass;
typedef struct _BoolSection      BoolSection;


struct _BoolSection
{
    GtkVBox parent_instance;
};


struct _BoolSectionClass
{
    GtkVBoxClass parent_class;
     /* <private> */
    void (*set_toggled)(BoolSection*);
};


GType       bool_section_get_type(void);

GtkWidget*  bool_section_new(void);

void        bool_section_set_bool( BoolSection*, PatchBoolType);
void        bool_section_set_patch(BoolSection*, int patch_id);

gboolean    bool_section_get_active(BoolSection*);

G_END_DECLS


#endif /* __BOOL_SECTION__ */

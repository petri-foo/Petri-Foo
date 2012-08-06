/*  Petri-Foo is a fork of the Specimen audio sampler.

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
*/


#ifndef __MOD_SECTION__
#define __MOD_SECTION__

#include <gtk/gtk.h>


#include "patch.h"


G_BEGIN_DECLS

#define MOD_SECTION_TYPE        (mod_section_get_type())
#define MOD_SECTION(obj)        (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                MOD_SECTION_TYPE, ModSection))

#define IS_MOD_SECTION(obj)     (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                MOD_SECTION_TYPE))

#define MOD_SECTION_CLASS(klass)(G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                MOD_SECTION_TYPE, ModSectionClass))

#define IS_MOD_SECTION_CLASS(klass) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((klass), MOD_SECTION_TYPE))


typedef struct _ModSectionClass ModSectionClass;
typedef struct _ModSection      ModSection;


struct _ModSection
{
    GtkVBox parent;
};


struct _ModSectionClass
{
    GtkVBoxClass parent_class;
};


GType       mod_section_get_type(void);

GtkWidget*  mod_section_new(void);
void        mod_section_set_mod_only(ModSection*);
void        mod_section_set_param(ModSection*, PatchParamType);

void        mod_section_set_lfo_id(ModSection*, int lfo_id);

void        mod_section_set_patch(ModSection*, int patch_id);


G_END_DECLS


#endif /* __MOD_SECTION__ */

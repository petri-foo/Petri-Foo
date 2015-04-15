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

#include <assert.h>
#include <gtk/gtk.h>

#include "phin.h"

#include "paramtab.h"
#include "gui.h"
#include "patch.h"

#include "mod_src_gui.h"
#include "mod_section.h"


typedef struct _ParamTabPrivate ParamTabPrivate;

#define PARAM_TAB_GET_PRIVATE(obj)      \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
        PARAM_TAB_TYPE, ParamTabPrivate))

struct _ParamTabPrivate
{
    int         patch_id;
    guint       refresh;

    PatchParamType  param;

    GtkWidget*  modsect1; /* 1st param with modulation */
    GtkWidget*  modsect2; /* optional 2nd param with modulation */
};


G_DEFINE_TYPE(ParamTab, param_tab, GTK_TYPE_VBOX);


static void param_tab_class_init(ParamTabClass* klass)
{
    GtkObjectClass *object_class = GTK_OBJECT_CLASS(klass);
    param_tab_parent_class = g_type_class_peek_parent(klass);
    g_type_class_add_private(object_class, sizeof(ParamTabPrivate));
}


static void param_tab_init(ParamTab* self)
{
    ParamTabPrivate* p = PARAM_TAB_GET_PRIVATE(self);
    p->patch_id = -1;
    p->param = PATCH_PARAM_INVALID;
    p->modsect1 = 0;
    p->modsect2 = 0;
}


GtkWidget* param_tab_new(void)
{
    return (GtkWidget*) g_object_new(PARAM_TAB_TYPE, NULL);
}

void param_tab_set_param(ParamTab* self, PatchParamType param)
{
    ParamTabPrivate* p = PARAM_TAB_GET_PRIVATE(self);
    GtkBox* box = GTK_BOX(self);

    PatchParamType ms1 = PATCH_PARAM_INVALID;
    PatchParamType ms2 = PATCH_PARAM_INVALID;

    switch(param)
    {
    case PATCH_PARAM_AMPLITUDE:
    case PATCH_PARAM_PANNING:
        ms1 = PATCH_PARAM_AMPLITUDE;
        ms2 = PATCH_PARAM_PANNING;
        break;

    case PATCH_PARAM_CUTOFF:
    case PATCH_PARAM_RESONANCE:
        ms1 = PATCH_PARAM_CUTOFF;
        ms2 = PATCH_PARAM_RESONANCE;
        break;

    case PATCH_PARAM_PITCH:
        ms1 = PATCH_PARAM_PITCH;
        break;

    default:
        assert(0);
        return;
    }

    if (ms1 != PATCH_PARAM_INVALID)
    {
        p->modsect1 = mod_section_new();
        mod_section_set_param(MOD_SECTION(p->modsect1), ms1);
        gtk_box_pack_start(box, p->modsect1, FALSE, FALSE, 0);
        gtk_widget_show(p->modsect1);
    }

    if (ms2 != PATCH_PARAM_INVALID)
    {
        p->modsect2 = mod_section_new();
        mod_section_set_param(MOD_SECTION(p->modsect2), ms2);
        gtk_box_pack_start(box, p->modsect2, FALSE, FALSE, 0);
        gtk_widget_show(p->modsect2);
    }
}

void param_tab_set_patch(ParamTab* self, int patch)
{
    ParamTabPrivate* p = PARAM_TAB_GET_PRIVATE(self);
    if (p->modsect1)
        mod_section_set_patch(MOD_SECTION(p->modsect1), patch);

    if (p->modsect2)
        mod_section_set_patch(MOD_SECTION(p->modsect2), patch);
}

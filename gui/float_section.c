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


#include <gtk/gtk.h>

#include "phin.h"

#include "float_section.h"
#include "gui.h"
#include "patch_set_and_get.h"
#include "names.h"

#include "mod_src_gui.h"


typedef struct _FloatSectionPrivate FloatSectionPrivate;

#define FLOAT_SECTION_GET_PRIVATE(obj)  \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
        FLOAT_SECTION_TYPE, FloatSectionPrivate))

struct _FloatSectionPrivate
{
    int             patch_id;
    PatchFloatType  float_type;

    GtkWidget*      float_fan;
    GtkWidget*      mod_combo;
    GtkWidget*      mod_amt;
};


G_DEFINE_TYPE(FloatSection, float_section, GTK_TYPE_VBOX);


static void float_section_class_init(FloatSectionClass* klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    float_section_parent_class = g_type_class_peek_parent(klass);
    g_type_class_add_private(object_class, sizeof(FloatSectionPrivate));
}


static void float_cb(GtkWidget* w, FloatSectionPrivate* p)
{
    float val = phin_fan_slider_get_value(PHIN_FAN_SLIDER(w));
    patch_float_set_assign(p->patch_id, p->float_type, val);
}


static void mod_combo_cb(GtkComboBox* combo, FloatSectionPrivate* p)
{
    int mod_src = mod_src_combo_get_mod_src_id(combo);
    patch_float_set_mod_src(p->patch_id, p->float_type, mod_src);
    gtk_widget_set_sensitive(p->mod_amt, (mod_src != MOD_SRC_NONE));
}


static void mod_amt_cb(GtkWidget* w, FloatSectionPrivate* p)
{
    patch_float_set_mod_amt(p->patch_id,
                            p->float_type,
                            phin_fan_slider_get_value(PHIN_FAN_SLIDER(w)));
}


static void block(FloatSectionPrivate* p)
{
    g_signal_handlers_block_by_func(p->float_fan,   float_cb,       p);
    g_signal_handlers_block_by_func(p->mod_amt,     mod_amt_cb,     p);
    g_signal_handlers_block_by_func(p->mod_combo,   mod_combo_cb,   p);
}


static void unblock(FloatSectionPrivate* p)
{
    g_signal_handlers_unblock_by_func(p->float_fan, float_cb,       p);
    g_signal_handlers_unblock_by_func(p->mod_amt,   mod_amt_cb,     p);
    g_signal_handlers_unblock_by_func(p->mod_combo, mod_combo_cb,   p);
}



static void float_section_init(FloatSection* self)
{
    FloatSectionPrivate* p = FLOAT_SECTION_GET_PRIVATE(self);
    p->patch_id = -1;
    p->float_type = PATCH_FLOAT_INVALID;
    p->float_fan = 0;
    p->mod_combo = 0;
    p->mod_amt = 0;
}


void float_section_set_float( FloatSection* self, PatchFloatType float_type)
{
    FloatSectionPrivate* p = FLOAT_SECTION_GET_PRIVATE(self);
    GtkBox* box;
    GtkWidget* table;
    GtkTable* t;

    int y = 0;

    int a1 = 0, a2 = 1;
    int b1 = 1, b2 = 2;
    int c1 = 2, c2 = 3;

    float floatmin, floatmax, floatstep;

    const char* float_names[] = { "Portamento Time" };

    box = GTK_BOX(self);

    p->float_type = float_type;

    switch(float_type)
    {
    case PATCH_FLOAT_PORTAMENTO_TIME:
        floatmin = 0.0;
        floatmax = 1.0;
        floatstep = 0.01;
        break;

    default:
        debug("Bad News! unknown float type!\n");
        return;
    }

    gtk_container_set_border_width(GTK_CONTAINER(self), GUI_BORDERSPACE);

    table = gtk_table_new(3, 3, FALSE);
    t = (GtkTable*)table;
    gui_pack(box, table);

    gui_attach(t, gui_title_new(float_names[float_type]), a1, c2, y, y + 1);
    ++y;

    /* title padding */
    gui_attach(t, gui_vpad_new(GUI_TITLESPACE), a1, c2, y, y + 1);
    ++y;

    /* label column spacing (of some description!?) */
    gui_attach(t, gui_hpad_new(GUI_TEXTSPACE), c1, c2, y, y + 1);
    ++y;

    p->float_fan = phin_hfan_slider_new_with_range(0.0, floatmin,
                                                        floatmax,
                                                        floatstep);
    gui_attach(t, p->float_fan, b1, b2, y, y + 1);
    ++y;

    gui_label_attach("Mod:", t, a1, a2, y, y + 1);

    p->mod_combo = mod_src_new_combo_with_cell();
    mod_src_combo_set_model(GTK_COMBO_BOX(p->mod_combo), MOD_SRC_GLOBALS);
    gui_attach(t, p->mod_combo, b1, b2, y, y + 1);

    p->mod_amt = phin_hfan_slider_new_with_range(0.0, -1.0, 1.0, 0.1);
    gui_attach(t, p->mod_amt, c1, c2, y, y + 1);
    ++y;

    g_signal_connect(G_OBJECT(p->float_fan),        "value-changed",
                        G_CALLBACK(float_cb),       (gpointer)p);

    g_signal_connect(G_OBJECT(p->mod_amt),          "value-changed",
                        G_CALLBACK(mod_amt_cb),     (gpointer)p);

    g_signal_connect(G_OBJECT(p->mod_combo),        "changed",
                        G_CALLBACK(mod_combo_cb),   (gpointer)p);
}


void float_section_set_list_global(FloatSection* self)
{
    FloatSectionPrivate* p = FLOAT_SECTION_GET_PRIVATE(self);

    mod_src_combo_set_model(GTK_COMBO_BOX(p->mod_combo), MOD_SRC_GLOBALS);
}


void float_section_set_list_all(FloatSection* self)
{
    FloatSectionPrivate* p = FLOAT_SECTION_GET_PRIVATE(self);

    mod_src_combo_set_model(GTK_COMBO_BOX(p->mod_combo), MOD_SRC_ALL);
}


GtkWidget* float_section_new(void)
{
    return (GtkWidget*)g_object_new(FLOAT_SECTION_TYPE, NULL);
}


void float_section_set_patch(FloatSection* self, int patch_id)
{
    FloatSectionPrivate* p = FLOAT_SECTION_GET_PRIVATE(self);

    float   assign;
    int     modsrc;
    float   mod_amt;

    GtkTreeIter moditer;

    p->patch_id = patch_id;

    if (patch_id < 0)
        return;

    patch_float_get_all(patch_id, p->float_type,    &assign,
                                                    &mod_amt,
                                                    &modsrc);
    block(p);

    phin_fan_slider_set_value(PHIN_FAN_SLIDER(p->float_fan), assign);
    phin_fan_slider_set_value(PHIN_FAN_SLIDER(p->mod_amt), mod_amt);

    if (!mod_src_combo_get_iter_with_id(GTK_COMBO_BOX(p->mod_combo),
                                        modsrc,
                                        &moditer))
    {
        debug("failed to get mod source id from combo box\n");
    }

    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(p->mod_combo), &moditer);

    unblock(p);
}


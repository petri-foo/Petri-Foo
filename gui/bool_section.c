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

#include "bool_section.h"
#include "gui.h"
#include "patch_set_and_get.h"
#include "names.h"

#include "mod_src_gui.h"


typedef struct _BoolSectionPrivate BoolSectionPrivate;

#define BOOL_SECTION_GET_PRIVATE(obj)   \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
        BOOL_SECTION_TYPE, BoolSectionPrivate))

struct _BoolSectionPrivate
{
    int             patch_id;
    PatchBoolType   bool_type;

    GtkWidget*      set_check;
    GtkWidget*      mod_combo;
    GtkWidget*      thresh;

    GtkWidget*      mod_label;
};

/* signals */
enum
{
    TOGGLED,
    LAST_SIGNAL,
};

static int signals[LAST_SIGNAL];


G_DEFINE_TYPE(BoolSection, bool_section, GTK_TYPE_VBOX);


static void bool_section_class_init(BoolSectionClass* klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    bool_section_parent_class = g_type_class_peek_parent(klass);

    signals[TOGGLED] =
        g_signal_new   ("toggled",
                        G_TYPE_FROM_CLASS(klass),
                        G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                        G_STRUCT_OFFSET(BoolSectionClass, set_toggled),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__VOID, G_TYPE_NONE,
                        0, NULL);

    g_type_class_add_private(object_class, sizeof(BoolSectionPrivate));
}


static void set_check_cb(GtkToggleButton* button, BoolSection* self)
{
    BoolSectionPrivate* p = BOOL_SECTION_GET_PRIVATE(self);
    int mod_src = mod_src_combo_get_mod_src_id(GTK_COMBO_BOX(p->mod_combo));
    gboolean active = gtk_toggle_button_get_active(button);

    gtk_widget_set_sensitive(p->mod_label,      active);
    gtk_widget_set_sensitive(p->mod_combo,      active);
    gtk_widget_set_sensitive(p->thresh, active && mod_src != MOD_SRC_NONE);

    patch_bool_set_active(p->patch_id, p->bool_type, active);

    g_signal_emit_by_name(G_OBJECT(self), "toggled");
}


static void mod_combo_cb(GtkComboBox* combo, BoolSectionPrivate* p)
{
    int mod_src = mod_src_combo_get_mod_src_id(combo);

    patch_bool_set_mod_src( p->patch_id, p->bool_type, mod_src);

    gtk_widget_set_sensitive(p->thresh, (mod_src != MOD_SRC_NONE));
}


static void thresh_cb(GtkWidget* w, BoolSectionPrivate* p)
{
    patch_bool_set_thresh(  p->patch_id,
                            p->bool_type,
                            phin_fan_slider_get_value(PHIN_FAN_SLIDER(w)));
}


static void block(BoolSectionPrivate* p)
{
    g_signal_handlers_block_by_func(p->set_check,   set_check_cb,   p);
    g_signal_handlers_block_by_func(p->thresh,      thresh_cb,      p);
    g_signal_handlers_block_by_func(p->mod_combo,   mod_combo_cb,   p);
}


static void unblock(BoolSectionPrivate* p)
{
    g_signal_handlers_unblock_by_func(p->set_check, set_check_cb,   p);
    g_signal_handlers_unblock_by_func(p->thresh,    thresh_cb,      p);
    g_signal_handlers_unblock_by_func(p->mod_combo, mod_combo_cb,   p);
}



static void bool_section_init(BoolSection* self)
{
    BoolSectionPrivate* p = BOOL_SECTION_GET_PRIVATE(self);
    p->patch_id = -1;
    p->bool_type = PATCH_BOOL_INVALID;
    p->set_check = 0;
    p->mod_combo = 0;
    p->thresh = 0;
}


void bool_section_set_bool( BoolSection* self, PatchBoolType bool_type)
{
    BoolSectionPrivate* p = BOOL_SECTION_GET_PRIVATE(self);
    GtkBox* box;
    GtkWidget* table;
    GtkTable* t;
    GtkWidget* title;

    int y = 0;

    int a1 = 0, a2 = 1;
    int b1 = 1, b2 = 2;
    int c1 = 2, c2 = 3;

    const char* bool_names[] = { "Portamento", "Mono", "Legato" };

    box = GTK_BOX(self);

    p->bool_type = bool_type;

    if (bool_type < 0 || bool_type > 2)
    {
        debug("Bad News! unknown bool type!\n");
        return;
    }

    gtk_container_set_border_width(GTK_CONTAINER(self), GUI_BORDERSPACE);

    table = gtk_table_new(3, 3, FALSE);
    t = (GtkTable*)table;
    gui_pack(box, table);

    title = gui_title_new(bool_names[bool_type]);
    p->set_check = gtk_check_button_new();
    gtk_container_add(GTK_CONTAINER(p->set_check), title);
    gui_attach(t, p->set_check, a1, c2, y, y + 1);
    gtk_widget_show(title);
    ++y;

    /* title padding */
    gui_attach(t, gui_vpad_new(GUI_TITLESPACE), a1, c2, y, y + 1);
    ++y;

    /* label column spacing (of some description!?) */
    gui_attach(t, gui_hpad_new(GUI_TEXTSPACE), c1, c2, y, y + 1);
    ++y;

    p->mod_label = gui_label_attach("Switch:", t, a1, a2, y, y + 1);

    p->mod_combo = mod_src_new_combo_with_cell();
    mod_src_combo_set_model(GTK_COMBO_BOX(p->mod_combo), MOD_SRC_GLOBALS);
    gui_attach(t, p->mod_combo, b1, b2, y, y + 1);

    p->thresh = phin_hfan_slider_new_with_range(0.0, -1.0, 1.0, 0.1);
    gui_attach(t, p->thresh, c1, c2, y, y + 1);
    gtk_widget_set_tooltip_text(p->thresh, "Switch threshold");
    ++y;

    g_signal_connect(G_OBJECT(p->set_check),       "toggled",
                        G_CALLBACK(set_check_cb),      (gpointer)self);

    g_signal_connect(G_OBJECT(p->thresh),           "value-changed",
                        G_CALLBACK(thresh_cb),          (gpointer)p);

    g_signal_connect(G_OBJECT(p->mod_combo),        "changed",
                            G_CALLBACK(mod_combo_cb),   (gpointer)p);
}


void bool_section_set_list_global(BoolSection* self)
{
    BoolSectionPrivate* p = BOOL_SECTION_GET_PRIVATE(self);

    mod_src_combo_set_model(GTK_COMBO_BOX(p->mod_combo), MOD_SRC_GLOBALS);
}


void bool_section_set_list_all(BoolSection* self)
{
    BoolSectionPrivate* p = BOOL_SECTION_GET_PRIVATE(self);

    mod_src_combo_set_model(GTK_COMBO_BOX(p->mod_combo), MOD_SRC_ALL);
}


GtkWidget* bool_section_new(void)
{
    return (GtkWidget*)g_object_new(BOOL_SECTION_TYPE, NULL);
}


void bool_section_set_patch(BoolSection* self, int patch_id)
{
    BoolSectionPrivate* p = BOOL_SECTION_GET_PRIVATE(self);

    bool    set;
    float   thresh;
    int     modsrc;

    GtkTreeIter moditer;

    p->patch_id = patch_id;

    if (patch_id < 0)
        return;

    patch_bool_get_all(patch_id, p->bool_type, &set, &thresh, &modsrc);

    block(p);

    phin_fan_slider_set_value(PHIN_FAN_SLIDER(p->thresh), thresh);

    if (!mod_src_combo_get_iter_with_id(GTK_COMBO_BOX(p->mod_combo),
                                        modsrc,
                                        &moditer))
    {
        debug("failed to get mod source id from combo box\n");
    }

    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(p->mod_combo), &moditer);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->set_check), set);

    unblock(p);
}


gboolean bool_section_get_active(BoolSection* self)
{
    BoolSectionPrivate* p = BOOL_SECTION_GET_PRIVATE(self);

    return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->set_check));
}

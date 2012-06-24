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

#include <assert.h>
#include <gtk/gtk.h>

#include "phin.h"

#include "mod_section.h"
#include "gui.h"
#include "patch_set_and_get.h"
#include "names.h"

#include "mod_src_gui.h"


typedef struct _ModSectionPrivate ModSectionPrivate;

#define MOD_SECTION_GET_PRIVATE(obj)   \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
        MOD_SECTION_TYPE, ModSectionPrivate))

struct _ModSectionPrivate
{
    int             patch_id;
    int             refresh;
    gboolean        mod_only;
    PatchParamType  param;
    GtkWidget*      param1;
    GtkWidget*      param2;
    GtkWidget*      vel_sens;
    GtkWidget*      key_track;

    GtkWidget*      mod_combo[MAX_MOD_SLOTS];
    GtkWidget*      mod_amount[MAX_MOD_SLOTS];
};


G_DEFINE_TYPE(ModSection, mod_section, GTK_TYPE_VBOX);


static void mod_section_class_init(ModSectionClass* klass)
{
    GtkObjectClass *object_class = GTK_OBJECT_CLASS(klass);
    mod_section_parent_class = g_type_class_peek_parent(klass);
    g_type_class_add_private(object_class, sizeof(ModSectionPrivate));
}


static void param1_cb(GtkWidget* w, ModSectionPrivate* p)
{
    float val = phin_slider_get_value(PHIN_SLIDER(w));
    patch_param_set_value(p->patch_id, p->param, val);
}

static void param2_cb(GtkWidget* w, ModSectionPrivate* p)
{
    if (p->param == PATCH_PARAM_PITCH)
    {
        float val = phin_slider_button_get_value(PHIN_SLIDER_BUTTON(w));
        patch_set_pitch_steps(p->patch_id, val);
    }
}

static void vel_sens_cb(GtkWidget* w, ModSectionPrivate* p)
{
    float val  = phin_slider_get_value(PHIN_SLIDER(w));
    patch_param_set_vel_amount(p->patch_id, p->param, val);
}

static void key_track_cb(GtkWidget* w, ModSectionPrivate* p)
{
    float val  = phin_slider_get_value(PHIN_SLIDER(w));
    patch_param_set_key_amount(p->patch_id, p->param, val);
}


static void mod_src_cb(GtkComboBox* combo, ModSectionPrivate* p)
{
    int i;
    gboolean active;

    for (i = 0; i < MAX_MOD_SLOTS; ++i)
        if (combo == GTK_COMBO_BOX(p->mod_combo[i]))
            break;

    if (i == MAX_MOD_SLOTS)
    {
        debug ("mod_src_cb called from unrecognised combo box\n");
        return;
    }

    active = mod_src_callback_helper(p->patch_id, i, combo, p->param);

    if (p->mod_amount[i])
        gtk_widget_set_sensitive(p->mod_amount[i], active);
}

static void mod_amount_cb(GtkWidget* w, ModSectionPrivate* p)
{
    int i;
    float val;
    int last_slot = MAX_MOD_SLOTS;

    if (p->param == PATCH_PARAM_AMPLITUDE)
        --last_slot;

    if (p->param == PATCH_PARAM_PITCH)
        val = phin_slider_button_get_value(PHIN_SLIDER_BUTTON(w))
                                                / PATCH_MAX_PITCH_STEPS;
    else
        val = phin_slider_get_value(PHIN_SLIDER(w));

    for (i = 0; i < last_slot; ++i)
        if (w == p->mod_amount[i])
            break;

    if (i == last_slot)
    {
        debug ("mod_amount_cb called from unrecognised widget\n");
        return;
    }

    patch_param_set_mod_amt(p->patch_id, p->param, i, val);
}


static void connect(ModSectionPrivate* p)
{
    int i;

    int last_slot = MAX_MOD_SLOTS;

    if (p->param == PATCH_PARAM_AMPLITUDE)
        --last_slot;

    if (p->mod_only)
        goto connect_mod_srcs;

    g_signal_connect(G_OBJECT(p->param1),       "value-changed",
                        G_CALLBACK(param1_cb),      (gpointer)p);

    if (p->param == PATCH_PARAM_PITCH)
        g_signal_connect(G_OBJECT(p->param2),   "value-changed",
                        G_CALLBACK(param2_cb),      (gpointer)p);

    g_signal_connect(G_OBJECT(p->vel_sens),     "value-changed",
                        G_CALLBACK(vel_sens_cb),    (gpointer)p);

    g_signal_connect(G_OBJECT(p->key_track),    "value-changed",
                        G_CALLBACK(key_track_cb),   (gpointer)p);

connect_mod_srcs:

    for (i = 0; i < MAX_MOD_SLOTS; ++i)
    {
        g_signal_connect(G_OBJECT(p->mod_combo[i]), "changed",
                            G_CALLBACK(mod_src_cb), (gpointer)p);

        if (i < last_slot) /* prevent 'ENV' slot amount connecting */
        {
            g_signal_connect(G_OBJECT(p->mod_amount[i]),    "value-changed",
                                G_CALLBACK(mod_amount_cb),  (gpointer)p);
        }
    }
}


static void block(ModSectionPrivate* p)
{
    int i;

    int last_slot = MAX_MOD_SLOTS;

    if (p->param == PATCH_PARAM_AMPLITUDE)
        --last_slot;

    if (p->mod_only)
        goto block_mod_srcs;

    g_signal_handlers_block_by_func(p->param1, param1_cb, p);

    if (p->param == PATCH_PARAM_PITCH)
        g_signal_handlers_block_by_func(p->param2, param2_cb, p);

    g_signal_handlers_block_by_func(p->key_track, key_track_cb, p);
    g_signal_handlers_block_by_func(p->vel_sens, vel_sens_cb, p);

block_mod_srcs:

    for (i = 0; i < MAX_MOD_SLOTS; ++i)
    {
        g_signal_handlers_block_by_func(p->mod_combo[i],  mod_src_cb, p);

        if (i < last_slot)
            g_signal_handlers_block_by_func(p->mod_amount[i], mod_amount_cb,                                                                       p);
    }
}


static void unblock(ModSectionPrivate* p)
{
    int i;

    int last_slot = MAX_MOD_SLOTS;

    if (p->param == PATCH_PARAM_AMPLITUDE)
        --last_slot;

    if (p->mod_only)
        goto unblock_mod_srcs;

    g_signal_handlers_unblock_by_func(p->param1, param1_cb, p);

    if (p->param == PATCH_PARAM_PITCH)
        g_signal_handlers_unblock_by_func(p->param2, param2_cb,  p);

    g_signal_handlers_unblock_by_func(p->key_track, key_track_cb, p);
    g_signal_handlers_unblock_by_func(p->vel_sens, vel_sens_cb, p);

unblock_mod_srcs:

    for (i = 0; i < MAX_MOD_SLOTS; ++i)
    {
        g_signal_handlers_unblock_by_func(p->mod_combo[i],  mod_src_cb, p);

        if (i < last_slot)
            g_signal_handlers_unblock_by_func(p->mod_amount[i],
                                                 mod_amount_cb, p);
    }
}


static void mod_section_init(ModSection* self)
{
    ModSectionPrivate* p = MOD_SECTION_GET_PRIVATE(self);
    p->patch_id = -1;
    p->mod_only = FALSE;
    p->param = PATCH_PARAM_INVALID;
    p->refresh = -1;
}


void mod_section_set_mod_only(ModSection* self)
{
    ModSectionPrivate* p = MOD_SECTION_GET_PRIVATE(self);
    p->mod_only = TRUE;
}


void mod_section_set_param(ModSection* self, PatchParamType param)
{
    ModSectionPrivate* p = MOD_SECTION_GET_PRIVATE(self);
    GtkBox* box;
    GtkWidget* table;
    GtkTable* t;

    float range_low = 0.0;
    float range_hi = 1.0;
    gboolean logscale = FALSE;

    const char* lstr;
    const char** param_names = names_params_get();

    char buf[80];

    int i;
    int y = 0;
    int env_slot = -1;
    int last_mod_slot = MAX_MOD_SLOTS;

    int a1 = 0, a2 = 1;
    int b1 = 1, b2 = 2;
    int c1 = 2, c2 = 3;

debug("creating mod section...\n");

    box = GTK_BOX(self);
    p->param = param;

    gtk_container_set_border_width(GTK_CONTAINER(self), GUI_BORDERSPACE);

    /* table */
    table = gtk_table_new(8, 3, FALSE);
    t = (GtkTable*)table;
    gui_pack(box, table);

    /* title */
    gui_attach(t, gui_title_new(param_names[param]), a1, c2, y, y + 1);
    ++y;

    /* title padding */
    gui_attach(t, gui_vpad_new(GUI_TITLESPACE), a1, c2, y, y + 1);
    ++y;

    /* label column spacing (of some description!?) */
    gui_attach(t, gui_hpad_new(GUI_TEXTSPACE), c1, c2, y, y + 1);
    ++y;

    if (p->mod_only)
        goto create_mod_srcs;

    switch(param)
    {
    case PATCH_PARAM_AMPLITUDE:
        lstr = "Level:";
        env_slot = EG_MOD_SLOT;
        --last_mod_slot;
        break;

    case PATCH_PARAM_PANNING:
        lstr = "Position:";
        range_low = -1.0;
        break;

    case PATCH_PARAM_PITCH:
        lstr = "Tuning:";
        range_low = -1.0;
        break;
    case PATCH_PARAM_RESONANCE:
        lstr = "Q:";
        /* broken impl in Phat/Phin: logscale = TRUE; */
        break;
    case PATCH_PARAM_CUTOFF:
        snprintf(buf, 80, "%s:", param_names[param]);
        lstr = buf;
        break;
    default:
        assert(0);
        return;
    }

    gui_label_attach(lstr, t, a1, a2, y, y + 1);

    p->param1 = phin_hslider_new_with_range(0.0, range_low,
                                                        range_hi,   0.1);
    phin_slider_set_log(PHIN_SLIDER(p->param1), logscale);

    gui_attach(t, p->param1, b1, b2, y, y + 1);

    if (param == PATCH_PARAM_PITCH)
    {
        p->param2 = mod_src_new_pitch_adjustment();
        gui_attach(t, p->param2, c1, c2, y, y + 1);
    }
    ++y;

    /* velocity sensitivity */
    gui_label_attach("Vel.Sens:", t, a1, a2, y, y + 1);
    p->vel_sens = phin_hslider_new_with_range(0.0, -1.0, 1.0, 0.1);
    gui_attach(t, p->vel_sens, b1, b2, y, y + 1);
    ++y;

    /* key tracking */
    gui_label_attach("Key Track:", t, a1, a2, y, y + 1);
    p->key_track = phin_hslider_new_with_range(0.0, -1.0, 1.0, 0.1);
    gui_attach(t, p->key_track, b1, b2, y, y + 1);
    ++y;

    if (param == PATCH_PARAM_AMPLITUDE)
    {
        /* env input source */
        gui_label_attach("Env:", t, a1, a2, y, y + 1);
        p->mod_combo[env_slot] = mod_src_new_combo_with_cell();
        gui_attach(t, p->mod_combo[env_slot], b1, b2, y, y + 1);
        ++y;
    }


create_mod_srcs:

    for (i = 0; i < last_mod_slot; ++i)
    {
        char buf[10];
        snprintf(buf, 10, "Mod%d:", i + 1);
        buf[9] = '\0'; /* paranoiac critical method */

        gui_label_attach(buf, t, a1, a2, y, y + 1);
        p->mod_combo[i] = mod_src_new_combo_with_cell();
        gui_attach(t, p->mod_combo[i], b1, b2, y, y + 1);

        if (param == PATCH_PARAM_PITCH)
            p->mod_amount[i] = mod_src_new_pitch_adjustment();
        else
            p->mod_amount[i] = phin_hslider_new_with_range(0.0, -1.0,
                                                            1.0, 0.1);
        gui_attach(t, p->mod_amount[i], c1, c2, y, y + 1);
        ++y;
    }

    if (last_mod_slot < MAX_MOD_SLOTS)
        p->mod_amount[last_mod_slot] = 0;

    connect(p);
}


void mod_section_set_list_global(ModSection* self)
{
    ModSectionPrivate* p = MOD_SECTION_GET_PRIVATE(self);
    int i;

    for (i = 0; i < MAX_MOD_SLOTS; ++i)
        mod_src_combo_set_model(GTK_COMBO_BOX(p->mod_combo[i]),
                                    MOD_SRC_GLOBALS);
}


void mod_section_set_list_all(ModSection* self)
{
    ModSectionPrivate* p = MOD_SECTION_GET_PRIVATE(self);

    int i;

    for (i = 0; i < MAX_MOD_SLOTS; ++i)
        mod_src_combo_set_model(GTK_COMBO_BOX(p->mod_combo[i]),
                                    MOD_SRC_ALL);
}


GtkWidget* mod_section_new(void)
{
    return (GtkWidget*)g_object_new(MOD_SECTION_TYPE, NULL);
}


void mod_section_set_patch(ModSection* self, int patch_id)
{
    ModSectionPrivate* p = MOD_SECTION_GET_PRIVATE(self);
    float param1 = 0;
    float param2 = 0;
    float vel_amt = 0;
    float key_trk = 0;

    int     i;
    int     last_slot = MAX_MOD_SLOTS;
    int     modsrc[MAX_MOD_SLOTS];
    float   modamt[MAX_MOD_SLOTS];

    GtkTreeIter moditer[MAX_MOD_SLOTS];

    p->patch_id = patch_id;

    if (patch_id < 0)
        return;

    if (p->mod_only)
        goto get_mod_srcs;

    param1 = patch_param_get_value(p->patch_id, p->param);

    if (p->param == PATCH_PARAM_PITCH)
        param2 = patch_get_pitch_steps(p->patch_id);
    else if (p->param == PATCH_PARAM_AMPLITUDE)
        --last_slot;

    vel_amt = patch_param_get_vel_amount(patch_id, p->param);
    key_trk = patch_param_get_key_amount(patch_id, p->param);

get_mod_srcs:

    for (i = 0; i < MAX_MOD_SLOTS; ++i)
    {
        modsrc[i] = patch_param_get_mod_src(patch_id, p->param, i);

        if (i < last_slot)
            modamt[i] = patch_param_get_mod_amt(patch_id, p->param, i);

        if (!mod_src_combo_get_iter_with_id(GTK_COMBO_BOX(p->mod_combo[i]),
                                                    modsrc[i], &moditer[i]))
        {
            debug("failed to get mod%d source id from combo box\n", i);
        }
    }

    block(p);

    if (p->mod_only)
        goto set_mod_srcs;

    phin_slider_set_value(PHIN_SLIDER(p->param1), param1);

    if (p->param == PATCH_PARAM_PITCH)
        phin_slider_button_set_value(PHIN_SLIDER_BUTTON(p->param2), param2);

    phin_slider_set_value(PHIN_SLIDER(p->key_track), key_trk);
    phin_slider_set_value(PHIN_SLIDER(p->vel_sens),  vel_amt);

set_mod_srcs:

    for (i = 0; i < MAX_MOD_SLOTS; ++i)
    {
        gtk_combo_box_set_active_iter(GTK_COMBO_BOX(p->mod_combo[i]),
                                                        &moditer[i]);
        if (p->param == PATCH_PARAM_PITCH)
        {
            phin_slider_button_set_value(
                                PHIN_SLIDER_BUTTON(p->mod_amount[i]),
                                modamt[i] * PATCH_MAX_PITCH_STEPS);
            gtk_widget_set_sensitive(p->mod_amount[i],
                                        modsrc[i] != MOD_SRC_NONE);
        }
        else if (i < last_slot)
        {
            phin_slider_set_value(PHIN_SLIDER(p->mod_amount[i]),
                                                            modamt[i]);
            gtk_widget_set_sensitive(p->mod_amount[i],
                                        modsrc[i] != MOD_SRC_NONE);
        }
    }

    unblock(p);
}

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


#include <gtk/gtk.h>

#include "phin.h"

#include "envelopetab.h"
#include "gui.h"
#include "idselector.h"
#include "mod_src_gui.h"
#include "mod_src.h"
#include "names.h"
#include "patch_set_and_get.h"


enum
{
    MAXSTEPS = PATCH_MAX_PITCH_STEPS,
};


typedef struct _EnvelopeTabPrivate EnvelopeTabPrivate;

#define ENVELOPE_TAB_GET_PRIVATE(obj)   \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
        ENVELOPE_TAB_TYPE, EnvelopeTabPrivate))


struct _EnvelopeTabPrivate
{
    int patch;
    GtkWidget* idsel;
    GtkWidget* env_check;
    GtkWidget* env_table;
    GtkWidget* delay;
    GtkWidget* attack;
    GtkWidget* hold;
    GtkWidget* decay;
    GtkWidget* sustain;
    GtkWidget* release;
    GtkWidget* key;
/*  GtkWidget* vel; - not well implemented */
    GtkWidget* exp;
};


G_DEFINE_TYPE(EnvelopeTab, envelope_tab, GTK_TYPE_VBOX)

static void update_env(EnvelopeTabPrivate*);


static void envelope_tab_class_init(EnvelopeTabClass* klass)
{
    GtkObjectClass *object_class = GTK_OBJECT_CLASS(klass);
    envelope_tab_parent_class = g_type_class_peek_parent(klass);
    g_type_class_add_private(object_class, sizeof(EnvelopeTabPrivate));
}


static void set_sensitive(EnvelopeTabPrivate* p, gboolean val)
{
    /*  setting the table itself takes care of the labels,
     *  but still need to set the phin widgets...
     */
    gtk_widget_set_sensitive(p->env_table, val);
    gtk_widget_set_sensitive(p->delay, val);
    gtk_widget_set_sensitive(p->attack, val);
    gtk_widget_set_sensitive(p->hold, val);
    gtk_widget_set_sensitive(p->decay, val);
    gtk_widget_set_sensitive(p->sustain, val);
    gtk_widget_set_sensitive(p->release, val);
    gtk_widget_set_sensitive(p->key, val);
/*  gtk_widget_set_sensitive(p->vel, val); */
    gtk_widget_set_sensitive(p->exp, val);
}


static void id_selector_cb(IDSelector* ids, EnvelopeTabPrivate* p)
{
    (void)ids;
    update_env(p);
    set_sensitive(p,
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->env_check)));
}


static void on_cb(GtkToggleButton* button, EnvelopeTabPrivate* p)
{
    patch_set_env_active(p->patch,
            id_selector_get_id(ID_SELECTOR(p->idsel)),
                    gtk_toggle_button_get_active(button));
}


static void on_cb2(GtkToggleButton* button, EnvelopeTabPrivate* p)
{
    set_sensitive(p, gtk_toggle_button_get_active(button));
}


static void delay_cb(PhinSlider* sl, EnvelopeTabPrivate* p)
{
    float val = phin_slider_get_value(sl);
    patch_set_env_delay(p->patch,
        id_selector_get_id(ID_SELECTOR(p->idsel)),val);
}


static void attack_cb(PhinSlider* sl, EnvelopeTabPrivate* p)
{
    float val = phin_slider_get_value(sl);
    patch_set_env_attack(p->patch,
        id_selector_get_id(ID_SELECTOR(p->idsel)), val);
}


static void hold_cb(PhinSlider* sl, EnvelopeTabPrivate* p)
{
    float val = phin_slider_get_value(sl);
    patch_set_env_hold(p->patch,
        id_selector_get_id(ID_SELECTOR(p->idsel)), val);
}


static void decay_cb(PhinSlider* sl, EnvelopeTabPrivate* p)
{
    float val = phin_slider_get_value(sl);
    patch_set_env_decay(p->patch,
        id_selector_get_id(ID_SELECTOR(p->idsel)), val);
}


static void sustain_cb(PhinSlider* sl, EnvelopeTabPrivate* p)
{
    float val = phin_slider_get_value(sl);
    patch_set_env_sustain(p->patch,
        id_selector_get_id(ID_SELECTOR(p->idsel)), val);
}


static void release_cb(PhinSlider* sl, EnvelopeTabPrivate* p)
{
    float val = phin_slider_get_value(sl);
    patch_set_env_release(p->patch,
        id_selector_get_id(ID_SELECTOR(p->idsel)), val);
}


static void key_cb(PhinSlider* sl, EnvelopeTabPrivate* p)
{
    float val = phin_slider_get_value(sl);
    patch_set_env_key_amt(p->patch,
        id_selector_get_id(ID_SELECTOR(p->idsel)), val);
}

/*
static void vel_cb(PhinSlider* sl, EnvelopeTabPrivate* p)
{
    float val = phin_slider_get_value(sl);
    patch_set_env_vel_amt(p->patch,
        id_selector_get_id(ID_SELECTOR(p->idsel)), val);
}
*/

static void exp_cb(GtkToggleButton* button, EnvelopeTabPrivate* p)
{
    patch_set_env_exp(p->patch,
        id_selector_get_id(ID_SELECTOR(p->idsel)),
                            gtk_toggle_button_get_active(button));
}


static void connect(EnvelopeTabPrivate* p)
{
    g_signal_connect(G_OBJECT(p->idsel), "changed",
                    G_CALLBACK(id_selector_cb), (gpointer)p);
    g_signal_connect(G_OBJECT(p->env_check), "toggled",
                    G_CALLBACK(on_cb), (gpointer)p);
    g_signal_connect(G_OBJECT(p->env_check), "toggled",
                    G_CALLBACK(on_cb2), (gpointer)p);
    g_signal_connect(G_OBJECT(p->delay), "value-changed",
                    G_CALLBACK(delay_cb), (gpointer) p);
    g_signal_connect(G_OBJECT(p->attack), "value-changed",
                    G_CALLBACK(attack_cb), (gpointer) p);
    g_signal_connect(G_OBJECT(p->hold), "value-changed",
                    G_CALLBACK(hold_cb), (gpointer) p);
    g_signal_connect(G_OBJECT(p->decay), "value-changed",
                    G_CALLBACK(decay_cb), (gpointer) p);
    g_signal_connect(G_OBJECT(p->sustain), "value-changed",
                    G_CALLBACK(sustain_cb), (gpointer) p);
    g_signal_connect(G_OBJECT(p->release), "value-changed",
                    G_CALLBACK(release_cb), (gpointer) p);
    g_signal_connect(G_OBJECT(p->key), "value-changed",
                    G_CALLBACK(key_cb), (gpointer) p);
/*  g_signal_connect(G_OBJECT(p->vel), "value-changed",
                    G_CALLBACK(vel_cb), (gpointer) p); */
    g_signal_connect(G_OBJECT(p->exp), "toggled",
                    G_CALLBACK(exp_cb), (gpointer)p);
}


static void block(EnvelopeTabPrivate* p)
{
    g_signal_handlers_block_by_func(p->env_check,   on_cb,      p);
    g_signal_handlers_block_by_func(p->delay,   delay_cb,   p);
    g_signal_handlers_block_by_func(p->attack,  attack_cb,  p);
    g_signal_handlers_block_by_func(p->hold,    hold_cb,    p);
    g_signal_handlers_block_by_func(p->decay,   decay_cb,   p);
    g_signal_handlers_block_by_func(p->sustain, sustain_cb, p);
    g_signal_handlers_block_by_func(p->release, release_cb, p);
    g_signal_handlers_block_by_func(p->key,     key_cb,     p);
/*  g_signal_handlers_block_by_func(p->vel,     vel_cb,     p); */
    g_signal_handlers_block_by_func(p->exp,     exp_cb,     p);
}


static void unblock(EnvelopeTabPrivate* p)
{
    g_signal_handlers_unblock_by_func(p->env_check,     on_cb,      p);
    g_signal_handlers_unblock_by_func(p->delay,     delay_cb,   p);
    g_signal_handlers_unblock_by_func(p->attack,    attack_cb,  p);
    g_signal_handlers_unblock_by_func(p->hold,      hold_cb,    p);
    g_signal_handlers_unblock_by_func(p->decay,     decay_cb,   p);
    g_signal_handlers_unblock_by_func(p->sustain,   sustain_cb, p);
    g_signal_handlers_unblock_by_func(p->release,   release_cb, p);
    g_signal_handlers_unblock_by_func(p->key,       key_cb,     p);
/*  g_signal_handlers_unblock_by_func(p->vel,       vel_cb,     p); */
    g_signal_handlers_unblock_by_func(p->exp,       exp_cb,     p);
}


static void envelope_tab_init(EnvelopeTab* self)
{
    EnvelopeTabPrivate* p = ENVELOPE_TAB_GET_PRIVATE(self);
    GtkBox* box = GTK_BOX(self);
    GtkWidget* title;
    GtkTable* t;

    id_name* egs;

    int y = 0;
    int a1 = 0, a2 = 1;
    int b1 = 1, b2 = 4;
    int cols = 5;

    p->patch = -1;
    gtk_container_set_border_width(GTK_CONTAINER(self), GUI_BORDERSPACE);

    /* parameter selector */
    p->idsel = id_selector_new();
    egs = mod_src_get(MOD_SRC_EG);

    id_selector_set(ID_SELECTOR(p->idsel),
                    egs,
                    ID_SELECTOR_H);

    mod_src_free(egs);

    gui_pack(box, p->idsel);

    /* selector padding */
    gui_pack(box, gui_vpad_new(GUI_SPACING));

    title = gui_title_new("Envelope Generator");
    p->env_check = gtk_check_button_new();
    gtk_container_add(GTK_CONTAINER(p->env_check), title);
    gui_pack(box, p->env_check);
    gtk_widget_show(title); /* title requires additional showing */

    /* table */
    p->env_table = gtk_table_new(9, cols, FALSE);
    t = (GtkTable*) p->env_table;
    gui_pack(box, p->env_table);
/*    gtk_table_set_row_spacings(t, 2);*/


    /* delay */
    gui_label_attach("Delay:", t, a1, a2, y, y + 1);
    p->delay = phin_hslider_new_with_range(0.1, 0.0, 10.0, 0.01);
    gui_attach(t, p->delay, b1, b2, y, y + 1);
    ++y;

    /* attack */
    gui_label_attach("Attack:", t, a1, a2, y, y + 1);
    p->attack = phin_hslider_new_with_range(0.1, 0.0, 15.0, 0.01);
    gui_attach(t, p->attack, b1, b2, y, y + 1);
    ++y;

    /* hold */
    gui_label_attach("Hold:", t, a1, a2, y, y + 1);
    p->hold = phin_hslider_new_with_range(0.1, 0.0, 15.0, 0.01);
    gui_attach(t, p->hold, b1, b2, y, y + 1);
    ++y;

    /* decay */
    gui_label_attach("Decay:", t, a1, a2, y, y + 1);
    p->decay = phin_hslider_new_with_range(0.1, 0.0, 25.0, 0.01);
    gui_attach(t, p->decay, b1, b2, y, y + 1);
    ++y;

    /* sustain */
    gui_label_attach("Sustain:", t, a1, a2, y, y + 1);
    p->sustain = phin_hslider_new_with_range(0.7, 0.0, 1.0, 0.01);
    gui_attach(t, p->sustain, b1, b2, y, y + 1);
    ++y;

    /* release */
    gui_label_attach("Release:", t, a1, a2, y, y  + 1);
    p->release = phin_hslider_new_with_range(0.1, 0.0, 25.0, 0.01);
    gui_attach(t, p->release, b1, b2, y, y + 1);
    ++y;

    /* key */
    gui_label_attach("Key Track:", t, a1, a2, y, y + 1);
    p->key = phin_hslider_new_with_range(0.1, -1.0, 1.0, 0.01);
    gui_attach(t, p->key, b1, b2, y, y + 1);
    ++y;

    /* vel
    gui_label_attach("Vel.Sens:", t, a1, a2, y, y + 1);
    p->vel = phin_hslider_new_with_range(0.1, -1.0, 1.0, 0.01);
    gui_attach(t, p->vel, b1, b2, y, y + 1);
    ++y;
    */

    /* exp/lin */
    gui_label_attach("Exponential:", t, a1, a2, y, y + 1);
    p->exp = gtk_check_button_new();
    gui_attach(t, p->exp, b1, b2, y, y + 1);
    ++y;

    set_sensitive(p, FALSE);
    connect(p);
}


static void update_env(EnvelopeTabPrivate* p)
{
    int i = p->patch;
    float l, a, h, d, s, r, key;
    bool on, exp;

    int id;

    id = id_selector_get_id(ID_SELECTOR(p->idsel));

    l = patch_get_env_delay(i, id);
    a = patch_get_env_attack(i, id);
    h = patch_get_env_hold(i, id);
    d = patch_get_env_decay(i, id);
    s = patch_get_env_sustain(i, id);
    r = patch_get_env_release(i, id);
    on = patch_get_env_active(i, id);
    key = patch_get_env_key_amt(i, id);
/*  patch_get_env_vel_amt(i, id, &vel); */
    exp = patch_get_env_exp(i, id);

    block(p);

    phin_slider_set_value(PHIN_SLIDER(p->delay), l);
    phin_slider_set_value(PHIN_SLIDER(p->attack), a);
    phin_slider_set_value(PHIN_SLIDER(p->hold), h);
    phin_slider_set_value(PHIN_SLIDER(p->decay), d);
    phin_slider_set_value(PHIN_SLIDER(p->sustain), s);
    phin_slider_set_value(PHIN_SLIDER(p->release), r);
    phin_slider_set_value(PHIN_SLIDER(p->key), key);
/*  phin_slider_set_value(PHIN_SLIDER(p->vel), vel); */

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->exp), exp);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->env_check), on);

    unblock(p);
}


GtkWidget* envelope_tab_new(void)
{
    return (GtkWidget*) g_object_new(ENVELOPE_TAB_TYPE, NULL);
}


void envelope_tab_set_patch(EnvelopeTab* self, int patch)
{
    EnvelopeTabPrivate* p = ENVELOPE_TAB_GET_PRIVATE(self);
    p->patch = patch;

    if (patch < 0)
        return;

    update_env(p);
}

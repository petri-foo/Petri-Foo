#include <gtk/gtk.h>
#include <phat/phat.h>
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
    GtkWidget* delay_fan;
    GtkWidget* attack_fan;
    GtkWidget* hold_fan;
    GtkWidget* decay_fan;
    GtkWidget* sustain_fan;
    GtkWidget* release_fan;
    GtkWidget* key_fan;
/*  GtkWidget* vel_fan; - not well implemented */
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
     *  but still need to set the phat widgets...
     */
    gtk_widget_set_sensitive(p->env_table, val);
    gtk_widget_set_sensitive(p->delay_fan, val);
    gtk_widget_set_sensitive(p->attack_fan, val);
    gtk_widget_set_sensitive(p->hold_fan, val);
    gtk_widget_set_sensitive(p->decay_fan, val);
    gtk_widget_set_sensitive(p->sustain_fan, val);
    gtk_widget_set_sensitive(p->release_fan, val);
    gtk_widget_set_sensitive(p->key_fan, val);
/*  gtk_widget_set_sensitive(p->vel_fan, val); */
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
    patch_set_env_on(p->patch,
            id_selector_get_id(ID_SELECTOR(p->idsel)),
                    gtk_toggle_button_get_active(button));
}


static void on_cb2(GtkToggleButton* button, EnvelopeTabPrivate* p)
{
    set_sensitive(p, gtk_toggle_button_get_active(button));
}


static void delay_cb(PhatFanSlider* fan, EnvelopeTabPrivate* p)
{
    float val = phat_fan_slider_get_value(fan);
    patch_set_env_delay(p->patch,
        id_selector_get_id(ID_SELECTOR(p->idsel)),val);
}


static void attack_cb(PhatFanSlider* fan, EnvelopeTabPrivate* p)
{
    float val = phat_fan_slider_get_value(fan);
    patch_set_env_attack(p->patch,
        id_selector_get_id(ID_SELECTOR(p->idsel)), val);
}


static void hold_cb(PhatFanSlider* fan, EnvelopeTabPrivate* p)
{
    float val = phat_fan_slider_get_value(fan);
    patch_set_env_hold(p->patch,
        id_selector_get_id(ID_SELECTOR(p->idsel)), val);
}


static void decay_cb(PhatFanSlider* fan, EnvelopeTabPrivate* p)
{
    float val = phat_fan_slider_get_value(fan);
    patch_set_env_decay(p->patch,
        id_selector_get_id(ID_SELECTOR(p->idsel)), val);
}


static void sustain_cb(PhatFanSlider* fan, EnvelopeTabPrivate* p)
{
    float val = phat_fan_slider_get_value(fan);
    patch_set_env_sustain(p->patch,
        id_selector_get_id(ID_SELECTOR(p->idsel)), val);
}


static void release_cb(PhatFanSlider* fan, EnvelopeTabPrivate* p)
{
    float val = phat_fan_slider_get_value(fan);
    patch_set_env_release(p->patch,
        id_selector_get_id(ID_SELECTOR(p->idsel)), val);
}


static void key_cb(PhatFanSlider* fan, EnvelopeTabPrivate* p)
{
    float val = phat_fan_slider_get_value(fan);
    patch_set_env_key_amt(p->patch,
        id_selector_get_id(ID_SELECTOR(p->idsel)), val);
}

/*
static void vel_cb(PhatFanSlider* fan, EnvelopeTabPrivate* p)
{
    float val = phat_fan_slider_get_value(fan);
    patch_set_env_vel_amt(p->patch,
        id_selector_get_id(ID_SELECTOR(p->idsel)), val);
}
*/

static void connect(EnvelopeTabPrivate* p)
{
    g_signal_connect(G_OBJECT(p->idsel), "changed",
                    G_CALLBACK(id_selector_cb), (gpointer)p);
    g_signal_connect(G_OBJECT(p->env_check), "toggled",
                    G_CALLBACK(on_cb), (gpointer)p);
    g_signal_connect(G_OBJECT(p->env_check), "toggled",
                    G_CALLBACK(on_cb2), (gpointer)p);
    g_signal_connect(G_OBJECT(p->delay_fan), "value-changed",
                    G_CALLBACK(delay_cb), (gpointer) p);
    g_signal_connect(G_OBJECT(p->attack_fan), "value-changed",
                    G_CALLBACK(attack_cb), (gpointer) p);
    g_signal_connect(G_OBJECT(p->hold_fan), "value-changed",
                    G_CALLBACK(hold_cb), (gpointer) p);
    g_signal_connect(G_OBJECT(p->decay_fan), "value-changed",
                    G_CALLBACK(decay_cb), (gpointer) p);
    g_signal_connect(G_OBJECT(p->sustain_fan), "value-changed",
                    G_CALLBACK(sustain_cb), (gpointer) p);
    g_signal_connect(G_OBJECT(p->release_fan), "value-changed",
                    G_CALLBACK(release_cb), (gpointer) p);
    g_signal_connect(G_OBJECT(p->key_fan), "value-changed",
                    G_CALLBACK(key_cb), (gpointer) p);
/*  g_signal_connect(G_OBJECT(p->vel_fan), "value-changed",
                    G_CALLBACK(vel_cb), (gpointer) p); */
}


static void block(EnvelopeTabPrivate* p)
{
    g_signal_handlers_block_by_func(p->env_check,   on_cb,      p);
    g_signal_handlers_block_by_func(p->delay_fan,   delay_cb,   p);
    g_signal_handlers_block_by_func(p->attack_fan,  attack_cb,  p);
    g_signal_handlers_block_by_func(p->hold_fan,    hold_cb,    p);
    g_signal_handlers_block_by_func(p->decay_fan,   decay_cb,   p);
    g_signal_handlers_block_by_func(p->sustain_fan, sustain_cb, p);
    g_signal_handlers_block_by_func(p->release_fan, release_cb, p);
    g_signal_handlers_block_by_func(p->key_fan,     key_cb,     p);
/*  g_signal_handlers_block_by_func(p->vel_fan,     vel_cb,     p); */
}


static void unblock(EnvelopeTabPrivate* p)
{
    g_signal_handlers_unblock_by_func(p->env_check,     on_cb,      p);
    g_signal_handlers_unblock_by_func(p->delay_fan,     delay_cb,   p);
    g_signal_handlers_unblock_by_func(p->attack_fan,    attack_cb,  p);
    g_signal_handlers_unblock_by_func(p->hold_fan,      hold_cb,    p);
    g_signal_handlers_unblock_by_func(p->decay_fan,     decay_cb,   p);
    g_signal_handlers_unblock_by_func(p->sustain_fan,   sustain_cb, p);
    g_signal_handlers_unblock_by_func(p->release_fan,   release_cb, p);
    g_signal_handlers_unblock_by_func(p->key_fan,       key_cb,     p);
/*  g_signal_handlers_unblock_by_func(p->vel_fan,       vel_cb,     p); */
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
    int b1 = 1, b2 = 2;

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
    p->env_table = gtk_table_new(9, 3, FALSE);
    t = (GtkTable*) p->env_table;
    gui_pack(box, p->env_table);

    /* delay fan */
    gui_label_attach("Delay:", t, a1, a2, y, y + 1);
    p->delay_fan = phat_hfan_slider_new_with_range(0.1, 0.0, 1.0, 0.01);
    gui_attach(t, p->delay_fan, b1, b2, y, y + 1);
    ++y;

    /* attack fan */
    gui_label_attach("Attack:", t, a1, a2, y, y + 1);
    p->attack_fan = phat_hfan_slider_new_with_range(0.1, 0.0, 1.0, 0.01);
    gui_attach(t, p->attack_fan, b1, b2, y, y + 1);
    ++y;

    /* hold fan */
    gui_label_attach("Hold:", t, a1, a2, y, y + 1);
    p->hold_fan = phat_hfan_slider_new_with_range(0.1, 0.0, 1.0, 0.01);
    gui_attach(t, p->hold_fan, b1, b2, y, y + 1);
    ++y;

    /* decay fan */
    gui_label_attach("Decay:", t, a1, a2, y, y + 1);
    p->decay_fan = phat_hfan_slider_new_with_range(0.1, 0.0, 1.0, 0.01);
    gui_attach(t, p->decay_fan, b1, b2, y, y + 1);
    ++y;

    /* sustain fan */
    gui_label_attach("Sustain:", t, a1, a2, y, y + 1);
    p->sustain_fan = phat_hfan_slider_new_with_range(0.7, 0.0, 1.0, 0.01);
    gui_attach(t, p->sustain_fan, b1, b2, y, y + 1);
    ++y;

    /* release fan */
    gui_label_attach("Release:", t, a1, a2, y, y  + 1);
    p->release_fan = phat_hfan_slider_new_with_range(0.1, 0.0, 1.0, 0.01);
    gui_attach(t, p->release_fan, b1, b2, y, y + 1);
    ++y;

    /* key fan */
    gui_label_attach("Key Track:", t, a1, a2, y, y + 1);
    p->key_fan = phat_hfan_slider_new_with_range(0.1, -1.0, 1.0, 0.01);
    gui_attach(t, p->key_fan, b1, b2, y, y + 1);
    ++y;

    /* vel fan
    gui_label_attach("Vel.Sens:", t, a1, a2, y, y + 1);
    p->vel_fan = phat_hfan_slider_new_with_range(0.1, -1.0, 1.0, 0.01);
    gui_attach(t, p->vel_fan, b1, b2, y, y + 1);
    ++y;
    */

    set_sensitive(p, FALSE);
    connect(p);
}


static void update_env(EnvelopeTabPrivate* p)
{
    int i = p->patch;
    float l, a, h, d, s, r, key, vel;
    bool on;

    int id;

    id = id_selector_get_id(ID_SELECTOR(p->idsel));

    patch_get_env_delay(i, id, &l);
    patch_get_env_attack(i, id, &a);
    patch_get_env_hold(i, id, &h);
    patch_get_env_decay(i, id, &d);
    patch_get_env_sustain(i, id, &s);
    patch_get_env_release(i, id, &r);
    patch_get_env_on(i, id, &on);
    patch_get_env_key_amt(i, id, &key);
/*  patch_get_env_vel_amt(i, id, &vel); */

    block(p);

    phat_fan_slider_set_value(PHAT_FAN_SLIDER(p->delay_fan), l);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(p->attack_fan), a);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(p->hold_fan), h);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(p->decay_fan), d);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(p->sustain_fan), s);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(p->release_fan), r);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(p->key_fan), key);
/*  phat_fan_slider_set_value(PHAT_FAN_SLIDER(p->vel_fan), vel); */

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

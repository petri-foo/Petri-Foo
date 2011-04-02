#include <gtk/gtk.h>
#include <phat/phat.h>
#include "envelopetab.h"
#include "gui.h"
#include "idselector.h"
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
    GtkWidget* delay_fan;
    GtkWidget* attack_fan;
    GtkWidget* hold_fan;
    GtkWidget* decay_fan;
    GtkWidget* sustain_fan;
    GtkWidget* release_fan;
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
    gtk_widget_set_sensitive(p->delay_fan, val);
    gtk_widget_set_sensitive(p->attack_fan, val);
    gtk_widget_set_sensitive(p->hold_fan, val);
    gtk_widget_set_sensitive(p->decay_fan, val);
    gtk_widget_set_sensitive(p->sustain_fan, val);
    gtk_widget_set_sensitive(p->release_fan, val);
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
}


static void envelope_tab_init(EnvelopeTab* self)
{
    EnvelopeTabPrivate* p = ENVELOPE_TAB_GET_PRIVATE(self);
    GtkBox* box = GTK_BOX(self);
    GtkWidget* title;
    GtkWidget* table;
    GtkTable* t;
    GtkWidget* pad;
    GtkWidget* label;

    p->patch = -1;
    gtk_container_set_border_width(GTK_CONTAINER(self), GUI_BORDERSPACE);

    /* parameter selector */
    p->idsel = id_selector_new();
    id_selector_set(ID_SELECTOR(p->idsel), names_egs_get(), ID_SELECTOR_H);
    gtk_box_pack_start(box, p->idsel, FALSE, FALSE, 0);
    gtk_widget_show(p->idsel);

    /* selector padding */
    pad = gui_vpad_new(GUI_SPACING);
    gtk_box_pack_start(box, pad, FALSE, FALSE, 0);
    gtk_widget_show(pad);

    /* table */
    table = gtk_table_new(9, 4, FALSE);
    t = (GtkTable*) table;
    gtk_box_pack_start(box, table, FALSE, FALSE, 0);
    gtk_widget_show(table);

    /* envelope title  */
    title = gui_title_new("Envelope Generator");
    p->env_check = gtk_check_button_new();
    gtk_container_add(GTK_CONTAINER(p->env_check), title);
    gtk_table_attach_defaults(t, p->env_check, 0, 4, 0, 1);
    gtk_widget_show(title);
    gtk_widget_show(p->env_check);

    /* indentation */
    pad = gui_hpad_new(GUI_INDENT);
    gtk_table_attach(t, pad, 0, 1, 1, 2, 0, 0, 0, 0);
    gtk_widget_show(pad);

    /* envelope title padding */
    pad = gui_vpad_new(GUI_TITLESPACE);
    gtk_table_attach(t, pad, 1, 2, 1, 2, 0, 0, 0, 0);
    gtk_widget_show(pad);

    /* label-fan column spacing */
    pad = gui_hpad_new(GUI_TEXTSPACE);
    gtk_table_attach(t, pad, 2, 3, 1, 2, 0, 0, 0, 0);
    gtk_widget_show(pad);

    /* delay fan */
    label = gtk_label_new("Delay:");
    p->delay_fan = phat_hfan_slider_new_with_range(0.1, 0.0, 1.0, 0.01);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, 2, 3, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, p->delay_fan, 3, 4, 2, 3);
    gtk_widget_show(label);
    gtk_widget_show(p->delay_fan);

    /* attack fan */
    label = gtk_label_new("Attack:");
    p->attack_fan = phat_hfan_slider_new_with_range(0.1, 0.0, 1.0, 0.01);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, 3, 4, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, p->attack_fan, 3, 4, 3, 4);
    gtk_widget_show(label);
    gtk_widget_show(p->attack_fan);

    /* hold fan */
    label = gtk_label_new("Hold:");
    p->hold_fan = phat_hfan_slider_new_with_range(0.1, 0.0, 1.0, 0.01);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, 4, 5, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, p->hold_fan, 3, 4, 4, 5);
    gtk_widget_show(label);
    gtk_widget_show(p->hold_fan);

    /* decay fan */
    label = gtk_label_new("Decay:");
    p->decay_fan = phat_hfan_slider_new_with_range(0.1, 0.0, 1.0, 0.01);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, 5, 6, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, p->decay_fan, 3, 4, 5, 6);
    gtk_widget_show(label);
    gtk_widget_show(p->decay_fan);

    /* sustain fan */
    label = gtk_label_new("Sustain:");
    p->sustain_fan = phat_hfan_slider_new_with_range(0.7, 0.0, 1.0, 0.01);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, 6, 7, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, p->sustain_fan, 3, 4, 6, 7);
    gtk_widget_show(label);
    gtk_widget_show(p->sustain_fan);

    /* release fan */
    label = gtk_label_new("Release:");
    p->release_fan = phat_hfan_slider_new_with_range(0.1, 0.0, 1.0, 0.01);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, 7, 8, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, p->release_fan, 3, 4, 7, 8);
    gtk_widget_show(label);
    gtk_widget_show(p->release_fan);

    gtk_table_set_row_spacing(t, 7, GUI_SPACING);

    set_sensitive(p, FALSE);
    connect(p);
}


static void update_env(EnvelopeTabPrivate* p)
{
    int i = p->patch;
    float l, a, h, d, s, r;
    gboolean on;

    int id;

    id = id_selector_get_id(ID_SELECTOR(p->idsel));

    patch_get_env_delay(i, id, &l);
    patch_get_env_attack(i, id, &a);
    patch_get_env_hold(i, id, &h);
    patch_get_env_decay(i, id, &d);
    patch_get_env_sustain(i, id, &s);
    patch_get_env_release(i, id, &r);
    patch_get_env_on(i, id, &on);

    block(p);

    phat_fan_slider_set_value(PHAT_FAN_SLIDER(p->delay_fan), l);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(p->attack_fan), a);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(p->hold_fan), h);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(p->decay_fan), d);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(p->sustain_fan), s);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(p->release_fan), r);

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

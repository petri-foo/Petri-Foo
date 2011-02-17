#include <gtk/gtk.h>
#include <phat/phat.h>
#include "envelopetab.h"
#include "gui.h"
#include "idselector.h"
#include "mod_src.h"
#include "patch_util.h"

enum
{
    MAXSTEPS = PATCH_MAX_PITCH_STEPS,
};


static GtkVBoxClass* parent_class;

static void envelope_tab_class_init(EnvelopeTabClass* klass);
static void envelope_tab_init(EnvelopeTab* self);
static void update_env(EnvelopeTab* self);


GType envelope_tab_get_type(void)
{
    static GType type = 0;

    if (!type)
    {
	static const GTypeInfo info =
	    {
		sizeof (EnvelopeTabClass),
		NULL,
		NULL,
		(GClassInitFunc) envelope_tab_class_init,
		NULL,
		NULL,
		sizeof (EnvelopeTab),
		0,
		(GInstanceInitFunc) envelope_tab_init,
	    };

	type = g_type_register_static(GTK_TYPE_VBOX, "EnvelopeTab", &info, 0);
    }

    return type;
}


static void envelope_tab_class_init(EnvelopeTabClass* klass)
{
    parent_class = g_type_class_peek_parent(klass);
}


static void set_sensitive(EnvelopeTab* self, gboolean val)
{
    gtk_widget_set_sensitive(self->delay_fan, val);
    gtk_widget_set_sensitive(self->attack_fan, val);
    gtk_widget_set_sensitive(self->hold_fan, val);
    gtk_widget_set_sensitive(self->decay_fan, val);
    gtk_widget_set_sensitive(self->sustain_fan, val);
    gtk_widget_set_sensitive(self->release_fan, val);
}


static void id_selector_cb(IDSelector* ids, EnvelopeTab* self)
{
    update_env(self);

    set_sensitive(self,
            gtk_toggle_button_get_active(
                    GTK_TOGGLE_BUTTON(self->env_check)));
}


static void on_cb(GtkToggleButton* button, EnvelopeTab* self)
{
    patch_set_env_on(self->patch,
            id_selector_get_id(ID_SELECTOR(self->idsel)),
                    gtk_toggle_button_get_active(button));
}


static void on_cb2(GtkToggleButton* button, EnvelopeTab* self)
{
    set_sensitive(self, gtk_toggle_button_get_active(button));
}


static void delay_cb(PhatFanSlider* fan, EnvelopeTab* self)
{
    float val = phat_fan_slider_get_value(fan);
    patch_set_env_delay(self->patch,
        id_selector_get_id(ID_SELECTOR(self->idsel)),val);
}


static void attack_cb(PhatFanSlider* fan, EnvelopeTab* self)
{
    float val = phat_fan_slider_get_value(fan);
    patch_set_env_attack(self->patch,
        id_selector_get_id(ID_SELECTOR(self->idsel)), val);
}


static void hold_cb(PhatFanSlider* fan, EnvelopeTab* self)
{
    float val = phat_fan_slider_get_value(fan);
    patch_set_env_hold(self->patch,
        id_selector_get_id(ID_SELECTOR(self->idsel)), val);
}


static void decay_cb(PhatFanSlider* fan, EnvelopeTab* self)
{
    float val = phat_fan_slider_get_value(fan);
    patch_set_env_decay(self->patch,
        id_selector_get_id(ID_SELECTOR(self->idsel)), val);
}


static void sustain_cb(PhatFanSlider* fan, EnvelopeTab* self)
{
    float val = phat_fan_slider_get_value(fan);
    patch_set_env_sustain(self->patch,
        id_selector_get_id(ID_SELECTOR(self->idsel)), val);
}


static void release_cb(PhatFanSlider* fan, EnvelopeTab* self)
{
    float val = phat_fan_slider_get_value(fan);
    patch_set_env_release(self->patch,
        id_selector_get_id(ID_SELECTOR(self->idsel)), val);
}


static void connect(EnvelopeTab* self)
{
    g_signal_connect(G_OBJECT(self->idsel), "changed",
		     G_CALLBACK(id_selector_cb), (gpointer)self);
    g_signal_connect(G_OBJECT(self->env_check), "toggled",
		     G_CALLBACK(on_cb), (gpointer)self);
    g_signal_connect(G_OBJECT(self->env_check), "toggled",
		     G_CALLBACK(on_cb2), (gpointer)self);
    g_signal_connect(G_OBJECT(self->delay_fan), "value-changed",
		     G_CALLBACK(delay_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->attack_fan), "value-changed",
		     G_CALLBACK(attack_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->hold_fan), "value-changed",
		     G_CALLBACK(hold_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->decay_fan), "value-changed",
		     G_CALLBACK(decay_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->sustain_fan), "value-changed",
		     G_CALLBACK(sustain_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->release_fan), "value-changed",
		     G_CALLBACK(release_cb), (gpointer) self);
}


static void block(EnvelopeTab* self)
{
    g_signal_handlers_block_by_func(self->env_check, on_cb, self);
    g_signal_handlers_block_by_func(self->delay_fan, delay_cb, self);
    g_signal_handlers_block_by_func(self->attack_fan, attack_cb, self);
    g_signal_handlers_block_by_func(self->hold_fan, hold_cb, self);
    g_signal_handlers_block_by_func(self->decay_fan, decay_cb, self);
    g_signal_handlers_block_by_func(self->sustain_fan, sustain_cb, self);
    g_signal_handlers_block_by_func(self->release_fan, release_cb, self);
}


static void unblock(EnvelopeTab* self)
{
    g_signal_handlers_unblock_by_func(self->env_check, on_cb, self);
    g_signal_handlers_unblock_by_func(self->delay_fan, delay_cb, self);
    g_signal_handlers_unblock_by_func(self->attack_fan, attack_cb, self);
    g_signal_handlers_unblock_by_func(self->hold_fan, hold_cb, self);
    g_signal_handlers_unblock_by_func(self->decay_fan, decay_cb, self);
    g_signal_handlers_unblock_by_func(self->sustain_fan, sustain_cb, self);
    g_signal_handlers_unblock_by_func(self->release_fan, release_cb, self);
}


static void envelope_tab_init(EnvelopeTab* self)
{
    GtkBox* box = GTK_BOX(self);
    GtkWidget* title;
    GtkWidget* table;
    GtkTable* t;
    GtkWidget* pad;
    GtkWidget* label;
    
    self->patch = -1;
    gtk_container_set_border_width(GTK_CONTAINER(self), GUI_BORDERSPACE);

    /* parameter selector */
    self->idsel = id_selector_new();
    id_selector_set(ID_SELECTOR(self->idsel), mod_src_adsr_names(),
                                                ID_SELECTOR_H);
    gtk_box_pack_start(box, self->idsel, FALSE, FALSE, 0);
    gtk_widget_show(self->idsel);

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
    self->env_check = gtk_check_button_new();
    gtk_container_add(GTK_CONTAINER(self->env_check), title);
    gtk_table_attach_defaults(t, self->env_check, 0, 4, 0, 1);
    gtk_widget_show(title);
    gtk_widget_show(self->env_check);

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
    self->delay_fan = phat_hfan_slider_new_with_range(0.1, 0.0, 1.0, 0.01);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, 2, 3, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, self->delay_fan, 3, 4, 2, 3);
    gtk_widget_show(label);
    gtk_widget_show(self->delay_fan);

    /* attack fan */
    label = gtk_label_new("Attack:");
    self->attack_fan = phat_hfan_slider_new_with_range(0.1, 0.0, 1.0, 0.01);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, 3, 4, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, self->attack_fan, 3, 4, 3, 4);
    gtk_widget_show(label);
    gtk_widget_show(self->attack_fan);

    /* hold fan */
    label = gtk_label_new("Hold:");
    self->hold_fan = phat_hfan_slider_new_with_range(0.1, 0.0, 1.0, 0.01);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, 4, 5, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, self->hold_fan, 3, 4, 4, 5);
    gtk_widget_show(label);
    gtk_widget_show(self->hold_fan);

    /* decay fan */
    label = gtk_label_new("Decay:");
    self->decay_fan = phat_hfan_slider_new_with_range(0.1, 0.0, 1.0, 0.01);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, 5, 6, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, self->decay_fan, 3, 4, 5, 6);
    gtk_widget_show(label);
    gtk_widget_show(self->decay_fan);

    /* sustain fan */
    label = gtk_label_new("Sustain:");
    self->sustain_fan = phat_hfan_slider_new_with_range(0.7, 0.0, 1.0, 0.01);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, 6, 7, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, self->sustain_fan, 3, 4, 6, 7);
    gtk_widget_show(label);
    gtk_widget_show(self->sustain_fan);

    /* release fan */
    label = gtk_label_new("Release:");
    self->release_fan = phat_hfan_slider_new_with_range(0.1, 0.0, 1.0, 0.01);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, 7, 8, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, self->release_fan, 3, 4, 7, 8);
    gtk_widget_show(label);
    gtk_widget_show(self->release_fan);

    gtk_table_set_row_spacing(t, 7, GUI_SPACING);

    set_sensitive(self, FALSE);
    connect(self);
}

static void update_env(EnvelopeTab* self)
{
    PatchParamType p;
    int i = self->patch;
    float l, a, h, d, s, r, m;
    gboolean on;

    int id;

    id = id_selector_get_id(ID_SELECTOR(self->idsel));

    patch_get_env_delay(i, id, &l);
    patch_get_env_attack(i, id, &a);
    patch_get_env_hold(i, id, &h);
    patch_get_env_decay(i, id, &d);
    patch_get_env_sustain(i, id, &s);
    patch_get_env_release(i, id, &r);
    patch_get_env_on(i, id, &on);

    block(self);
    
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->delay_fan), l);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->attack_fan), a);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->hold_fan), h);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->decay_fan), d);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->sustain_fan), s);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->release_fan), r);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->env_check), on);

    unblock(self);
}


GtkWidget* envelope_tab_new(void)
{
    return (GtkWidget*) g_object_new(ENVELOPE_TAB_TYPE, NULL);
}


void envelope_tab_set_patch(EnvelopeTab* self, int patch)
{
    self->patch = patch;

    if (patch < 0)
	return;

    update_env(self);
}

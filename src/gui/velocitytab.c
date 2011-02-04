#include <gtk/gtk.h>
#include <phat/phat.h>
#include "velocitytab.h"
#include "gui.h"
#include "patch.h"

static GtkVBoxClass* parent_class;

static void velocity_tab_class_init(VelocityTabClass* klass);
static void velocity_tab_init(VelocityTab* self);


GType velocity_tab_get_type(void)
{
    static GType type = 0;

    if (!type)
    {
	static const GTypeInfo info =
	    {
		sizeof (VelocityTabClass),
		NULL,
		NULL,
		(GClassInitFunc) velocity_tab_class_init,
		NULL,
		NULL,
		sizeof (VelocityTab),
		0,
		(GInstanceInitFunc) velocity_tab_init,
	    };

	type = g_type_register_static(GTK_TYPE_VBOX, "VelocityTab", &info, 0);
    }

    return type;
}


static void velocity_tab_class_init(VelocityTabClass* klass)
{
    parent_class = g_type_class_peek_parent(klass);
}


static void vol_cb(PhatFanSlider* fan, VelocityTab* self)
{
    float val;

    val = phat_fan_slider_get_value(fan);
    patch_set_vel_amount(self->patch, PATCH_PARAM_VOLUME, val);
}


static void pan_cb(PhatFanSlider* fan, VelocityTab* self)
{
    float val;

    val = phat_fan_slider_get_value(fan);
    patch_set_vel_amount(self->patch, PATCH_PARAM_PANNING, val);
}


static void freq_cb(PhatFanSlider* fan, VelocityTab* self)
{
    float val;

    val = phat_fan_slider_get_value(fan);
    patch_set_vel_amount(self->patch, PATCH_PARAM_CUTOFF, val);
}


static void reso_cb(PhatFanSlider* fan, VelocityTab* self)
{
    float val;

    val = phat_fan_slider_get_value(fan);
    patch_set_vel_amount(self->patch, PATCH_PARAM_RESONANCE, val);
}


static void pitch_cb(PhatFanSlider* fan, VelocityTab* self)
{
    float val;

    val = phat_fan_slider_get_value(fan);
    patch_set_vel_amount(self->patch, PATCH_PARAM_PITCH, val);
}


static void connect(VelocityTab* self)
{
    g_signal_connect(G_OBJECT(self->vol_fan), "value-changed",
		     G_CALLBACK(vol_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->pan_fan), "value-changed",
		     G_CALLBACK(pan_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->freq_fan), "value-changed",
		     G_CALLBACK(freq_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->reso_fan), "value-changed",
		     G_CALLBACK(reso_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->pitch_fan), "value-changed",
		     G_CALLBACK(pitch_cb), (gpointer) self);
}


static void block(VelocityTab* self)
{
    g_signal_handlers_block_by_func(self->vol_fan, vol_cb, self);
    g_signal_handlers_block_by_func(self->pan_fan, pan_cb, self);
    g_signal_handlers_block_by_func(self->freq_fan, freq_cb, self);
    g_signal_handlers_block_by_func(self->reso_fan, reso_cb, self);
    g_signal_handlers_block_by_func(self->pitch_fan, pitch_cb, self);
}


static void unblock(VelocityTab* self)
{
    g_signal_handlers_unblock_by_func(self->vol_fan, vol_cb, self);
    g_signal_handlers_unblock_by_func(self->pan_fan, pan_cb, self);
    g_signal_handlers_unblock_by_func(self->freq_fan, freq_cb, self);
    g_signal_handlers_unblock_by_func(self->reso_fan, reso_cb, self);
    g_signal_handlers_unblock_by_func(self->pitch_fan, pitch_cb, self);
}


static void velocity_tab_init(VelocityTab* self)
{
    GtkBox* box = GTK_BOX(self);
    GtkWidget* title;
    GtkWidget* table;
    GtkTable* t;
    GtkWidget* pad;
    GtkWidget* label;
    
    self->patch = -1;
    gtk_container_set_border_width(GTK_CONTAINER(self), GUI_BORDERSPACE);

    /* table */
    table = gtk_table_new(7, 4, FALSE);
    t = (GtkTable*) table;
    gtk_box_pack_start(box, table, FALSE, FALSE, 0);
    gtk_widget_show(table);
    
    /* title */
    title = gui_title_new("Velocity Sensitivity");
    gtk_table_attach_defaults(t, title, 0, 4, 0, 1);
    gtk_widget_show(title);

    /* indentation */
    pad = gui_hpad_new(GUI_INDENT);
    gtk_table_attach(t, pad, 0, 1, 1, 2, 0, 0, 0, 0);
    gtk_widget_show(pad);
    
    /* title padding */
    pad = gui_vpad_new(GUI_TITLESPACE);
    gtk_table_attach(t, pad, 1, 2, 1, 2, 0, 0, 0, 0);
    gtk_widget_show(pad);

    /* label-fan column spacing */
    pad = gui_hpad_new(GUI_TEXTSPACE);
    gtk_table_attach(t, pad, 2, 3, 1, 2, 0, 0, 0, 0);
    gtk_widget_show(pad);

    /* vol fan */
    label = gtk_label_new("Volume:");
    self->vol_fan = phat_hfan_slider_new_with_range(1.0, 0.0, 1.0, 0.01);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, 2, 3, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, self->vol_fan, 3, 4, 2, 3);
    gtk_widget_show(label);
    gtk_widget_show(self->vol_fan);

    /* pan fan */
    label = gtk_label_new("Panning:"); 
    self->pan_fan = phat_hfan_slider_new_with_range(0.0, 0.0, 1.0, 0.01);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, 3, 4, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, self->pan_fan, 3, 4, 3, 4);
    gtk_widget_show(label);
    gtk_widget_show(self->pan_fan);

    /* freq fan */
    label = gtk_label_new("Cutoff:");
    self->freq_fan = phat_hfan_slider_new_with_range(0.0, 0.0, 1.0, 0.01);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, 4, 5, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, self->freq_fan, 3, 4, 4, 5);
    gtk_widget_show(label);
    gtk_widget_show(self->freq_fan);

    /* reso fan */
    label = gtk_label_new("Resonance:");
    self->reso_fan = phat_hfan_slider_new_with_range(0.0, 0.0, 1.0, 0.01);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, 5, 6, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, self->reso_fan, 3, 4, 5, 6);
    gtk_widget_show(label);
    gtk_widget_show(self->reso_fan);

    /* pitch fan */
    label = gtk_label_new("Pitch:");
    self->pitch_fan = phat_hfan_slider_new_with_range(0.0, 0.0, 1.0, 0.01);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, 6, 7, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, self->pitch_fan, 3, 4, 6, 7);
    gtk_widget_show(label);
    gtk_widget_show(self->pitch_fan);

    connect(self);
}


GtkWidget* velocity_tab_new(void)
{
    return (GtkWidget*) g_object_new(VELOCITY_TAB_TYPE, NULL);
}


void velocity_tab_set_patch(VelocityTab* self, int patch)
{
    float vol, pan, freq, reso, pitch;

    self->patch = patch;

    if (patch < 0)
	return;

    patch_get_vel_amount(patch, PATCH_PARAM_VOLUME, &vol);
    patch_get_vel_amount(patch, PATCH_PARAM_PANNING, &pan);
    patch_get_vel_amount(patch, PATCH_PARAM_CUTOFF, &freq);
    patch_get_vel_amount(patch, PATCH_PARAM_RESONANCE, &reso);
    patch_get_vel_amount(patch, PATCH_PARAM_PITCH, &pitch);

    block(self);
    
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->vol_fan), vol);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->pan_fan), pan);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->freq_fan), freq);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->reso_fan), reso);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->pitch_fan), pitch);

    unblock(self);
}

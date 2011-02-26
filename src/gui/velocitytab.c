#include <gtk/gtk.h>
#include <phat/phat.h>
#include "velocitytab.h"
#include "gui.h"
#include "patch_set_and_get.h"


typedef struct _VelocityTabPrivate VelocityTabPrivate;

#define VELOCITY_TAB_GET_PRIVATE(obj)   \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
        VELOCITY_TAB_TYPE, VelocityTabPrivate))

struct _VelocityTabPrivate
{
    int patch;
    GtkWidget* vol_fan;
    GtkWidget* pan_fan;
    GtkWidget* freq_fan;
    GtkWidget* reso_fan;
    GtkWidget* pitch_fan;
};


G_DEFINE_TYPE(VelocityTab, velocity_tab, GTK_TYPE_VBOX);


static void velocity_tab_class_init(VelocityTabClass* klass)
{
    GtkObjectClass *object_class = GTK_OBJECT_CLASS(klass);
    velocity_tab_parent_class = g_type_class_peek_parent(klass);
    g_type_class_add_private(object_class, sizeof(VelocityTabPrivate));
}


static void vol_cb(PhatFanSlider* fan, VelocityTabPrivate* p)
{
    float val = phat_fan_slider_get_value(fan);
    patch_set_vel_amount(p->patch, PATCH_PARAM_AMPLITUDE, val);
}


static void pan_cb(PhatFanSlider* fan, VelocityTabPrivate* p)
{
    float val = phat_fan_slider_get_value(fan);
    patch_set_vel_amount(p->patch, PATCH_PARAM_PANNING, val);
}


static void freq_cb(PhatFanSlider* fan, VelocityTabPrivate* p)
{
    float val = phat_fan_slider_get_value(fan);
    patch_set_vel_amount(p->patch, PATCH_PARAM_CUTOFF, val);
}


static void reso_cb(PhatFanSlider* fan, VelocityTabPrivate* p)
{
    float val = phat_fan_slider_get_value(fan);
    patch_set_vel_amount(p->patch, PATCH_PARAM_RESONANCE, val);
}


static void pitch_cb(PhatFanSlider* fan, VelocityTabPrivate* p)
{
    float val = phat_fan_slider_get_value(fan);
    patch_set_vel_amount(p->patch, PATCH_PARAM_PITCH, val);
}


static void connect(VelocityTabPrivate* p)
{
    g_signal_connect(G_OBJECT(p->vol_fan), "value-changed",
                        G_CALLBACK(vol_cb), (gpointer)p);
    g_signal_connect(G_OBJECT(p->pan_fan), "value-changed",
                        G_CALLBACK(pan_cb), (gpointer)p);
    g_signal_connect(G_OBJECT(p->freq_fan), "value-changed",
                        G_CALLBACK(freq_cb), (gpointer)p);
    g_signal_connect(G_OBJECT(p->reso_fan), "value-changed",
                        G_CALLBACK(reso_cb), (gpointer)p);
    g_signal_connect(G_OBJECT(p->pitch_fan), "value-changed",
                        G_CALLBACK(pitch_cb), (gpointer)p);
}


static void block(VelocityTabPrivate* p)
{
    g_signal_handlers_block_by_func(p->vol_fan, vol_cb, p);
    g_signal_handlers_block_by_func(p->pan_fan, pan_cb, p);
    g_signal_handlers_block_by_func(p->freq_fan, freq_cb, p);
    g_signal_handlers_block_by_func(p->reso_fan, reso_cb, p);
    g_signal_handlers_block_by_func(p->pitch_fan, pitch_cb, p);
}


static void unblock(VelocityTabPrivate* p)
{
    g_signal_handlers_unblock_by_func(p->vol_fan, vol_cb, p);
    g_signal_handlers_unblock_by_func(p->pan_fan, pan_cb, p);
    g_signal_handlers_unblock_by_func(p->freq_fan, freq_cb, p);
    g_signal_handlers_unblock_by_func(p->reso_fan, reso_cb, p);
    g_signal_handlers_unblock_by_func(p->pitch_fan, pitch_cb, p);
}


static void velocity_tab_init(VelocityTab* self)
{
    VelocityTabPrivate* p = VELOCITY_TAB_GET_PRIVATE(self);
    GtkBox* box = GTK_BOX(self);
    GtkWidget* title;
    GtkWidget* table;
    GtkTable* t;
    GtkWidget* pad;
    GtkWidget* label;

    gtk_container_set_border_width(GTK_CONTAINER(self), GUI_BORDERSPACE);

    p->patch = -1;

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
    label = gtk_label_new("Amplitude:");
    p->vol_fan = phat_hfan_slider_new_with_range(1.0, 0.0, 1.0, 0.01);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, 2, 3, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, p->vol_fan, 3, 4, 2, 3);
    gtk_widget_show(label);
    gtk_widget_show(p->vol_fan);

    /* pan fan */
    label = gtk_label_new("Panning:"); 
    p->pan_fan = phat_hfan_slider_new_with_range(0.0, 0.0, 1.0, 0.01);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, 3, 4, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, p->pan_fan, 3, 4, 3, 4);
    gtk_widget_show(label);
    gtk_widget_show(p->pan_fan);

    /* freq fan */
    label = gtk_label_new("Cutoff:");
    p->freq_fan = phat_hfan_slider_new_with_range(0.0, 0.0, 1.0, 0.01);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, 4, 5, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, p->freq_fan, 3, 4, 4, 5);
    gtk_widget_show(label);
    gtk_widget_show(p->freq_fan);

    /* reso fan */
    label = gtk_label_new("Resonance:");
    p->reso_fan = phat_hfan_slider_new_with_range(0.0, 0.0, 1.0, 0.01);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, 5, 6, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, p->reso_fan, 3, 4, 5, 6);
    gtk_widget_show(label);
    gtk_widget_show(p->reso_fan);

    /* pitch fan */
    label = gtk_label_new("Pitch:");
    p->pitch_fan = phat_hfan_slider_new_with_range(0.0, 0.0, 1.0, 0.01);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, 6, 7, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, p->pitch_fan, 3, 4, 6, 7);
    gtk_widget_show(label);
    gtk_widget_show(p->pitch_fan);

    connect(p);
}


GtkWidget* velocity_tab_new(void)
{
    return (GtkWidget*) g_object_new(VELOCITY_TAB_TYPE, NULL);
}


void velocity_tab_set_patch(VelocityTab* self, int patch)
{
    VelocityTabPrivate* p = VELOCITY_TAB_GET_PRIVATE(self);
    float vol, pan, freq, reso, pitch;

    p->patch = patch;

    if (patch < 0)
	return;

    patch_get_vel_amount(patch, PATCH_PARAM_AMPLITUDE,  &vol);
    patch_get_vel_amount(patch, PATCH_PARAM_PANNING,    &pan);
    patch_get_vel_amount(patch, PATCH_PARAM_CUTOFF,     &freq);
    patch_get_vel_amount(patch, PATCH_PARAM_RESONANCE,  &reso);
    patch_get_vel_amount(patch, PATCH_PARAM_PITCH,      &pitch);

    block(p);
    
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(p->vol_fan), vol);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(p->pan_fan), pan);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(p->freq_fan), freq);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(p->reso_fan), reso);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(p->pitch_fan), pitch);

    unblock(p);
}

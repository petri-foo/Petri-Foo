#include <gtk/gtk.h>
#include <phat/phat.h>
#include "filtertab.h"
#include "gui.h"
#include "patch_set_and_get.h"



typedef struct _FilterTabPrivate FilterTabPrivate;

#define FILTER_TAB_GET_PRIVATE(obj)     \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
        FILTER_TAB_TYPE, FilterTabPrivate))


struct _FilterTabPrivate
{
    int         patch;
    guint       refresh;
    GtkWidget*  freq_fan;
    GtkWidget*  reso_fan;
};

G_DEFINE_TYPE(FilterTab, filter_tab, GTK_TYPE_VBOX)


static void filter_tab_class_init(FilterTabClass* klass)
{
    GtkObjectClass *object_class = GTK_OBJECT_CLASS(klass);
    filter_tab_parent_class = g_type_class_peek_parent(klass);
    g_type_class_add_private(object_class, sizeof(FilterTabPrivate));
}


static void freq_cb(PhatFanSlider* fan, FilterTabPrivate* p)
{
    float val;

    val = phat_fan_slider_get_value(fan);
    patch_set_cutoff(p->patch, val);
}


static void reso_cb(PhatFanSlider* fan, FilterTabPrivate* p)
{
    float val;

    val = phat_fan_slider_get_value(fan);
    patch_set_resonance(p->patch, val);
}


static void connect(FilterTabPrivate* p)
{
    g_signal_connect(G_OBJECT(p->freq_fan), "value-changed",
                        G_CALLBACK(freq_cb), (gpointer)p);
    g_signal_connect(G_OBJECT(p->reso_fan), "value-changed",
                        G_CALLBACK(reso_cb), (gpointer)p);
}


static void block(FilterTabPrivate* p)
{
    g_signal_handlers_block_by_func(p->freq_fan, freq_cb, p);
    g_signal_handlers_block_by_func(p->reso_fan, reso_cb, p);
}


static void unblock(FilterTabPrivate* p)
{
    g_signal_handlers_unblock_by_func(p->freq_fan, freq_cb, p);
    g_signal_handlers_unblock_by_func(p->reso_fan, reso_cb, p);
}


static gboolean refresh(gpointer data)
{
    FilterTabPrivate* p = FILTER_TAB_GET_PRIVATE(data);
    float cut, res;

    if (p->patch < 0)
        return TRUE;

    cut = patch_get_cutoff(p->patch);
    res = patch_get_resonance(p->patch);

    block(p);

    phat_fan_slider_set_value(PHAT_FAN_SLIDER(p->freq_fan), cut);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(p->reso_fan), res);

    unblock(p);

    return TRUE;
}


static void filter_tab_init(FilterTab* self)
{
    FilterTabPrivate* p = FILTER_TAB_GET_PRIVATE(self);
    GtkBox* box = GTK_BOX(self);
    GtkWidget* title;
    GtkWidget* table;
    GtkTable* t;
    GtkWidget* pad;
    GtkWidget* label;

    p->patch = -1;
    gtk_container_set_border_width(GTK_CONTAINER(self), GUI_BORDERSPACE);

    /* table */
    table = gtk_table_new(4, 4, FALSE);
    t = (GtkTable*) table;
    gtk_box_pack_start(box, table, FALSE, FALSE, 0);
    gtk_widget_show(table);
    
    /* filter title */
    title = gui_title_new("Filter");
    gtk_table_attach_defaults(t, title, 0, 4, 0, 1);
    gtk_widget_show(title);

    /* indentation */
    pad = gui_hpad_new(GUI_INDENT);
    gtk_table_attach(t, pad, 0, 1, 1, 2, 0, 0, 0, 0);
    gtk_widget_show(pad);
    
    /* filter title padding */
    pad = gui_vpad_new(GUI_TITLESPACE);
    gtk_table_attach(t, pad, 1, 2, 1, 2, 0, 0, 0, 0);
    gtk_widget_show(pad);

    /* label-fan column spacing */
    pad = gui_hpad_new(GUI_TEXTSPACE);
    gtk_table_attach(t, pad, 2, 3, 1, 2, 0, 0, 0, 0);
    gtk_widget_show(pad);

    /* freq fan */
    label = gtk_label_new("Cutoff:");
    p->freq_fan = phat_hfan_slider_new_with_range(1.0, 0.0, 1.0, 0.01);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, 2, 3, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, p->freq_fan, 3, 4, 2, 3);
    gtk_widget_show(label);
    gtk_widget_show(p->freq_fan);

    /* reso fan */
    label = gtk_label_new("Resonance:");
    p->reso_fan = phat_hfan_slider_new_with_range(0.0, 0.0, 1.0, 0.01);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, 3, 4, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, p->reso_fan, 3, 4, 3, 4);
    gtk_widget_show(label);
    gtk_widget_show(p->reso_fan);

    /* done */
    connect(p);
    p->refresh = g_timeout_add(GUI_REFRESH_TIMEOUT, refresh,
        /* we do want to use self this time: */     (gpointer) self);
}


GtkWidget* filter_tab_new(void)
{
    return (GtkWidget*) g_object_new(FILTER_TAB_TYPE, NULL);
}


void filter_tab_set_patch(FilterTab* self, int patch)
{
    FilterTabPrivate* p = FILTER_TAB_GET_PRIVATE(self);
    float freq, reso;

    p->patch = patch;

    if (patch < 0)
        return;

    freq = patch_get_cutoff(patch);
    reso = patch_get_resonance(patch);

    block(p);

    phat_fan_slider_set_value(PHAT_FAN_SLIDER(p->freq_fan), freq);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(p->reso_fan), reso);

    unblock(p);
}

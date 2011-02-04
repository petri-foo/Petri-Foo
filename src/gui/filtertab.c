#include <gtk/gtk.h>
#include <phat/phat.h>
#include "filtertab.h"
#include "gui.h"
#include "patch.h"

static GtkVBoxClass* parent_class;

static void filter_tab_class_init(FilterTabClass* klass);
static void filter_tab_init(FilterTab* self);
static void filter_tab_destroy(GtkObject* object);


GType filter_tab_get_type(void)
{
    static GType type = 0;

    if (!type)
    {
	static const GTypeInfo info =
	    {
		sizeof (FilterTabClass),
		NULL,
		NULL,
		(GClassInitFunc) filter_tab_class_init,
		NULL,
		NULL,
		sizeof (FilterTab),
		0,
		(GInstanceInitFunc) filter_tab_init,
	    };

	type = g_type_register_static(GTK_TYPE_VBOX, "FilterTab", &info, 0);
    }

    return type;
}


static void filter_tab_class_init(FilterTabClass* klass)
{
    parent_class = g_type_class_peek_parent(klass);
    GTK_OBJECT_CLASS(klass)->destroy = filter_tab_destroy;
}


static void freq_cb(PhatFanSlider* fan, FilterTab* self)
{
    float val;

    val = phat_fan_slider_get_value(fan);
    patch_set_cutoff(self->patch, val);
}


static void reso_cb(PhatFanSlider* fan, FilterTab* self)
{
    float val;

    val = phat_fan_slider_get_value(fan);
    patch_set_resonance(self->patch, val);
}


static void connect(FilterTab* self)
{
    g_signal_connect(G_OBJECT(self->freq_fan), "value-changed",
		     G_CALLBACK(freq_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->reso_fan), "value-changed",
		     G_CALLBACK(reso_cb), (gpointer) self);
}


static void block(FilterTab* self)
{
    g_signal_handlers_block_by_func(self->freq_fan, freq_cb, self);
    g_signal_handlers_block_by_func(self->reso_fan, reso_cb, self);
}


static void unblock(FilterTab* self)
{
    g_signal_handlers_unblock_by_func(self->freq_fan, freq_cb, self);
    g_signal_handlers_unblock_by_func(self->reso_fan, reso_cb, self);
}


static gboolean refresh(gpointer data)
{
    FilterTab* self = FILTER_TAB(data);
    float cut, res;

    if (self->patch < 0)
	return TRUE;
    
    cut = patch_get_cutoff(self->patch);
    res = patch_get_resonance(self->patch);

    block(self);
    
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->freq_fan), cut);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->reso_fan), res);
    
    unblock(self);

    return TRUE;
}


static void filter_tab_init(FilterTab* self)
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
    self->freq_fan = phat_hfan_slider_new_with_range(1.0, 0.0, 1.0, 0.01);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, 2, 3, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, self->freq_fan, 3, 4, 2, 3);
    gtk_widget_show(label);
    gtk_widget_show(self->freq_fan);

    /* reso fan */
    label = gtk_label_new("Resonance:");
    self->reso_fan = phat_hfan_slider_new_with_range(0.0, 0.0, 1.0, 0.01);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, 3, 4, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, self->reso_fan, 3, 4, 3, 4);
    gtk_widget_show(label);
    gtk_widget_show(self->reso_fan);

    /* done */
    connect(self);
    self->refresh = g_timeout_add(GUI_REFRESH_TIMEOUT, refresh, (gpointer) self);
}


static void filter_tab_destroy(GtkObject* object)
{
    FilterTab* self = FILTER_TAB(object);
    GtkObjectClass* klass = GTK_OBJECT_CLASS(parent_class);

    if (!g_source_remove(self->refresh))
    {
	debug("failed to remove refresh function from idle loop: %u\n", self->refresh);
    }
    else
    {
	debug("refresh function removed\n");
    }
    
    
    if (klass->destroy)
	klass->destroy(object);
}


GtkWidget* filter_tab_new(void)
{
    return (GtkWidget*) g_object_new(FILTER_TAB_TYPE, NULL);
}


void filter_tab_set_patch(FilterTab* self, int patch)
{
    float freq, reso;

    self->patch = patch;

    if (patch < 0)
	return;

    freq = patch_get_cutoff(patch);
    reso = patch_get_resonance(patch);

    block(self);
    
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->freq_fan), freq);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->reso_fan), reso);

    unblock(self);
}

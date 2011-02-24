#include <gtk/gtk.h>
#include <phat/phat.h>
#include "voicetab.h"
#include "gui.h"
#include "patch_set_and_get.h"

static GtkVBoxClass* parent_class;

static void voice_tab_class_init(VoiceTabClass* klass);
static void voice_tab_init(VoiceTab* self);
static void voice_tab_destroy(GtkObject* object);


GType voice_tab_get_type(void)
{
    static GType type = 0;

    if (!type)
    {
	static const GTypeInfo info =
	    {
		sizeof (VoiceTabClass),
		NULL,
		NULL,
		(GClassInitFunc) voice_tab_class_init,
		NULL,
		NULL,
		sizeof (VoiceTab),
		0,
		(GInstanceInitFunc) voice_tab_init,
        NULL
	    };

	type = g_type_register_static(GTK_TYPE_VBOX, "VoiceTab", &info, 0);
    }

    return type;
}


static void voice_tab_class_init(VoiceTabClass* klass)
{
    parent_class = g_type_class_peek_parent(klass);
    GTK_OBJECT_CLASS(klass)->destroy = voice_tab_destroy;
}


static void cut_cb(PhatSliderButton* button, VoiceTab* self)
{
    int val;

    val = phat_slider_button_get_value(button);
    patch_set_cut(self->patch, val);
}


static void cutby_cb(PhatSliderButton* button, VoiceTab* self)
{
    int val;

    val = phat_slider_button_get_value(button);
    patch_set_cut_by(self->patch, val);
}


static void porta_cb(GtkToggleButton* button, VoiceTab* self)
{
    patch_set_portamento(self->patch,
			 gtk_toggle_button_get_active(button));
}


static void porta_cb2(GtkToggleButton* button, VoiceTab* self)
{
    if (gtk_toggle_button_get_active(button))
    {
	gtk_widget_set_sensitive(self->time_fan, TRUE);
    }
    else
    {
	gtk_widget_set_sensitive(self->time_fan, FALSE);
    }
}


static void time_cb(PhatFanSlider* fan, VoiceTab* self)
{
    float val;

    val = phat_fan_slider_get_value(fan);
    patch_set_portamento_time(self->patch, val);
}


static void mono_cb(GtkToggleButton* button, VoiceTab* self)
{
    patch_set_monophonic(self->patch,
			 gtk_toggle_button_get_active(button));
}


static void mono_cb2(GtkToggleButton* button, VoiceTab* self)
{
    gtk_widget_set_sensitive(self->legato_check,
			     gtk_toggle_button_get_active(button));
}


static void legato_cb(GtkToggleButton* button, VoiceTab* self)
{
    patch_set_legato(self->patch,
		     gtk_toggle_button_get_active(button));
}


static void connect(VoiceTab* self)
{
    g_signal_connect(G_OBJECT(self->cut_sb), "value-changed",
		     G_CALLBACK(cut_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->cutby_sb), "value-changed",
		     G_CALLBACK(cutby_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->mono_check), "toggled",
		     G_CALLBACK(mono_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->mono_check), "toggled",
		     G_CALLBACK(mono_cb2), (gpointer) self);
    g_signal_connect(G_OBJECT(self->legato_check), "toggled",
		     G_CALLBACK(legato_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->porta_check), "toggled",
		     G_CALLBACK(porta_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->porta_check), "toggled",
		     G_CALLBACK(porta_cb2), (gpointer) self);
    g_signal_connect(G_OBJECT(self->time_fan), "value-changed",
		     G_CALLBACK(time_cb), (gpointer) self);
}


static void block(VoiceTab* self)
{
    g_signal_handlers_block_by_func(self->cut_sb, cut_cb, self);
    g_signal_handlers_block_by_func(self->cutby_sb, cutby_cb, self);
    g_signal_handlers_block_by_func(self->mono_check, mono_cb, self);
    g_signal_handlers_block_by_func(self->legato_check, legato_cb, self);
    g_signal_handlers_block_by_func(self->porta_check, porta_cb, self);
    g_signal_handlers_block_by_func(self->time_fan, time_cb, self);

    /* *_cb2 intentionally omitted */
}


static void unblock(VoiceTab* self)
{
    g_signal_handlers_unblock_by_func(self->cut_sb, cut_cb, self);
    g_signal_handlers_unblock_by_func(self->cutby_sb, cutby_cb, self);
    g_signal_handlers_unblock_by_func(self->mono_check, mono_cb, self);
    g_signal_handlers_unblock_by_func(self->legato_check, legato_cb, self);
    g_signal_handlers_unblock_by_func(self->porta_check, porta_cb, self);
    g_signal_handlers_unblock_by_func(self->time_fan, time_cb, self);

    /* *_cb2 intentionally omitted */
}


static gboolean refresh(gpointer data)
{
    VoiceTab* self = VOICE_TAB(data);
    gboolean porta;
    float time;

    if (self->patch < 0)
	return TRUE;
    
    porta = patch_get_portamento(self->patch);
    time = patch_get_portamento_time(self->patch);

    block(self);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->porta_check), porta);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->time_fan), time);

    unblock(self);

    return TRUE;
}


static void voice_tab_init(VoiceTab* self)
{
    GtkBox* box = GTK_BOX(self);
    GtkWidget* title;
    GtkWidget* table;
    GtkTable* t;
    GtkWidget* pad;
    GtkWidget* label;

    self->patch = -1;
    self->refresh = -1;

    gtk_container_set_border_width(GTK_CONTAINER(self), GUI_BORDERSPACE);

    /* table */
    table = gtk_table_new(8, 6, FALSE);
    t = (GtkTable*) table;
    gtk_box_pack_start(box, table, FALSE, FALSE, 0);
    gtk_widget_show(table);
    
    /* voice title */
    title = gui_title_new("Voice");
    gtk_table_attach_defaults(t, title, 0, 6, 0, 1);
    gtk_widget_show(title);

    /* indentation */
    pad = gui_hpad_new(GUI_INDENT);
    gtk_table_attach(t, pad, 0, 1, 1, 2, 0, 0, 0, 0);
    gtk_widget_show(pad);
    
    /* voice title padding */
    pad = gui_vpad_new(GUI_TITLESPACE);
    gtk_table_attach(t, pad, 1, 2, 1, 2, 0, 0, 0, 0);
    gtk_widget_show(pad);

    /* portamento title padding */
    pad = gui_vpad_new(GUI_TITLESPACE);
    gtk_table_attach(t, pad, 1, 2, 6, 7, 0, 0, 0, 0);
    gtk_widget_show(pad);

    /* cut-mono column spacing */
    pad = gui_hpad_new(GUI_SECSPACE);
    gtk_table_attach(t, pad, 4, 5, 1, 2, 0, 0, 0, 0);
    gtk_widget_show(pad);

    /* time-fan column spacing */
    pad = gui_hpad_new(GUI_TEXTSPACE);
    gtk_table_attach(t, pad, 2, 3, 1, 2, 0, 0, 0, 0);
    gtk_widget_show(pad);

    /* section spacing */
    pad = gui_vpad_new(GUI_SECSPACE);
    gtk_table_attach(t, pad, 0, 1, 4, 5, 0, 0, 0, 0);
    gtk_widget_show(pad);

    /* cut sliderbutton */
    self->cut_sb = phat_slider_button_new_with_range(0, 0, 99, 1, 0);
    phat_slider_button_set_format(PHAT_SLIDER_BUTTON(self->cut_sb), 0, "Cut:", NULL);
    phat_slider_button_set_threshold(PHAT_SLIDER_BUTTON(self->cut_sb), GUI_THRESHOLD);
    gtk_table_attach_defaults(t, self->cut_sb, 1, 4, 2, 3);
    gtk_widget_show(self->cut_sb);

    gtk_table_set_row_spacing(t, 2, GUI_SPACING);

    /* cutby sliderbutton */
    self->cutby_sb = phat_slider_button_new_with_range(0, 0, 99, 1, 0);
    phat_slider_button_set_format(PHAT_SLIDER_BUTTON(self->cutby_sb), 0, "Cut by:", NULL);
    phat_slider_button_set_threshold(PHAT_SLIDER_BUTTON(self->cutby_sb), GUI_THRESHOLD);
    gtk_table_attach_defaults(t, self->cutby_sb, 1, 4, 3, 4);
    gtk_widget_show(self->cutby_sb);

    /* mono sliderbutton */
    self->mono_check = gtk_check_button_new_with_label("Monophonic");
    gtk_table_attach_defaults(t, self->mono_check, 5, 6, 2, 3);
    gtk_widget_show(self->mono_check);

    /* legato sliderbutton */
    self->legato_check = gtk_check_button_new_with_label("Legato");
    gtk_table_attach_defaults(t, self->legato_check, 5, 6, 3, 4);
    gtk_widget_show(self->legato_check);
    gtk_widget_set_sensitive(self->legato_check, FALSE);

    /* portamento checkbox */
    title = gui_title_new("Portamento");
    self->porta_check = gtk_check_button_new();
    gtk_container_add(GTK_CONTAINER(self->porta_check), title);
    gtk_table_attach_defaults(t, self->porta_check, 0, 6, 5, 6);
    gtk_widget_show(title);
    gtk_widget_show(self->porta_check);

    /* time fan */
    label = gtk_label_new("Time:");
    self->time_fan = phat_hfan_slider_new_with_range(0.05, 0.0, .25, 0.01);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, 7, 8, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, self->time_fan, 3, 4, 7, 8);
    gtk_widget_show(label);
    gtk_widget_show(self->time_fan);
    gtk_widget_set_sensitive(self->time_fan, FALSE);

    /* done */
    connect(self);
    self->refresh = g_timeout_add(GUI_REFRESH_TIMEOUT, refresh, (gpointer) self);
}


static void voice_tab_destroy(GtkObject* object)
{
    VoiceTab* self = VOICE_TAB(object);
    GtkObjectClass* klass = GTK_OBJECT_CLASS(parent_class);

debug("hi there>> refresh:%d\n",self->refresh);

    if (!g_source_remove(self->refresh))
    {
        debug("failed to remove refresh function from idle loop: %u\n",
                                                            self->refresh);
    }
    else
    {
        debug("refresh function removed\n");
    }

    if (klass->destroy)
        klass->destroy(object);
}


GtkWidget* voice_tab_new(void)
{
    return (GtkWidget*) g_object_new(VOICE_TAB_TYPE, NULL);
}


void voice_tab_set_patch(VoiceTab* self, int patch)
{
    int cut, cutby;
    gboolean porta, mono, legato;
    float time;

    self->patch = patch;

    if (patch < 0)
	return;

    cut = patch_get_cut(patch);
    cutby = patch_get_cut_by(patch);
    porta = patch_get_portamento(patch);
    mono = patch_get_monophonic(patch);
    legato = patch_get_legato(patch);
    time = patch_get_portamento_time(patch);

    block(self);
    
    phat_slider_button_set_value(PHAT_SLIDER_BUTTON(self->cut_sb), cut);
    phat_slider_button_set_value(PHAT_SLIDER_BUTTON(self->cutby_sb), cutby);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->porta_check), porta);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->mono_check), mono);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->legato_check), legato);

    phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->time_fan), time);

    unblock(self);
}
    

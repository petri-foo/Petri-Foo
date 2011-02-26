#include <gtk/gtk.h>
#include <phat/phat.h>
#include "voicetab.h"
#include "gui.h"
#include "patch_set_and_get.h"


typedef struct _VoiceTabPrivate VoiceTabPrivate;

#define VOICE_TAB_GET_PRIVATE(obj)      \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
        VOICE_TAB_TYPE, VoiceTabPrivate))

struct _VoiceTabPrivate
{
    int patch;
    guint refresh;
    GtkWidget* cut_sb;
    GtkWidget* cutby_sb;
    GtkWidget* time_fan;
    GtkWidget* mono_check;
    GtkWidget* legato_check;
    GtkWidget* porta_check;
};


G_DEFINE_TYPE(VoiceTab, voice_tab, GTK_TYPE_VBOX);


static void voice_tab_class_init(VoiceTabClass* klass)
{
    GtkObjectClass *object_class = GTK_OBJECT_CLASS(klass);
    voice_tab_parent_class = g_type_class_peek_parent(klass);
    g_type_class_add_private(object_class, sizeof(VoiceTabPrivate));
}


static void cut_cb(PhatSliderButton* button, VoiceTabPrivate* p)
{
    int val = phat_slider_button_get_value(button);
    patch_set_cut(p->patch, val);
}


static void cutby_cb(PhatSliderButton* button, VoiceTabPrivate* p)
{
    int val = phat_slider_button_get_value(button);
    patch_set_cut_by(p->patch, val);
}


static void porta_cb(GtkToggleButton* button, VoiceTabPrivate* p)
{
    patch_set_portamento(p->patch, gtk_toggle_button_get_active(button));
}


static void porta_cb2(GtkToggleButton* button, VoiceTabPrivate* p)
{
    if (gtk_toggle_button_get_active(button))
        gtk_widget_set_sensitive(p->time_fan, TRUE);
    else
        gtk_widget_set_sensitive(p->time_fan, FALSE);
}


static void time_cb(PhatFanSlider* fan, VoiceTabPrivate* p)
{
    float val = phat_fan_slider_get_value(fan);
    patch_set_portamento_time(p->patch, val);
}


static void mono_cb(GtkToggleButton* button, VoiceTabPrivate* p)
{
    patch_set_monophonic(p->patch, gtk_toggle_button_get_active(button));
}


static void mono_cb2(GtkToggleButton* button, VoiceTabPrivate* p)
{
    gtk_widget_set_sensitive(p->legato_check, 
                            gtk_toggle_button_get_active(button));
}


static void legato_cb(GtkToggleButton* button, VoiceTabPrivate* p)
{
    patch_set_legato(p->patch, gtk_toggle_button_get_active(button));
}


static void connect(VoiceTabPrivate* p)
{
    g_signal_connect(G_OBJECT(p->cut_sb), "value-changed",
                        G_CALLBACK(cut_cb), (gpointer)p);
    g_signal_connect(G_OBJECT(p->cutby_sb), "value-changed",
                        G_CALLBACK(cutby_cb), (gpointer)p);
    g_signal_connect(G_OBJECT(p->mono_check), "toggled",
                        G_CALLBACK(mono_cb), (gpointer)p);
    g_signal_connect(G_OBJECT(p->mono_check), "toggled",
                        G_CALLBACK(mono_cb2), (gpointer)p);
    g_signal_connect(G_OBJECT(p->legato_check), "toggled",
                        G_CALLBACK(legato_cb), (gpointer)p);
    g_signal_connect(G_OBJECT(p->porta_check), "toggled",
                        G_CALLBACK(porta_cb), (gpointer)p);
    g_signal_connect(G_OBJECT(p->porta_check), "toggled",
                        G_CALLBACK(porta_cb2), (gpointer)p);
    g_signal_connect(G_OBJECT(p->time_fan), "value-changed",
                        G_CALLBACK(time_cb), (gpointer)p);
}


static void block(VoiceTabPrivate* p)
{
    g_signal_handlers_block_by_func(p->cut_sb,      cut_cb,     p);
    g_signal_handlers_block_by_func(p->cutby_sb,    cutby_cb,   p);
    g_signal_handlers_block_by_func(p->mono_check,  mono_cb,    p);
    g_signal_handlers_block_by_func(p->legato_check,legato_cb,  p);
    g_signal_handlers_block_by_func(p->porta_check, porta_cb,   p);
    g_signal_handlers_block_by_func(p->time_fan,    time_cb,    p);
    /* *_cb2 intentionally omitted */
}


static void unblock(VoiceTabPrivate* p)
{
    g_signal_handlers_unblock_by_func(p->cut_sb,        cut_cb,     p);
    g_signal_handlers_unblock_by_func(p->cutby_sb,      cutby_cb,   p);
    g_signal_handlers_unblock_by_func(p->mono_check,    mono_cb,    p);
    g_signal_handlers_unblock_by_func(p->legato_check,  legato_cb,  p);
    g_signal_handlers_unblock_by_func(p->porta_check,   porta_cb,   p);
    g_signal_handlers_unblock_by_func(p->time_fan,      time_cb,    p);
    /* *_cb2 intentionally omitted */
}


static gboolean refresh(gpointer data)
{
    VoiceTabPrivate* p = data;
    gboolean porta;
    float time;

    if (p->patch < 0)
        return TRUE;

    porta = patch_get_portamento(p->patch);
    time = patch_get_portamento_time(p->patch);

    block(p);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->porta_check), porta);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(p->time_fan), time);

    unblock(p);

    return TRUE;
}


static void voice_tab_init(VoiceTab* self)
{
    VoiceTabPrivate* p = VOICE_TAB_GET_PRIVATE(self);
    GtkBox* box = GTK_BOX(self);
    GtkWidget* title;
    GtkWidget* table;
    GtkTable* t;
    GtkWidget* pad;
    GtkWidget* label;

    p->patch = -1;
    p->refresh = -1;

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
    p->cut_sb = phat_slider_button_new_with_range(0, 0, 99, 1, 0);
    phat_slider_button_set_format(PHAT_SLIDER_BUTTON(p->cut_sb), 0,
                                                    "Cut:", NULL);
    phat_slider_button_set_threshold(PHAT_SLIDER_BUTTON(p->cut_sb),
                                                    GUI_THRESHOLD);
    gtk_table_attach_defaults(t, p->cut_sb, 1, 4, 2, 3);
    gtk_widget_show(p->cut_sb);

    gtk_table_set_row_spacing(t, 2, GUI_SPACING);

    /* cutby sliderbutton */
    p->cutby_sb = phat_slider_button_new_with_range(0, 0, 99, 1, 0);
    phat_slider_button_set_format(PHAT_SLIDER_BUTTON(p->cutby_sb), 0,
                                                    "Cut by:", NULL);
    phat_slider_button_set_threshold(PHAT_SLIDER_BUTTON(p->cutby_sb),
                                                    GUI_THRESHOLD);
    gtk_table_attach_defaults(t, p->cutby_sb, 1, 4, 3, 4);
    gtk_widget_show(p->cutby_sb);

    /* mono sliderbutton */
    p->mono_check = gtk_check_button_new_with_label("Monophonic");
    gtk_table_attach_defaults(t, p->mono_check, 5, 6, 2, 3);
    gtk_widget_show(p->mono_check);

    /* legato sliderbutton */
    p->legato_check = gtk_check_button_new_with_label("Legato");
    gtk_table_attach_defaults(t, p->legato_check, 5, 6, 3, 4);
    gtk_widget_show(p->legato_check);
    gtk_widget_set_sensitive(p->legato_check, FALSE);

    /* portamento checkbox */
    title = gui_title_new("Portamento");
    p->porta_check = gtk_check_button_new();
    gtk_container_add(GTK_CONTAINER(p->porta_check), title);
    gtk_table_attach_defaults(t, p->porta_check, 0, 6, 5, 6);
    gtk_widget_show(title);
    gtk_widget_show(p->porta_check);

    /* time fan */
    label = gtk_label_new("Time:");
    p->time_fan = phat_hfan_slider_new_with_range(0.05, 0.0, 1.0, 0.01);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, 7, 8, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, p->time_fan, 3, 4, 7, 8);
    gtk_widget_show(label);
    gtk_widget_show(p->time_fan);
    gtk_widget_set_sensitive(p->time_fan, FALSE);

    /* done */
    connect(p);
    p->refresh = g_timeout_add(GUI_REFRESH_TIMEOUT, refresh, (gpointer)p);
}


GtkWidget* voice_tab_new(void)
{
    return (GtkWidget*) g_object_new(VOICE_TAB_TYPE, NULL);
}


void voice_tab_set_patch(VoiceTab* self, int patch)
{
    VoiceTabPrivate* p = VOICE_TAB_GET_PRIVATE(self);
    int cut, cutby;
    gboolean porta, mono, legato;
    float time;

    p->patch = patch;

    if (patch < 0)
        return;

    cut = patch_get_cut(patch);
    cutby = patch_get_cut_by(patch);
    porta = patch_get_portamento(patch);
    mono = patch_get_monophonic(patch);
    legato = patch_get_legato(patch);
    time = patch_get_portamento_time(patch);

    block(p);

    phat_slider_button_set_value(PHAT_SLIDER_BUTTON(p->cut_sb), cut);
    phat_slider_button_set_value(PHAT_SLIDER_BUTTON(p->cutby_sb), cutby);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->porta_check), porta);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->mono_check), mono);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->legato_check),legato);

    phat_fan_slider_set_value(PHAT_FAN_SLIDER(p->time_fan), time);

    unblock(p);
}

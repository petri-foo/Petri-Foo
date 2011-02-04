#include <gtk/gtk.h>
#include <phat/phat.h>
#include "lfotab.h"
#include "gui.h"
#include "idselector.h"
#include "lfo.h"
#include "patch.h"

/* must match order of items in menu */
enum
{
    SINE,
    TRIANGLE,
    SAW,
    SQUARE,
};

enum
{
    MAXSTEPS = PATCH_MAX_PITCH_STEPS,
};

static GtkVBoxClass* parent_class;

static void lfo_tab_class_init(LfoTabClass* klass);
static void lfo_tab_init(LfoTab* self);
static void update_lfo(LfoTab* self);

GType lfo_tab_get_type(void)
{
    static GType type = 0;

    if (!type)
    {
	static const GTypeInfo info =
	    {
		sizeof (LfoTabClass),
		NULL,
		NULL,
		(GClassInitFunc) lfo_tab_class_init,
		NULL,
		NULL,
		sizeof (LfoTab),
		0,
		(GInstanceInitFunc) lfo_tab_init,
	    };

	type = g_type_register_static(GTK_TYPE_VBOX, "LfoTab", &info, 0);
    }

    return type;
}


static void lfo_tab_class_init(LfoTabClass* klass)
{
    parent_class = g_type_class_peek_parent(klass);
}


static void set_sensitive(LfoTab* self, gboolean val)
{
    gboolean active;
    
    gtk_widget_set_sensitive(self->shape_opt, val);
    gtk_widget_set_sensitive(self->free_radio, val);
    gtk_widget_set_sensitive(self->sync_radio, val);
    gtk_widget_set_sensitive(self->beats_sb, val);
    gtk_widget_set_sensitive(self->pos_check, val);
    gtk_widget_set_sensitive(self->delay_fan, val);
    gtk_widget_set_sensitive(self->attack_fan, val);

    active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->sync_radio));

    gtk_widget_set_sensitive(self->speed_fan, !active && val);
    gtk_widget_set_sensitive(self->beats_sb, active && val);
}


static void param_cb(IDSelector* ids, LfoTab* self)
{
    update_lfo(self);
}


static void on_cb(GtkToggleButton* button, LfoTab* self)
{
    patch_set_lfo_on(self->patch,
            id_selector_get_id(ID_SELECTOR(self->idsel)),
                    gtk_toggle_button_get_active(button));
}


static void on_cb2(GtkToggleButton* button, LfoTab* self)
{
    set_sensitive(self, gtk_toggle_button_get_active(button));
}


static void shape_cb(GtkOptionMenu* opt, LfoTab* self)
{
    int val;
    LFOShape shape;

    val = gtk_option_menu_get_history(opt);

    switch(val)
    {
    case TRIANGLE:  shape = LFO_SHAPE_TRIANGLE; break;
    case SAW:       shape = LFO_SHAPE_SAW;      break;
    case SQUARE:    shape = LFO_SHAPE_SQUARE;   break;
    default:        shape = LFO_SHAPE_SINE;     break;
    }

    patch_set_lfo_shape(self->patch,
            id_selector_get_id(ID_SELECTOR(self->idsel)),
            shape);
}


static void sync_cb(GtkToggleButton* button, LfoTab* self)
{
    patch_set_lfo_sync(self->patch, 
            id_selector_get_id(ID_SELECTOR(self->idsel)),
            gtk_toggle_button_get_active(button));
}


static void sync_cb2(GtkToggleButton* button, LfoTab* self)
{
    set_sensitive(self,
            gtk_toggle_button_get_active(
                    GTK_TOGGLE_BUTTON(self->lfo_check)));
}


static void speed_cb(PhatFanSlider* fan, LfoTab* self)
{
    patch_set_lfo_freq( self->patch,
                        id_selector_get_id(ID_SELECTOR(self->idsel)),
                        phat_fan_slider_get_value(fan));
}


static void beats_cb(PhatSliderButton* button, LfoTab* self)
{
    patch_set_lfo_beats(self->patch,
                        id_selector_get_id(ID_SELECTOR(self->idsel)),
                        phat_slider_button_get_value(button));
}


static void positive_cb(GtkToggleButton* button, LfoTab* self)
{
    patch_set_lfo_positive(self->patch, 
                        id_selector_get_id(ID_SELECTOR(self->idsel)),
                        gtk_toggle_button_get_active(button));
}


static void delay_cb(PhatFanSlider* fan, LfoTab* self)
{
    patch_set_lfo_delay(self->patch,
                        id_selector_get_id(ID_SELECTOR(self->idsel)),
                        phat_fan_slider_get_value(fan));
}


static void attack_cb(PhatFanSlider* fan, LfoTab* self)
{
    patch_set_lfo_attack(self->patch,
                        id_selector_get_id(ID_SELECTOR(self->idsel)),
                        phat_fan_slider_get_value(fan));
}


static void connect(LfoTab* self)
{
    g_signal_connect(G_OBJECT(self->idsel), "changed",
                    G_CALLBACK(param_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->lfo_check), "toggled",
                    G_CALLBACK(on_cb), (gpointer)self);
    g_signal_connect(G_OBJECT(self->lfo_check), "toggled",
                    G_CALLBACK(on_cb2), (gpointer)self);
    g_signal_connect(G_OBJECT(self->shape_opt), "changed",
                    G_CALLBACK(shape_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->speed_fan), "value-changed",
                    G_CALLBACK(speed_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->sync_radio), "toggled",
                    G_CALLBACK(sync_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->sync_radio), "toggled",
                    G_CALLBACK(sync_cb2), (gpointer) self);
    g_signal_connect(G_OBJECT(self->pos_check), "toggled",
                    G_CALLBACK(positive_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->beats_sb), "value-changed",
                    G_CALLBACK(beats_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->delay_fan), "value-changed",
                    G_CALLBACK(delay_cb), (gpointer) self);
    g_signal_connect(G_OBJECT(self->attack_fan), "value-changed",
                    G_CALLBACK(attack_cb), (gpointer) self);
}


static void block(LfoTab* self)
{
    g_signal_handlers_block_by_func(self->lfo_check, on_cb, self);
    g_signal_handlers_block_by_func(self->shape_opt, shape_cb, self);
    g_signal_handlers_block_by_func(self->speed_fan, speed_cb, self);
    g_signal_handlers_block_by_func(self->sync_radio, sync_cb, self);
    g_signal_handlers_block_by_func(self->pos_check, positive_cb, self);
    g_signal_handlers_block_by_func(self->beats_sb, beats_cb, self);
    g_signal_handlers_block_by_func(self->delay_fan, delay_cb, self);
    g_signal_handlers_block_by_func(self->attack_fan, attack_cb, self);
}


static void unblock(LfoTab* self)
{
    g_signal_handlers_unblock_by_func(self->lfo_check, on_cb, self);
    g_signal_handlers_unblock_by_func(self->shape_opt, shape_cb, self);
    g_signal_handlers_unblock_by_func(self->speed_fan, speed_cb, self);    
    g_signal_handlers_unblock_by_func(self->sync_radio, sync_cb, self);
    g_signal_handlers_unblock_by_func(self->pos_check, positive_cb, self);
    g_signal_handlers_unblock_by_func(self->beats_sb, beats_cb, self);
    g_signal_handlers_unblock_by_func(self->delay_fan, delay_cb, self);
    g_signal_handlers_unblock_by_func(self->attack_fan, attack_cb, self);
}


inline static GtkWidget* shape_opt_new(LfoTab* self)
{
    GtkWidget* menu;
    GtkWidget* opt;
    GtkWidget* item;

        
    /* lfo menu */
    menu = gtk_menu_new();
    
    item = gtk_menu_item_new_with_label("Sine");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    gtk_widget_show(item);
    
    item = gtk_menu_item_new_with_label("Triangle");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    gtk_widget_show(item);
    
    item = gtk_menu_item_new_with_label("Saw");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    gtk_widget_show(item);
    
    item = gtk_menu_item_new_with_label("Square");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    gtk_widget_show(item);

    /* lfo option menu */
    opt = gtk_option_menu_new();
    gtk_option_menu_set_menu(GTK_OPTION_MENU(opt), menu);

    return opt;
}    


static void lfo_tab_init(LfoTab* self)
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
    id_selector_set(ID_SELECTOR(self->idsel), patch_lfo_names());
    gtk_box_pack_start(box, self->idsel, FALSE, FALSE, 0);
    gtk_widget_show(self->idsel);

    /* selector padding */
    pad = gui_vpad_new(GUI_SPACING);
    gtk_box_pack_start(box, pad, FALSE, FALSE, 0);
    gtk_widget_show(pad);

    /* table */
    table = gtk_table_new(10, 5, FALSE);
    t = (GtkTable*) table;
    gtk_box_pack_start(box, table, FALSE, FALSE, 0);
    gtk_widget_show(table);
    
    /* lfo title */
    title = gui_title_new("LFO");
    self->lfo_check = gtk_check_button_new();
    gtk_container_add(GTK_CONTAINER(self->lfo_check), title);
    gtk_table_attach_defaults(t, self->lfo_check, 0, 5, 0, 1);
    gtk_widget_show(title);
    gtk_widget_show(self->lfo_check);

    /* indentation */
    pad = gui_hpad_new(GUI_INDENT);
    gtk_table_attach(t, pad, 0, 1, 1, 2, 0, 0, 0, 0);
    gtk_widget_show(pad);
    
    /* lfo title padding */
    pad = gui_vpad_new(GUI_TITLESPACE);
    gtk_table_attach(t, pad, 1, 2, 1, 2, 0, 0, 0, 0);
    gtk_widget_show(pad);

    /* label-fan column spacing */
    pad = gui_hpad_new(GUI_TEXTSPACE);
    gtk_table_attach(t, pad, 2, 3, 1, 2, 0, 0, 0, 0);
    gtk_widget_show(pad);

    /* shape */
    label = gtk_label_new("Shape:");
    self->shape_opt = shape_opt_new(self);

    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, 2, 3, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, self->shape_opt, 3, 5, 2, 3);

    gtk_widget_show(label);
    gtk_widget_show(self->shape_opt);

    gtk_table_set_row_spacing(t, 2, GUI_SPACING);

    /* speed */
    label = gtk_label_new("Speed:");
    self->free_radio = gtk_radio_button_new(NULL);
    self->speed_fan = phat_hfan_slider_new_with_range(5.0, 0.0, 20.0, 0.1);

    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, 3, 4, GTK_FILL, 0, 0, 0);
    gtk_table_attach(t, self->free_radio, 2, 3, 3, 4, 0, 0, 0, 0);
    gtk_table_attach_defaults(t, self->speed_fan, 3, 5, 3, 4);

    gtk_widget_show(label);
    gtk_widget_show(self->free_radio);
    gtk_widget_show(self->speed_fan);

    /* sync */
    self->sync_radio = gtk_radio_button_new_from_widget(
                            GTK_RADIO_BUTTON(self->free_radio));
    self->beats_sb =
            phat_slider_button_new_with_range(1.0, .25, 32.0, .25, 2);

    phat_slider_button_set_format(PHAT_SLIDER_BUTTON(self->beats_sb),
                                    -1, NULL, "Beats");
    phat_slider_button_set_threshold(PHAT_SLIDER_BUTTON(self->beats_sb),
                                    GUI_THRESHOLD);

    gtk_table_attach(t, self->sync_radio, 2, 3, 4, 5, 0, 0, 0, 0);
    gtk_table_attach_defaults(t, self->beats_sb, 3, 5, 4, 5);

    gtk_widget_show(self->sync_radio);
    gtk_widget_show(self->beats_sb);
    gtk_widget_set_sensitive(self->beats_sb, FALSE);

    gtk_table_set_row_spacing(t, 4, GUI_SPACING);

    /* delay fan */
    label = gtk_label_new("Delay:");
    self->delay_fan = phat_hfan_slider_new_with_range(0.1, 0.0, 1.0, 0.01);

    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, 7, 8, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, self->delay_fan, 3, 4, 7, 8);

    gtk_widget_show(label);
    gtk_widget_show(self->delay_fan);

    gtk_table_set_col_spacing(t, 3, GUI_SPACING*2);

    /* attack fan */
    label = gtk_label_new("Attack:");
    self->attack_fan = phat_hfan_slider_new_with_range(0.1, 0.0, 1.0, 0.01);

    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, 8, 9, GTK_FILL, 0, 0, 0);
    gtk_table_attach_defaults(t, self->attack_fan, 3, 4, 8, 9);

    gtk_widget_show(label);
    gtk_widget_show(self->attack_fan);

    /* positive */
    self->pos_check = gtk_check_button_new_with_label("Positive");
    gtk_table_attach(t, self->pos_check, 4, 5, 8, 9, GTK_FILL, 0, 0, 0);
    gtk_widget_show(self->pos_check);

    gtk_table_set_row_spacing(t, 8, GUI_SPACING);

    gtk_widget_show(label);

    set_sensitive(self, FALSE);
    connect(self);
}


static void update_lfo(LfoTab* self)
{
    PatchParamType param;
    LFOShape shape;
    float speed, beats, delay, attack, amount;
    gboolean sync, global, positive;
    gboolean on;

    int id;

    id = id_selector_get_id(ID_SELECTOR(self->idsel));

    patch_get_lfo_shape(self->patch, id, &shape);
    patch_get_lfo_freq(self->patch, id, &speed);
    patch_get_lfo_beats(self->patch, id, &beats);
    patch_get_lfo_delay(self->patch, id, &delay);
    patch_get_lfo_attack(self->patch, id, &attack);
    patch_get_lfo_sync(self->patch, id, &sync);
    patch_get_lfo_positive(self->patch, id, &positive);
    patch_get_lfo_on(self->patch, id, &on);

    block(self);
    
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->speed_fan), speed);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->delay_fan), delay);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->attack_fan), attack);

    phat_slider_button_set_value(PHAT_SLIDER_BUTTON(self->beats_sb), beats);

    if (sync)
        gtk_toggle_button_set_active(
                    GTK_TOGGLE_BUTTON(self->sync_radio), TRUE);
    else
        gtk_toggle_button_set_active(
                    GTK_TOGGLE_BUTTON(self->free_radio), TRUE);

    gtk_toggle_button_set_active(
                    GTK_TOGGLE_BUTTON(self->pos_check), positive);

    switch (shape)
    {
    case LFO_SHAPE_TRIANGLE:
        gtk_option_menu_set_history(
                    GTK_OPTION_MENU(self->shape_opt), TRIANGLE);
        break;
    case LFO_SHAPE_SAW:
        gtk_option_menu_set_history(
                    GTK_OPTION_MENU(self->shape_opt), SAW);
        break;
    case LFO_SHAPE_SQUARE:
        gtk_option_menu_set_history(
                    GTK_OPTION_MENU(self->shape_opt), SQUARE);
        break;
    default:
        gtk_option_menu_set_history(
                    GTK_OPTION_MENU(self->shape_opt), SINE);
        break;
    }

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->lfo_check), on);

    unblock(self);
}


GtkWidget* lfo_tab_new(void)
{
    return (GtkWidget*) g_object_new(LFO_TAB_TYPE, NULL);
}


void lfo_tab_set_patch(LfoTab* self, int patch)
{
    self->patch = patch;

    if (patch < 0)
	return;

    update_lfo(self);
}

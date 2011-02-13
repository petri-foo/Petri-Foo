#include <gtk/gtk.h>
#include <phat/phat.h>

#include "mod_section.h"
#include "gui.h"
#include "patch.h"

#include "mod_src.h"

static GtkVBoxClass* parent_class;

static void mod_section_class_init(ModSectionClass* klass);
static void mod_section_init(ModSection* self);
static void mod_section_destroy(GtkObject* object);


GType mod_section_get_type(void)
{
    static GType type = 0;

    if (!type)
    {
        static const GTypeInfo info =
        {
            sizeof (ModSectionClass),
            NULL,
            NULL,
            (GClassInitFunc) mod_section_class_init,
            NULL,
            NULL,
            sizeof (ModSection),
            0,
            (GInstanceInitFunc) mod_section_init,
        };

        type = g_type_register_static(GTK_TYPE_VBOX,    "ModSection",
                                                        &info, 0);
    }

    return type;
}


static void mod_section_class_init(ModSectionClass* klass)
{
    parent_class = g_type_class_peek_parent(klass);
    GTK_OBJECT_CLASS(klass)->destroy = mod_section_destroy;
}

/* GUI callbacks
*/


static void param1_cb(GtkWidget* w, ModSection* self)
{
    float val = phat_fan_slider_get_value(PHAT_FAN_SLIDER(w));
    patch_param_set_value(self->patch_id, self->param, val);
}

static void param2_cb(GtkWidget* w, ModSection* self)
{
    if (self->param == PATCH_PARAM_PITCH)
    {
        float val = phat_slider_button_get_value(PHAT_SLIDER_BUTTON(w));
//        patch_set_range(self->patch_id, val);
        patch_set_pitch_steps(self->patch_id, val);
    }
}

static void vel_sens_cb(GtkWidget* w, ModSection* self)
{
    if (self->param != PATCH_PARAM_PITCH)
    {
        float val  = phat_fan_slider_get_value(PHAT_FAN_SLIDER(w));
        patch_set_vel_amount(self->patch_id, self->param, val);
    }
}


static void mod_src_cb(GtkComboBox* combo, ModSection* self)
{
    int input_id;

    if (combo == GTK_COMBO_BOX(self->env_combo))
        input_id = MOD_ENV;
    else if (combo == GTK_COMBO_BOX(self->mod1_combo))
        input_id = MOD_IN1;
    else if (combo == GTK_COMBO_BOX(self->mod2_combo))
        input_id = MOD_IN2;
    else
    {
        debug ("mod_src_cb called from unrecognised combo box\n");
        return;
    }

    mod_src_callback_helper(self->patch_id,
                                input_id,
                                combo,
                                self->param );
}

static void mod_amount_cb(GtkWidget* w, ModSection* self)
{
    float val = (self->param == PATCH_PARAM_PITCH
                    ? phat_slider_button_get_value(PHAT_SLIDER_BUTTON(w))
                        / PATCH_MAX_PITCH_STEPS
                    : phat_fan_slider_get_value(PHAT_FAN_SLIDER(w)));

    if (w == self->mod1_amount)
        patch_set_mod1_amt(self->patch_id, self->param, val);
    else if (w == self->mod2_amount)
        patch_set_mod2_amt(self->patch_id, self->param, val);
    else
    {
        debug ("mod_amount_cb called from unrecognised widget\n");
    }
}


static void connect(ModSection* self)
{
    if (self->mod_only)
        goto connect_mod_srcs;

    g_signal_connect(G_OBJECT(self->param1),        "value-changed",
                        G_CALLBACK(param1_cb),      (gpointer) self);

    if (self->param == PATCH_PARAM_PITCH)
        g_signal_connect(G_OBJECT(self->param2),    "value-changed",
                        G_CALLBACK(param2_cb),      (gpointer) self);
    else
        g_signal_connect(G_OBJECT(self->vel_sens),  "value-changed",
                        G_CALLBACK(vel_sens_cb),    (gpointer) self);

    if (self->param == PATCH_PARAM_AMPLITUDE)
        g_signal_connect(G_OBJECT(self->env_combo), "changed",
                        G_CALLBACK(mod_src_cb),     (gpointer) self);

connect_mod_srcs:

    g_signal_connect(G_OBJECT(self->mod1_combo),    "changed",
                        G_CALLBACK(mod_src_cb),    (gpointer) self);

    g_signal_connect(G_OBJECT(self->mod1_amount),   "value-changed",
                        G_CALLBACK(mod_amount_cb), (gpointer) self);

    g_signal_connect(G_OBJECT(self->mod2_combo),    "changed",
                        G_CALLBACK(mod_src_cb),    (gpointer) self);

    g_signal_connect(G_OBJECT(self->mod2_amount),   "value-changed",
                        G_CALLBACK(mod_amount_cb), (gpointer) self);
}


static void block(ModSection* self)
{
    if (self->mod_only)
        goto block_mod_srcs;

    g_signal_handlers_block_by_func(self->param1, param1_cb, self);

    if (self->param == PATCH_PARAM_PITCH)
        g_signal_handlers_block_by_func(self->param2,   param2_cb,  self);
    else
        g_signal_handlers_block_by_func(self->vel_sens, vel_sens_cb,self);

    if (self->param == PATCH_PARAM_AMPLITUDE)
        g_signal_handlers_block_by_func(self->env_combo,mod_src_cb, self);

block_mod_srcs:

    g_signal_handlers_block_by_func(self->mod1_combo,   mod_src_cb, self);
    g_signal_handlers_block_by_func(self->mod1_amount,  mod_amount_cb,
                                                                    self);
    g_signal_handlers_block_by_func(self->mod2_combo,   mod_src_cb,self);
    g_signal_handlers_block_by_func(self->mod2_amount,  mod_amount_cb,
                                                                    self);
}


static void unblock(ModSection* self)
{
    if (self->mod_only)
        goto unblock_mod_srcs;

    g_signal_handlers_unblock_by_func(self->param1, param1_cb, self);

    if (self->param == PATCH_PARAM_PITCH)
        g_signal_handlers_unblock_by_func(self->param2, param2_cb,  self);
    else
        g_signal_handlers_unblock_by_func(self->vel_sens, vel_sens_cb,
                                                                    self);
    if (self->param == PATCH_PARAM_AMPLITUDE)
        g_signal_handlers_unblock_by_func(self->env_combo, mod_src_cb,
                                                                    self);

unblock_mod_srcs:

    g_signal_handlers_unblock_by_func(self->mod1_combo, mod_src_cb,self);
    g_signal_handlers_unblock_by_func(self->mod1_amount,mod_amount_cb,
                                                                    self);
    g_signal_handlers_unblock_by_func(self->mod2_combo, mod_src_cb,self);
    g_signal_handlers_unblock_by_func(self->mod2_amount,mod_amount_cb,
                                                                    self);
}


static gboolean refresh(gpointer data)
{
    float param1, param2;

    ModSection* self = MOD_SECTION(data);

    if (self->patch_id < 0)
        return TRUE;
/*
    block(self);

    phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->amp_fan), amp);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->pan_fan), pan);

    unblock(self);
*/
    return TRUE;
}


static void mod_section_init(ModSection* self)
{
    self->patch_id = -1;
    self->mod_only = FALSE;
    self->param = PATCH_PARAM_INVALID;
    self->refresh = -1;
}


void mod_section_set_mod_only(ModSection* self)
{
    self->mod_only = TRUE;
}


void mod_section_set_param(ModSection* self, PatchParamType param)
{
    GtkBox* box;
    GtkWidget* title;
    GtkWidget* table;
    GtkTable* t;
    GtkWidget* pad;
    GtkWidget* label;

    float range_low = 0.0;
    float range_hi = 1.0;

    const char* lstr;
    const char** param_names = mod_src_param_names();

    int y = 0;

    box = GTK_BOX(self);
    self->param = param;

    gtk_container_set_border_width(GTK_CONTAINER(self), GUI_BORDERSPACE);

    /* table */
    table = gtk_table_new(8, 4, FALSE);
    t = (GtkTable*)table;
    gtk_box_pack_start(box, table, FALSE, FALSE, 0);
    gtk_widget_show(table);

    /* title */
    title = gui_title_new(param_names[param]);
    gtk_table_attach_defaults(t, title, 0, 4, y, y + 1);
    gtk_widget_show(title);

    ++y;

    /* indentation */
    pad = gui_hpad_new(GUI_INDENT);
    gtk_table_attach(t, pad, 0, 1, y, y + 1, 0, 0, 0, 0);
    gtk_widget_show(pad);

    ++y;

    /* title padding */
    pad = gui_vpad_new(GUI_TITLESPACE);
    gtk_table_attach(t, pad, 1, 2, y, y + 1, 0, 0, 0, 0);
    gtk_widget_show(pad);

    ++y;

    /* label column spacing (of some description!?) */
    pad = gui_hpad_new(GUI_TEXTSPACE);
    gtk_table_attach(t, pad, 2, 3, y, y + 1, 0, 0, 0, 0);
    gtk_widget_show(pad);

    ++y;

    if (self->mod_only)
        goto create_mod_srcs;

    switch(param)
    {
    case PATCH_PARAM_AMPLITUDE: lstr = "Level";     break;
    case PATCH_PARAM_PANNING:   lstr = "Position";  break;
    case PATCH_PARAM_PITCH:     lstr = "Tuning";    break;
                                                    break;
    default:
        lstr = param_names[param];
        break;
    }

    label = gtk_label_new(lstr);

    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table), label, 1, 2, y, y + 1,
                                            GTK_FILL, 0, 0, 0);
    gtk_widget_show(label);

    if (param == PATCH_PARAM_PANNING || param == PATCH_PARAM_PITCH)
        range_low = -1.0;

    self->param1 = phat_hfan_slider_new_with_range(0.0, range_low,
                                                        range_hi,   0.1);
    gtk_table_attach(GTK_TABLE(table), self->param1, 3, 4, y, y + 1,
                                            GTK_EXPAND | GTK_FILL, 0, 0, 0);
    gtk_widget_show(self->param1);


    if (param == PATCH_PARAM_PITCH)
    {
        self->param2 = mod_src_new_pitch_adjustment();
        gtk_table_attach(GTK_TABLE(table), self->param2, 5, 6, y, y + 1,
                                            GTK_EXPAND | GTK_FILL, 0, 0, 0);
        gtk_widget_show(self->param2);
    }
    else
    {
        /* velocity sensitivity */
        label = gtk_label_new("Vel.Sens:");
        gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
        gtk_table_attach(t, label, 5, 6, y, y + 1, GTK_FILL, 0, 0, 0);
        gtk_widget_show(label);

        self->vel_sens = phat_hfan_slider_new_with_range(0.0, 0.0,
                                                            1.0, 0.1);
        gtk_table_attach_defaults(t, self->vel_sens, 7, 8, y, y + 1);
        gtk_widget_show(self->vel_sens);
    }

    ++y;

    if (param == PATCH_PARAM_AMPLITUDE)
    {
        /* env input source */
        label = gtk_label_new("Env:");
        gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
        gtk_table_attach(t, label, 1, 2, y, y + 1, GTK_FILL, 0, 0, 0);
        gtk_widget_show(label);

        self->env_combo = mod_src_new_combo_with_cell();
        gtk_table_attach_defaults(t, self->env_combo, 3, 4, y, y + 1);
        gtk_widget_show(self->env_combo);

        ++y;
    }


create_mod_srcs:


    /* mod1 input source */
    label = gtk_label_new("Mod1:");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, y, y + 1, GTK_FILL, 0, 0, 0);
    gtk_widget_show(label);

    self->mod1_combo = mod_src_new_combo_with_cell();
    gtk_table_attach_defaults(t, self->mod1_combo, 3, 4, y, y + 1);
    gtk_widget_show(self->mod1_combo);

    if (param == PATCH_PARAM_PITCH)
        self->mod1_amount = mod_src_new_pitch_adjustment();
    else
        self->mod1_amount = phat_hfan_slider_new_with_range(0.0, -1.0,
                                                            1.0, 0.1);
    gtk_table_attach_defaults(t, self->mod1_amount, 5, 6, y, y + 1);
    gtk_widget_show(self->mod1_amount);

    ++y;

    /* mod2 input source */
    label = gtk_label_new("Mod2:");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(t, label, 1, 2, y, y + 1, GTK_FILL, 0, 0, 0);
    gtk_widget_show(label);

    self->mod2_combo = mod_src_new_combo_with_cell();
    gtk_table_attach_defaults(t, self->mod2_combo, 3, 4, y, y + 1);
    gtk_widget_show(self->mod2_combo);

    if (param == PATCH_PARAM_PITCH)
        self->mod2_amount = mod_src_new_pitch_adjustment();
    else
        self->mod2_amount = phat_hfan_slider_new_with_range(0.0, -1.0,
                                                            1.0, 0.1);
    gtk_table_attach_defaults(t, self->mod2_amount, 5, 6, y, y + 1);
    gtk_widget_show(self->mod2_amount);

    /* done */
    connect(self);

    switch(param)
    {
    case PATCH_PARAM_AMPLITUDE:
        self->refresh = g_timeout_add(GUI_REFRESH_TIMEOUT,  refresh,
                                                        (gpointer) self);
        break;
    default:
        self->refresh = -1;
    }
}


void mod_section_set_list_global(ModSection* self)
{
    mod_src_combo_set_model(GTK_COMBO_BOX(self->env_combo),
                                    MOD_SRC_INPUTS_GLOBAL);

    mod_src_combo_set_model(GTK_COMBO_BOX(self->mod1_combo),
                                    MOD_SRC_INPUTS_GLOBAL);

    mod_src_combo_set_model(GTK_COMBO_BOX(self->mod2_combo),
                                    MOD_SRC_INPUTS_GLOBAL);
}


void mod_section_set_list_all(ModSection* self)
{
    mod_src_combo_set_model(GTK_COMBO_BOX(self->env_combo),
                                    MOD_SRC_INPUTS_ALL);

    mod_src_combo_set_model(GTK_COMBO_BOX(self->mod1_combo),
                                    MOD_SRC_INPUTS_ALL);

    mod_src_combo_set_model(GTK_COMBO_BOX(self->mod2_combo),
                                    MOD_SRC_INPUTS_ALL);
}


static void mod_section_destroy(GtkObject* object)
{
    ModSection* self = MOD_SECTION(object);
    GtkObjectClass* klass = GTK_OBJECT_CLASS(parent_class);

    if (self->refresh != -1)
    {
        if (!g_source_remove(self->refresh))
        {
            debug("failed to remove refresh function from idle loop: %u\n",
                                                            self->refresh);
        }
        else
        {
            debug("refresh function removed\n");
        }
    }

    if (klass->destroy)
        klass->destroy(object);
}


GtkWidget* mod_section_new(void)
{
    return (GtkWidget*) g_object_new(MOD_SECTION_TYPE, NULL);
}


void mod_section_set_patch(ModSection* self, int patch_id)
{
    float param1 = PATCH_PARAM_INVALID;
    float param2 = PATCH_PARAM_INVALID;
    float vsens;
    int envsrc, m1src, m2src;
    float m1amt, m2amt;

    GtkTreeIter enviter;
    GtkTreeIter m1iter;
    GtkTreeIter m2iter;

    self->patch_id = patch_id;

    if (patch_id < 0)
        return;

    if (self->mod_only)
        goto get_mod_srcs;

    patch_param_get_value(self->patch_id, self->param, &param1);

    if (self->param == PATCH_PARAM_PITCH)
        param2 = patch_get_pitch_steps(self->patch_id);
    else
        patch_get_vel_amount(patch_id, self->param, &vsens);

get_mod_srcs:

    if (self->param == PATCH_PARAM_AMPLITUDE)
    {
        patch_get_amp_env(patch_id, &envsrc);

        if (!mod_src_combo_get_iter_with_id(GTK_COMBO_BOX(self->env_combo),
                                                envsrc, &enviter))
        {
            debug("failed to get env source id from combo box\n");
        }
    }

    patch_get_mod1_src(patch_id, self->param, &m1src);
    patch_get_mod2_src(patch_id, self->param, &m2src);

    patch_get_mod1_amt(patch_id, self->param, &m1amt);
    patch_get_mod2_amt(patch_id, self->param, &m2amt);

    if (!mod_src_combo_get_iter_with_id(GTK_COMBO_BOX(self->mod1_combo),
                                                        m1src, &m1iter))
    {
        debug("failed to get mod1 source id from combo box\n");
    }

    if (!mod_src_combo_get_iter_with_id(GTK_COMBO_BOX(self->mod2_combo),
                                                        m2src, &m2iter))
    {
        debug("failed to get mod2 source id from combo box\n");
    }

    block(self);

    if (self->mod_only)
        goto set_mod_srcs;

    phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->param1), param1);

    if (self->param == PATCH_PARAM_PITCH)
        phat_slider_button_set_value(PHAT_SLIDER_BUTTON(self->param2),
                                                                param2);
    else
        phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->vel_sens), vsens);

    if (self->param == PATCH_PARAM_AMPLITUDE)
        gtk_combo_box_set_active_iter(GTK_COMBO_BOX(self->env_combo),
                                                                &enviter);

set_mod_srcs:

    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(self->mod1_combo), &m1iter);
    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(self->mod2_combo), &m2iter);


    if (self->param == PATCH_PARAM_PITCH)
    {
        phat_slider_button_set_value(PHAT_SLIDER_BUTTON(self->mod1_amount),
                                            m1amt * PATCH_MAX_PITCH_STEPS);
        phat_slider_button_set_value(PHAT_SLIDER_BUTTON(self->mod2_amount),
                                            m2amt * PATCH_MAX_PITCH_STEPS);
    }
    else
    {
        phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->mod1_amount),
                                                                    m1amt);
        phat_fan_slider_set_value(PHAT_FAN_SLIDER(self->mod2_amount),
                                                                    m2amt);
    }

    unblock(self);
}

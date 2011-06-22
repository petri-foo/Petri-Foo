#include <gtk/gtk.h>
#include <phat/phat.h>

#include "mod_section.h"
#include "gui.h"
#include "patch_set_and_get.h"
#include "names.h"

#include "mod_src_gui.h"


typedef struct _ModSectionPrivate ModSectionPrivate;

#define MOD_SECTION_GET_PRIVATE(obj)   \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
        MOD_SECTION_TYPE, ModSectionPrivate))

struct _ModSectionPrivate
{
    int             patch_id;
    int             refresh;
    gboolean        mod_only;
    PatchParamType  param;
    GtkWidget*      param1;
    GtkWidget*      param2;
    GtkWidget*      env_combo;
    GtkWidget*      vel_sens;
    GtkWidget*      key_track;
    GtkWidget*      mod1_combo;
    GtkWidget*      mod1_amount;
    GtkWidget*      mod2_combo;
    GtkWidget*      mod2_amount;
};


G_DEFINE_TYPE(ModSection, mod_section, GTK_TYPE_VBOX);


static void mod_section_class_init(ModSectionClass* klass)
{
    GtkObjectClass *object_class = GTK_OBJECT_CLASS(klass);
    mod_section_parent_class = g_type_class_peek_parent(klass);
    g_type_class_add_private(object_class, sizeof(ModSectionPrivate));
}


static void param1_cb(GtkWidget* w, ModSectionPrivate* p)
{
    float val = phat_fan_slider_get_value(PHAT_FAN_SLIDER(w));
    patch_param_set_value(p->patch_id, p->param, val);
}

static void param2_cb(GtkWidget* w, ModSectionPrivate* p)
{
    if (p->param == PATCH_PARAM_PITCH)
    {
        float val = phat_slider_button_get_value(PHAT_SLIDER_BUTTON(w));
        patch_set_pitch_steps(p->patch_id, val);
    }
}

static void vel_sens_cb(GtkWidget* w, ModSectionPrivate* p)
{
    float val  = phat_fan_slider_get_value(PHAT_FAN_SLIDER(w));
    patch_set_vel_amount(p->patch_id, p->param, val);
}

static void key_track_cb(GtkWidget* w, ModSectionPrivate* p)
{
    float val  = phat_fan_slider_get_value(PHAT_FAN_SLIDER(w));
    patch_set_key_amount(p->patch_id, p->param, val);
}


static void mod_src_cb(GtkComboBox* combo, ModSectionPrivate* p)
{
    int input_id;

    if (combo == GTK_COMBO_BOX(p->env_combo))
        input_id = MOD_ENV;
    else if (combo == GTK_COMBO_BOX(p->mod1_combo))
        input_id = MOD_IN1;
    else if (combo == GTK_COMBO_BOX(p->mod2_combo))
        input_id = MOD_IN2;
    else
    {
        debug ("mod_src_cb called from unrecognised combo box\n");
        return;
    }

    mod_src_callback_helper(p->patch_id, input_id, combo, p->param );
}

static void mod_amount_cb(GtkWidget* w, ModSectionPrivate* p)
{
    float val = (p->param == PATCH_PARAM_PITCH
                    ? phat_slider_button_get_value(PHAT_SLIDER_BUTTON(w))
                        / PATCH_MAX_PITCH_STEPS
                    : phat_fan_slider_get_value(PHAT_FAN_SLIDER(w)));

    if (w == p->mod1_amount)
        patch_set_mod1_amt(p->patch_id, p->param, val);
    else if (w == p->mod2_amount)
        patch_set_mod2_amt(p->patch_id, p->param, val);
    else
    {
        debug ("mod_amount_cb called from unrecognised widget\n");
    }
}


static void connect(ModSectionPrivate* p)
{
    if (p->mod_only)
        goto connect_mod_srcs;

    g_signal_connect(G_OBJECT(p->param1),       "value-changed",
                        G_CALLBACK(param1_cb),      (gpointer)p);

    if (p->param == PATCH_PARAM_PITCH)
        g_signal_connect(G_OBJECT(p->param2),   "value-changed",
                        G_CALLBACK(param2_cb),      (gpointer)p);

    g_signal_connect(G_OBJECT(p->vel_sens),     "value-changed",
                        G_CALLBACK(vel_sens_cb),    (gpointer)p);

    g_signal_connect(G_OBJECT(p->key_track),    "value-changed",
                        G_CALLBACK(key_track_cb),   (gpointer)p);

    if (p->param == PATCH_PARAM_AMPLITUDE)
        g_signal_connect(G_OBJECT(p->env_combo),"changed",
                        G_CALLBACK(mod_src_cb),     (gpointer)p);

connect_mod_srcs:

    g_signal_connect(G_OBJECT(p->mod1_combo),    "changed",
                        G_CALLBACK(mod_src_cb),    (gpointer)p);

    g_signal_connect(G_OBJECT(p->mod1_amount),   "value-changed",
                        G_CALLBACK(mod_amount_cb), (gpointer)p);

    g_signal_connect(G_OBJECT(p->mod2_combo),    "changed",
                        G_CALLBACK(mod_src_cb),    (gpointer)p);

    g_signal_connect(G_OBJECT(p->mod2_amount),   "value-changed",
                        G_CALLBACK(mod_amount_cb), (gpointer)p);
}


static void block(ModSectionPrivate* p)
{
    if (p->mod_only)
        goto block_mod_srcs;

    g_signal_handlers_block_by_func(p->param1, param1_cb, p);

    if (p->param == PATCH_PARAM_PITCH)
        g_signal_handlers_block_by_func(p->param2, param2_cb, p);

    g_signal_handlers_block_by_func(p->key_track, key_track_cb, p);
    g_signal_handlers_block_by_func(p->vel_sens, vel_sens_cb, p);

    if (p->param == PATCH_PARAM_AMPLITUDE)
        g_signal_handlers_block_by_func(p->env_combo,mod_src_cb, p);

block_mod_srcs:

    g_signal_handlers_block_by_func(p->mod1_combo,   mod_src_cb,    p);
    g_signal_handlers_block_by_func(p->mod1_amount,  mod_amount_cb, p);
    g_signal_handlers_block_by_func(p->mod2_combo,   mod_src_cb,    p);
    g_signal_handlers_block_by_func(p->mod2_amount,  mod_amount_cb, p);
}


static void unblock(ModSectionPrivate* p)
{
    if (p->mod_only)
        goto unblock_mod_srcs;

    g_signal_handlers_unblock_by_func(p->param1, param1_cb, p);

    if (p->param == PATCH_PARAM_PITCH)
        g_signal_handlers_unblock_by_func(p->param2, param2_cb,  p);

    g_signal_handlers_unblock_by_func(p->key_track, key_track_cb, p);
    g_signal_handlers_unblock_by_func(p->vel_sens, vel_sens_cb, p);

    if (p->param == PATCH_PARAM_AMPLITUDE)
        g_signal_handlers_unblock_by_func(p->env_combo, mod_src_cb, p);

unblock_mod_srcs:

    g_signal_handlers_unblock_by_func(p->mod1_combo, mod_src_cb,    p);
    g_signal_handlers_unblock_by_func(p->mod1_amount,mod_amount_cb, p);
    g_signal_handlers_unblock_by_func(p->mod2_combo, mod_src_cb,    p);
    g_signal_handlers_unblock_by_func(p->mod2_amount,mod_amount_cb, p);
}


static gboolean refresh(gpointer data)
{
    /* reflect certain midi cc changes of parameters in the gui */

    float v = 0.0;
    ModSection* self = MOD_SECTION(data);
    ModSectionPrivate* p = MOD_SECTION_GET_PRIVATE(self);

    if (p->patch_id < 0)
        return TRUE;

    mod_section_set_patch(self, p->patch_id);

    return TRUE;
}


static void mod_section_init(ModSection* self)
{
    ModSectionPrivate* p = MOD_SECTION_GET_PRIVATE(self);
    p->patch_id = -1;
    p->mod_only = FALSE;
    p->param = PATCH_PARAM_INVALID;
    p->refresh = -1;
}


void mod_section_set_mod_only(ModSection* self)
{
    ModSectionPrivate* p = MOD_SECTION_GET_PRIVATE(self);
    p->mod_only = TRUE;
}


void mod_section_set_param(ModSection* self, PatchParamType param)
{
    ModSectionPrivate* p = MOD_SECTION_GET_PRIVATE(self);
    GtkBox* box;
    GtkWidget* table;
    GtkTable* t;

    float range_low = 0.0;
    float range_hi = 1.0;

    const char* lstr;
    const char** param_names = names_params_get();

    int y = 0;

    int a1 = 0, a2 = 1;
    int b1 = 1, b2 = 2;
    int c1 = 2, c2 = 3;

    box = GTK_BOX(self);
    p->param = param;

    gtk_container_set_border_width(GTK_CONTAINER(self), GUI_BORDERSPACE);

    /* table */
    table = gtk_table_new(8, 3, FALSE);
    t = (GtkTable*)table;
    gui_pack(box, table);

    /* title */
    gui_attach(t, gui_title_new(param_names[param]), a1, c2, y, y + 1);
    ++y;

    /* indentation */
    gui_attach(t, gui_hpad_new(GUI_INDENT), a1, a2, y, y + 1);
    ++y;

    /* title padding */
    gui_attach(t, gui_vpad_new(GUI_TITLESPACE), b1, b2, y, y + 1);
    ++y;

    /* label column spacing (of some description!?) */
    gui_attach(t, gui_hpad_new(GUI_TEXTSPACE), c1, c2, y, y + 1);
    ++y;

    if (p->mod_only)
        goto create_mod_srcs;

    switch(param)
    {
    case PATCH_PARAM_AMPLITUDE: lstr = "Level:";    break;
    case PATCH_PARAM_PANNING:   lstr = "Position:"; break;
    case PATCH_PARAM_PITCH:     lstr = "Tuning:";   break;
                                                    break;
    default:
        lstr = param_names[param];
        break;
    }

    gui_label_attach(lstr, t, a1, a2, y, y + 1);

    if (param == PATCH_PARAM_PANNING || param == PATCH_PARAM_PITCH)
        range_low = -1.0;

    p->param1 = phat_hfan_slider_new_with_range(0.0, range_low,
                                                        range_hi,   0.1);
    gui_attach(t, p->param1, b1, b2, y, y + 1);

    if (param == PATCH_PARAM_PITCH)
    {
        p->param2 = mod_src_new_pitch_adjustment();
        gui_attach(t, p->param2, c1, c2, y, y + 1);
    }
    ++y;

    /* velocity sensitivity */
    gui_label_attach("Vel.Sens:", t, a1, a2, y, y + 1);
    p->vel_sens = phat_hfan_slider_new_with_range(0.0, 0.0, 1.0, 0.1);
    gui_attach(t, p->vel_sens, b1, b2, y, y + 1);
    ++y;

    /* key tracking */
    gui_label_attach("Key Track:", t, a1, a2, y, y + 1);
    p->key_track = phat_hfan_slider_new_with_range(0.0, -1.0, 1.0, 0.1);
    gui_attach(t, p->key_track, b1, b2, y, y + 1);
    ++y;

    if (param == PATCH_PARAM_AMPLITUDE)
    {
        /* env input source */
        gui_label_attach("Env:", t, a1, a2, y, y + 1);
        p->env_combo = mod_src_new_combo_with_cell();
        gui_attach(t, p->env_combo, b1, b2, y, y + 1);
        ++y;
    }


create_mod_srcs:


    /* mod1 input source */
    gui_label_attach("Mod1:", t, a1, a2, y, y + 1);
    p->mod1_combo = mod_src_new_combo_with_cell();
    gui_attach(t, p->mod1_combo, b1, b2, y, y + 1);

    if (param == PATCH_PARAM_PITCH)
        p->mod1_amount = mod_src_new_pitch_adjustment();
    else
        p->mod1_amount = phat_hfan_slider_new_with_range(0.0, -1.0,
                                                            1.0, 0.1);
    gui_attach(t, p->mod1_amount, c1, c2, y, y + 1);
    ++y;

    /* mod2 input source */
    gui_label_attach("Mod2:", t, a1, a2, y, y + 1);
    p->mod2_combo = mod_src_new_combo_with_cell();
    gui_attach(t, p->mod2_combo, b1, b2, y, y + 1);

    if (param == PATCH_PARAM_PITCH)
        p->mod2_amount = mod_src_new_pitch_adjustment();
    else
        p->mod2_amount = phat_hfan_slider_new_with_range(0.0, -1.0,
                                                            1.0, 0.1);
    gui_attach(t, p->mod2_amount, c1, c2, y, y + 1);

    /* done */
    connect(p);

    /*  add a timeout to allow the gui to reflect changes in
        parameters which are changeable via midi cc mesages
     */

    p->refresh = g_timeout_add(GUI_REFRESH_TIMEOUT, refresh,
                                                    (gpointer)self);
}


void mod_section_set_list_global(ModSection* self)
{
    ModSectionPrivate* p = MOD_SECTION_GET_PRIVATE(self);
    mod_src_combo_set_model(GTK_COMBO_BOX(p->env_combo),
                                    MOD_SRC_GLOBALS);

    mod_src_combo_set_model(GTK_COMBO_BOX(p->mod1_combo),
                                    MOD_SRC_GLOBALS);

    mod_src_combo_set_model(GTK_COMBO_BOX(p->mod2_combo),
                                    MOD_SRC_GLOBALS);
}


void mod_section_set_list_all(ModSection* self)
{
    ModSectionPrivate* p = MOD_SECTION_GET_PRIVATE(self);
    mod_src_combo_set_model(GTK_COMBO_BOX(p->env_combo),
                                    MOD_SRC_ALL);

    mod_src_combo_set_model(GTK_COMBO_BOX(p->mod1_combo),
                                    MOD_SRC_ALL);

    mod_src_combo_set_model(GTK_COMBO_BOX(p->mod2_combo),
                                    MOD_SRC_ALL);
}


GtkWidget* mod_section_new(void)
{
    return (GtkWidget*)g_object_new(MOD_SECTION_TYPE, NULL);
}


void mod_section_set_patch(ModSection* self, int patch_id)
{
    ModSectionPrivate* p = MOD_SECTION_GET_PRIVATE(self);
    float param1 = PATCH_PARAM_INVALID;
    float param2 = PATCH_PARAM_INVALID;
    float vsens;
    float ktrack;
    int envsrc, m1src, m2src;
    float m1amt, m2amt;

    GtkTreeIter enviter;
    GtkTreeIter m1iter;
    GtkTreeIter m2iter;

    p->patch_id = patch_id;

    if (patch_id < 0)
        return;

    if (p->mod_only)
        goto get_mod_srcs;

    patch_param_get_value(p->patch_id, p->param, &param1);

    if (p->param == PATCH_PARAM_PITCH)
        param2 = patch_get_pitch_steps(p->patch_id);

    patch_get_vel_amount(patch_id, p->param, &vsens);
    patch_get_key_amount(patch_id, p->param, &ktrack);

get_mod_srcs:

    if (p->param == PATCH_PARAM_AMPLITUDE)
    {
        patch_get_amp_env(patch_id, &envsrc);

        if (!mod_src_combo_get_iter_with_id(GTK_COMBO_BOX(p->env_combo),
                                                envsrc, &enviter))
        {
            debug("failed to get env source id from combo box\n");
        }
    }

    patch_get_mod1_src(patch_id, p->param, &m1src);
    patch_get_mod2_src(patch_id, p->param, &m2src);

    patch_get_mod1_amt(patch_id, p->param, &m1amt);
    patch_get_mod2_amt(patch_id, p->param, &m2amt);

    if (!mod_src_combo_get_iter_with_id(GTK_COMBO_BOX(p->mod1_combo),
                                                        m1src, &m1iter))
    {
        debug("failed to get mod1 source id from combo box\n");
    }

    if (!mod_src_combo_get_iter_with_id(GTK_COMBO_BOX(p->mod2_combo),
                                                        m2src, &m2iter))
    {
        debug("failed to get mod2 source id from combo box\n");
    }

    block(p);

    if (p->mod_only)
        goto set_mod_srcs;

    phat_fan_slider_set_value(PHAT_FAN_SLIDER(p->param1), param1);

    if (p->param == PATCH_PARAM_PITCH)
        phat_slider_button_set_value(PHAT_SLIDER_BUTTON(p->param2), param2);

    phat_fan_slider_set_value(PHAT_FAN_SLIDER(p->key_track), ktrack);
    phat_fan_slider_set_value(PHAT_FAN_SLIDER(p->vel_sens), vsens);

    if (p->param == PATCH_PARAM_AMPLITUDE)
        gtk_combo_box_set_active_iter(GTK_COMBO_BOX(p->env_combo),
                                                                &enviter);

set_mod_srcs:

    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(p->mod1_combo), &m1iter);
    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(p->mod2_combo), &m2iter);


    if (p->param == PATCH_PARAM_PITCH)
    {
        phat_slider_button_set_value(PHAT_SLIDER_BUTTON(p->mod1_amount),
                                            m1amt * PATCH_MAX_PITCH_STEPS);
        phat_slider_button_set_value(PHAT_SLIDER_BUTTON(p->mod2_amount),
                                            m2amt * PATCH_MAX_PITCH_STEPS);
    }
    else
    {
        phat_fan_slider_set_value(PHAT_FAN_SLIDER(p->mod1_amount), m1amt);
        phat_fan_slider_set_value(PHAT_FAN_SLIDER(p->mod2_amount), m2amt);
    }

    unblock(p);
}

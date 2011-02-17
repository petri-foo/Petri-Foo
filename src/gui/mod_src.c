#include <phat/phat.h>

#include "gui.h"
#include "mod_src.h"
#include "petri-foo.h"
#include "patch_util.h"


#include <string.h>


enum {
  COLUMN_STRING,
  COLUMN_INT,
  N_COLUMNS
};


static GtkListStore* mod_src_list_all = 0;
static GtkListStore* mod_src_list_global = 0;

static char** mod_src_names = 0;


static const char* adsr_names[] = {
    "EG1", "EG2", "EG3", "EG4", "EG5", 0
};

static const char* lfo_names[] = {
    "GLFO1", "GLFO2", "GLFO3", "GLFO4", "GLFO5",
    "VLFO1", "VLFO2", "VLFO3", "VLFO4", "VLFO5"
};

static const char* param_names[] = {
    "Amplitude",
    "Pan",
    "Cutoff",
    "Resonance",
    "Pitch",
    "Frequency Modulation"
};



static void mod_src_create_models(void)
{
    GtkTreeIter iter;

    int i;

    mod_src_list_all = gtk_list_store_new(N_COLUMNS,
                                          G_TYPE_STRING,
                                          G_TYPE_INT);

    mod_src_list_global = gtk_list_store_new(N_COLUMNS,
                                             G_TYPE_STRING,
                                             G_TYPE_INT);

    for (i = 0; i < MOD_SRC_LAST; ++i)
    {
        if (mod_src_names[i])
        {
            gtk_list_store_append ( mod_src_list_all,   &iter);
            gtk_list_store_set(     mod_src_list_all,   &iter,
                                    COLUMN_STRING,      mod_src_names[i],
                                    COLUMN_INT,         i,
                                    -1);

            if ((i >= MOD_SRC_FIRST_EG   && i <= MOD_SRC_LAST_EG)
             || (i >= MOD_SRC_FIRST_VLFO && i <= MOD_SRC_LAST_VLFO))
                continue;

            gtk_list_store_append ( mod_src_list_global, &iter);
            gtk_list_store_set(     mod_src_list_global, &iter,
                                    COLUMN_STRING,       mod_src_names[i],
                                    COLUMN_INT,          i,
                                    -1);
        }
    }
}


GtkWidget* mod_src_new_combo_with_cell()
{
    GtkWidget*          combo;
    GtkCellRenderer*    cell;

    if (!mod_src_list_all)
        mod_src_create_models();

    combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(mod_src_list_all));
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);

    cell = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), cell, TRUE);
    gtk_cell_layout_set_attributes( GTK_CELL_LAYOUT(combo),
                                    cell, "text", 0, NULL );

    return combo;
}


GtkWidget* mod_src_new_pitch_adjustment(void)
{
    GtkWidget* amt;

    amt = phat_slider_button_new_with_range(12,    -PATCH_MAX_PITCH_STEPS,
                                                    PATCH_MAX_PITCH_STEPS,
                                                    0.1, 1.0);

    phat_slider_button_set_format(PHAT_SLIDER_BUTTON(amt),
                                                    -1, NULL, "Semitones");
    phat_slider_button_set_threshold(PHAT_SLIDER_BUTTON(amt),
                                                            GUI_THRESHOLD);
    return amt;
}


gboolean mod_src_callback_helper(int patch_id,          int input_no,
                                    GtkComboBox* combo, PatchParamType par)
{
    GtkTreeIter iter;

    if (gtk_combo_box_get_active_iter(combo, &iter))
    {
        char*   string;
        int     id;

        GtkTreeModel* model = gtk_combo_box_get_model(combo);

        gtk_tree_model_get(model, &iter, 0,  &string, 1,  &id, -1);

        switch(input_no)
        {
        case MOD_ENV: patch_set_amp_env(patch_id, id);          break;
        case MOD_IN1: patch_set_mod1_src(patch_id, par, id);    break;
        case MOD_IN2: patch_set_mod2_src(patch_id, par, id);    break;
        default:
            debug("attempt to set mod src for out of range input %d.\n",
                    input_no);
            return FALSE;
        }
    }

    return TRUE;
}


gboolean        mod_src_callback_helper_lfo(int patch_id,
                                            int input_no,
                                            GtkComboBox* combo,
                                            int lfo_id)
{
    GtkTreeIter iter;

    if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combo), &iter))
    {
        char*   string;
        int     id;
        GtkTreeModel* model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));

        gtk_tree_model_get(model, &iter, 0,  &string, 1,  &id, -1);

        switch(input_no)
        {
        case MOD_IN1: patch_set_lfo_mod1_src(patch_id, lfo_id, id); break;
        case MOD_IN2: patch_set_lfo_mod2_src(patch_id, lfo_id, id); break;
        default:
            debug("attempt to set mod src for out of range input %d.\n",
                    input_no);
            return FALSE;
        }
    }

    return TRUE;
}


gboolean mod_src_combo_get_iter_with_id(GtkComboBox* combo,
                                        int mod_src_id,
                                        GtkTreeIter* iter)
{
    GtkTreeModel* model = gtk_combo_box_get_model(combo);

    gboolean valid = gtk_tree_model_get_iter_first(model, iter);

    while (valid)
    {
        gchar *str_data;
        gint   int_data;

        gtk_tree_model_get(model, iter, 0, &str_data, 1, &int_data, -1);

        if (int_data == mod_src_id)
            return TRUE;

        valid = gtk_tree_model_iter_next(model, iter);
    }

    return FALSE;
}

gboolean mod_src_combo_set_model(GtkComboBox* combo, int model_id)
{
    GtkTreeModel* model = 0;

    switch(model_id)
    {
    case MOD_SRC_INPUTS_ALL:
        model = GTK_TREE_MODEL(mod_src_list_all);
        break;

    case MOD_SRC_INPUTS_GLOBAL:
        model = GTK_TREE_MODEL(mod_src_list_global);
        break;

    default:
        debug("unknown mod src model.\n");
        return FALSE;
    }

    gtk_combo_box_set_model(combo, model);

    return TRUE;
}


int mod_src_combo_get_model_id(GtkComboBox* combo)
{
    GtkTreeModel* model = gtk_combo_box_get_model(combo);

    if (model == GTK_TREE_MODEL(mod_src_list_all))
        return MOD_SRC_INPUTS_ALL;
    else if (model == GTK_TREE_MODEL(mod_src_list_global))
        return MOD_SRC_INPUTS_GLOBAL;

    return MOD_SRC_INPUTS_UNKNOWN;
}


void mod_src_create_names(void)
{
    const char none[] = "OFF";
    const char one[] = "1.0";

    int i;
    int id;

    /* check for mismatched counts etc: */
    if (!mod_src_adsr_names() || !mod_src_lfo_names())
    {
        debug("*** PROBLEM OF DEATH IS FORECAST ***\n");
        return;
    }

    mod_src_names = malloc(sizeof(*mod_src_names) * MOD_SRC_LAST);

    for (i = 0; i < MOD_SRC_LAST; ++i)
        mod_src_names[i] = 0;

    mod_src_names[MOD_SRC_NONE] = malloc(strlen(none) + 1);
    strcpy(mod_src_names[MOD_SRC_NONE], none);
    mod_src_names[MOD_SRC_ONE] = malloc(strlen(one) + 1);
    strcpy(mod_src_names[MOD_SRC_ONE], one);

    for (i = MOD_SRC_FIRST_EG; i < MOD_SRC_LAST_EG; ++i)
    {
        id = i - MOD_SRC_FIRST_EG;
        if (adsr_names[id])
        {
            mod_src_names[i] = malloc(strlen(adsr_names[id]) + 1);
            strcpy(mod_src_names[i], adsr_names[id]);
        }
        else
        {
            debug("adsr_names mismatch adsr count\n");
            break;
        }
    }

    for (i = MOD_SRC_FIRST_GLFO; i < MOD_SRC_LAST_GLFO; ++i)
    {
        id = i - MOD_SRC_FIRST_GLFO;
        mod_src_names[i] = malloc(strlen(lfo_names[id]) + 1);
        strcpy(mod_src_names[i], lfo_names[id]);
    }

    for (i = MOD_SRC_FIRST_VLFO; i < MOD_SRC_LAST_VLFO; ++i)
    {
        id = (MOD_SRC_LAST_GLFO - MOD_SRC_FIRST_GLFO)
             + (i - MOD_SRC_FIRST_VLFO);
        mod_src_names[i] = malloc(strlen(lfo_names[id]) + 1);
        strcpy(mod_src_names[i], lfo_names[id]);
    }
}


void mod_src_destroy_names(void)
{
    if (mod_src_names)
    {
        int i;

        for (i = 0; i < MOD_SRC_LAST; ++i)
            if (mod_src_names[i])
                free(mod_src_names[i]);

        free(mod_src_names);
    }
}


char** mod_src_get_names(void)
{
    if (!mod_src_names)
    {
        debug("returns NULL\n");
    }
    return mod_src_names;
}


const char** mod_src_adsr_names(void)
{
    int i;

    for (i = 0; adsr_names[i] != 0; ++i);

    if (i != VOICE_MAX_ENVS)
    {
        debug(  "Friendly warning to the programmer:\n"
                "You've either changed the enum value for VOICE_MAX_ENVS\n"
                "Or you've changed the list of ADSR names\n"
                "In either case it's broken now. Please fix!\n");
        return 0;
    }

    return adsr_names;
}

const char** mod_src_lfo_names(void)
{
    int i;

    for (i = 0; lfo_names[i] != 0; ++i);

    if (i != TOTAL_LFOS)
    {
        debug(  "Friendly warning to the programmer:\n"
                "You've either changed the enum value for PATCH_MAX_LFOS\n"
                "and/or ther enum value VOICE_MAX_LFOS\n"
                "Or you've changed the list of LFO names\n"
                "In either case it's broken now. Please fix!\n");
        return 0;
    }

    return lfo_names;
}


const char** mod_src_param_names(void)
{
    return param_names;
}


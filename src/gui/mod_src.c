#include <phat/phat.h>

#include "gui.h"
#include "mod_src.h"
#include "patch.h"
#include "petri-foo.h"


enum {
  COLUMN_STRING,
  COLUMN_INT,
  N_COLUMNS
};


static GtkListStore* mod_src_list_all = 0;
static GtkListStore* mod_src_list_global = 0;


static void mod_src_create_models(void)
{
    GtkTreeIter iter;

    debug("here\n");

    int i;
    char** mod_src_names = patch_mod_source_names();

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

    debug("here\n");

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

    debug("here\n");

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

    debug("here\n");

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

    debug("here\n");

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

    debug("here\n");

    switch(model_id)
    {
    case MOD_SRC_INPUTS_ALL:
        debug("MOD_SRC_INPUTS_ALL\n");
        model = GTK_TREE_MODEL(mod_src_list_all);
        break;

    case MOD_SRC_INPUTS_GLOBAL:
        debug("MOD_SRC_INPUTS_GLOBAL\n");
        model = GTK_TREE_MODEL(mod_src_list_global);
        break;

    default:
        debug("glorious fail\n");
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



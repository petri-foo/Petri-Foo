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


static GtkListStore* mod_src_list = 0;


GtkTreeModel* mod_src_tree_model(void)
{
    if (mod_src_list)
        return GTK_TREE_MODEL(mod_src_list);

    GtkTreeIter iter;

    int i;
    char** mod_src_names = patch_mod_source_names();

    mod_src_list = gtk_list_store_new(  N_COLUMNS,
                                        G_TYPE_STRING,
                                        G_TYPE_INT);

    for (i = 0; i < MOD_SRC_LAST; ++i)
    {
        if (mod_src_names[i])
        {
            gtk_list_store_append ( mod_src_list, &iter);
            gtk_list_store_set(     mod_src_list, &iter,
                                    COLUMN_STRING,  mod_src_names[i],
                                    COLUMN_INT,     i,
                                    -1);
        }
    }

    return GTK_TREE_MODEL(mod_src_list);
}


GtkWidget* mod_src_new_combo_with_cell(void)
{
    GtkWidget*          combo;
    GtkCellRenderer*    cell;

    if (!mod_src_list)
        mod_src_tree_model();

    combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(mod_src_list));
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
                                 GtkTreeModel* model,   GtkComboBox* combo,
                                                        PatchParamType par)
{
    GtkTreeIter iter;

    if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combo), &iter))
    {
        char*   string;
        int     id;

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


gboolean mod_src_get_tree_iter_from_id(GtkTreeModel* model,
                                                int mod_src_id,
                                                GtkTreeIter* iter)
{
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


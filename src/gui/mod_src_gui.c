#include "mod_src_gui.h"
#include <phat/phat.h>

#include "gui.h"
#include "petri-foo.h"
#include "patch_set_and_get.h"
#include "mod_src.h"

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

    id_name* mod_src_ids;
    id_name* msp;

    mod_src_list_all = gtk_list_store_new(N_COLUMNS,
                                          G_TYPE_STRING,
                                          G_TYPE_INT);

    mod_src_list_global = gtk_list_store_new(N_COLUMNS,
                                             G_TYPE_STRING,
                                             G_TYPE_INT);

    mod_src_ids = mod_src_get(MOD_SRC_ALL);

    for (msp = mod_src_ids; msp->name; ++msp)
    {
/*
        debug("mod_src:%d %s\n", msp->id, msp->name);
 */
        gtk_list_store_append ( mod_src_list_all,   &iter);
        gtk_list_store_set(     mod_src_list_all,   &iter,
                                COLUMN_STRING,      msp->name,
                                COLUMN_INT,         msp->id,
                                -1);

        if (mod_src_is_global(msp->id))
        {
            gtk_list_store_append ( mod_src_list_global,&iter);
            gtk_list_store_set(     mod_src_list_global,&iter,
                                    COLUMN_STRING,      msp->name,
                                    COLUMN_INT,         msp->id,
                                    -1);
        }
    }

    mod_src_free(mod_src_ids);
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
                                                    -1, NULL, NULL);
    gtk_widget_set_tooltip_text(amt, "Semitones");
    phat_slider_button_set_threshold(PHAT_SLIDER_BUTTON(amt),
                                                            GUI_THRESHOLD);
    return amt;
}


gboolean mod_src_callback_helper(int patch_id,          int slot,
                                    GtkComboBox* combo, PatchParamType par)
{
    GtkTreeIter iter;

    if (gtk_combo_box_get_active_iter(combo, &iter))
    {
        char*   string;
        int     id;

        GtkTreeModel* model = gtk_combo_box_get_model(combo);

        gtk_tree_model_get(model, &iter, 0,  &string, 1,  &id, -1);

        debug("patch id:%d slot:%d mod src:%s (%d)\n",
               patch_id,   slot, mod_src_name(id), id);

        /* FIXME: probably should check return value */
        patch_set_mod_src(patch_id, par, slot, id);
    }

    return TRUE;
}


gboolean mod_src_callback_helper_lfo(int patch_id,
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
        case FM1:   patch_set_lfo_fm1_src(patch_id, lfo_id, id); break;
        case FM2:   patch_set_lfo_fm2_src(patch_id, lfo_id, id); break;
        case AM1:   patch_set_lfo_am1_src(patch_id, lfo_id, id); break;
        case AM2:   patch_set_lfo_am2_src(patch_id, lfo_id, id); break;
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
    case MOD_SRC_ALL:
        model = GTK_TREE_MODEL(mod_src_list_all);
        break;

    case MOD_SRC_GLOBALS:
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
        return MOD_SRC_ALL;
    else if (model == GTK_TREE_MODEL(mod_src_list_global))
        return MOD_SRC_GLOBALS;

    return MOD_SRC_NONE;
}


/*  Petri-Foo is a fork of the Specimen audio sampler.

    Copyright 2011 James W. Morris

    This file is part of Petri-Foo.

    Petri-Foo is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.

    Petri-Foo is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Petri-Foo.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "basic_combos.h"
#include "petri-foo.h"


enum {
    COLUMN_STRING,
    COLUMN_INT,
    N_COLUMNS
};


GtkWidget* basic_combo_create(const char* str_array[])
{
    int i;
    GtkTreeIter iter;
    GtkListStore* list;
    GtkWidget* combo;
    GtkCellRenderer* cell;

    list = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_INT);

    for (i = 0; str_array[i] != 0; ++i)
    {
        gtk_list_store_append(list, &iter);

        gtk_list_store_set( list,           &iter,
                            COLUMN_STRING,  str_array[i],
                            COLUMN_INT,     i,
                            -1);
    }

    combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(list));
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);

    cell = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), cell, TRUE);
    gtk_cell_layout_set_attributes( GTK_CELL_LAYOUT(combo),
                                    cell, "text", 0, NULL );
    return combo;
}


GtkWidget* basic_combo_id_name_create(const id_name const* idnames)
{
    int i;
    GtkTreeIter iter;
    GtkListStore* list;
    GtkWidget* combo;
    GtkCellRenderer* cell;

    list = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_INT);

    for (i = 0; idnames[i].name != 0; ++i)
    {
        gtk_list_store_append(list, &iter);

        gtk_list_store_set( list,           &iter,
                            COLUMN_STRING,  idnames[i].name,
                            COLUMN_INT,     idnames[i].id,
                            -1);
    }

    combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(list));
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);

    cell = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), cell, TRUE);
    gtk_cell_layout_set_attributes( GTK_CELL_LAYOUT(combo),
                                    cell, "text", 0, NULL );
    return combo;
}


int basic_combo_get_active(GtkWidget* combo)
{
    GtkTreeIter iter;
    GtkTreeModel* model;

    model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));

    if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combo), &iter))
    {
        char* str;
        int index;

        gtk_tree_model_get(GTK_TREE_MODEL(model), &iter,
                                    COLUMN_STRING, &str,
                                    COLUMN_INT,    &index, -1);
        return index;
    }

    return -1;
}


gboolean
basic_combo_get_iter_at_index(GtkWidget* combo, int index,
                                                GtkTreeIter* iter)
{
    GtkTreeModel* model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
    gboolean valid = gtk_tree_model_get_iter_first(model, iter);

    while(valid)
    {
        char *str;
        int   opt;

        gtk_tree_model_get(model, iter, COLUMN_STRING, &str,
                                        COLUMN_INT,    &opt, -1);
        if (opt == index)
            return TRUE;

        valid = gtk_tree_model_iter_next(model, iter);
    }

    return FALSE;
}

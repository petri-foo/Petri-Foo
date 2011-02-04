#include "mod_src_opt.h"
#include "patch.h"

enum {
  COLUMN_STRING,
  COLUMN_INT,
  N_COLUMNS
};


GtkTreeModel* mod_src_opt_new(void)
{
    GtkListStore* list;
    GtkTreeIter iter;

    int i;
    char** mod_src_names = patch_mod_source_names();

    list = gtk_list_store_new(  N_COLUMNS,
                                G_TYPE_STRING,
                                G_TYPE_INT);

    for (i = 0; i < MOD_SRC_LAST; ++i)
    {
        if (mod_src_names[i])
        {
            gtk_list_store_append ( list, &iter);
            gtk_list_store_set(     list, &iter,
                                    COLUMN_STRING,  mod_src_names[i],
                                    COLUMN_INT,     i,
                                    -1);
        }
    }

    return list;
}


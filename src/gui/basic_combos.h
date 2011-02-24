#ifndef BASIC_COMBOS_H
#define BASIC_COMBOS_H


#include <gtk/gtk.h>


GtkWidget*  basic_combo_create(const char* str_array[]);
int         basic_combo_get_active(GtkWidget*);
gboolean    basic_combo_get_iter_at_index(GtkWidget*, int index,
                                                GtkTreeIter* result);


#endif /* BASIC_COMBOS_H */

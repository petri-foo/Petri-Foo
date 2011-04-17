#ifndef BASIC_COMBOS_H
#define BASIC_COMBOS_H


#include "names.h"

#include <gtk/gtk.h>


/*  create a basic GTK combo button using one of two methods. both methods
 *  give the same end result to the user: the ability to select one from
 *  a list of strings. invisible to the user, each string has an ID:
 *      basic_combo_create:
 *          *assigns* each string an ID based upon its array index.
 *      basic_combo_id_name_create:
 *          the ID is *specified* within the array of struct _id_name.
 */
GtkWidget*  basic_combo_create(const char* str_array[]);
GtkWidget*  basic_combo_id_name_create(const id_name const*);


/*  basic_combo_get_active works for both of above - it returns the 
 *  specified ID within id_name, or the assigned ID of str_array.
 */
int         basic_combo_get_active(GtkWidget*);


/*  basic_combo_get_iter_at_index matches on the specified ID within
 *  id_name or the assigned ID of str_array.
 */
gboolean    basic_combo_get_iter_at_index(GtkWidget*, int index,
                                                GtkTreeIter* result);


#endif /* BASIC_COMBOS_H */

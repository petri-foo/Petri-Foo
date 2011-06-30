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

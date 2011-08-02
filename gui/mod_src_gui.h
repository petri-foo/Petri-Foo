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


#ifndef MOD_SRC_GUI_H
#define MOD_SRC_GUI_H

#include <gtk/gtk.h>

#include "patch.h"

enum {
    /* LFO inputs */
    FM1, FM2, AM1, AM2
};



GtkWidget*      mod_src_new_combo_with_cell();

/*  mod_src_new_pitch_adjustment
        creates a phin slider button with semitones label within.
*/
GtkWidget*      mod_src_new_pitch_adjustment(void);


int mod_src_combo_get_mod_src_id(GtkComboBox* combo);


/*  mod_src_callback_helper

        for use in the mod src combo box callback, it reads the
        combo box value and sets the patch accordingly.
*/

gboolean        mod_src_callback_helper(int patch_id,
                                        int slot,
                                        GtkComboBox* combo,
                                        PatchParamType par);

gboolean        mod_src_callback_helper_lfo(int patch_id,
                                            int lfo_input,
                                            GtkComboBox* combo,
                                            int lfo_id);

/*  mod_src_combo_get_iter_with_id
        searches the GtkTreeModel combo is using for GtkTreeIter
        with mod_src_id value.
*/
gboolean        mod_src_combo_get_iter_with_id( GtkComboBox*,
                                                int mod_src_id,
                                                GtkTreeIter*);

gboolean        mod_src_combo_set_model(GtkComboBox*, int model_id);

int             mod_src_combo_get_model_id(GtkComboBox*);


#endif

#ifndef MOD_SRC_H
#define MOD_SRC_H

#include <gtk/gtk.h>

#include "patch.h"

enum {
    MOD_ENV = 0,
    MOD_IN1,
    MOD_IN2
};


GtkTreeModel*   mod_src_tree_model(void);


GtkWidget*      mod_src_new_combo_with_cell(void);

/*  mod_src_new_pitch_adjustment
        creates a phat slider button with semitones label within.
*/
GtkWidget*      mod_src_new_pitch_adjustment(void);


/*  mod_src_callback_helper

        for use in the mod src combo box callback, it reads the
        combo box value and sets the patch accordingly.

        valid values for input_no are 1 or 2 (ie mod1 and mod2).
*/
gboolean        mod_src_callback_helper(int patch_id,
                                        int input,
                                        GtkTreeModel* model,
                                        GtkComboBox* combo,
                                        PatchParamType par);

gboolean        mod_src_get_tree_iter_from_id(  GtkTreeModel* model,
                                                int mod_src_id,
                                                GtkTreeIter*);

#endif

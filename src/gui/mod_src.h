#ifndef MOD_SRC_H
#define MOD_SRC_H

#include <gtk/gtk.h>

#include "patch.h"

enum {
    MOD_ENV = 0,
    MOD_IN1,
    MOD_IN2
};

enum {
    /* mod src combo model IDs: */
    MOD_SRC_INPUTS_UNKNOWN =    -1,
    MOD_SRC_INPUTS_ALL =        0,
    MOD_SRC_INPUTS_GLOBAL =     1
};


GtkWidget*      mod_src_new_combo_with_cell();

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
                                        GtkComboBox* combo,
                                        PatchParamType par);

gboolean        mod_src_callback_helper_lfo(int patch_id,
                                            int input,
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


void            mod_src_create_names(void);
void            mod_src_destroy_names(void);

char**          mod_src_get_names(void);

const char**    mod_src_adsr_names(void);
const char**    mod_src_lfo_names(void);
const char**    mod_src_param_names(void);

#endif

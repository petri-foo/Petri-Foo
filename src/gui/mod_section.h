#ifndef __MOD_SECTION__
#define __MOD_SECTION__

#include <gtk/gtk.h>


#include "patch.h"


G_BEGIN_DECLS

#define MOD_SECTION_TYPE \
    (mod_section_get_type())

#define MOD_SECTION(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), MOD_SECTION_TYPE, ModSection))

#define MOD_SECTION_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), MOD_SECTION_TYPE, ModSectionClass))

#define IS_MOD_SECTION(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MOD_SECTION_TYPE))

#define IS_MOD_SECTION_CLASS(klass) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((klass), MOD_SECTION_TYPE))


typedef struct _ModSectionClass ModSectionClass;
typedef struct _ModSection ModSection;


struct _ModSection
{
    GtkVBox         parent;

    /*< private >*/
    int             patch_id;
    int             refresh;
    gboolean        mod_only;

    PatchParamType  param;

    GtkWidget*      param1;
    GtkWidget*      param2;

    GtkWidget*      env_combo;
    GtkWidget*      vel_sens;

    GtkWidget*      mod1_combo;
    GtkWidget*      mod1_amount;

    GtkWidget*      mod2_combo;
    GtkWidget*      mod2_amount;
};


struct _ModSectionClass
{
    GtkVBoxClass parent_class;

    /*< private >*/
};


GType       mod_section_get_type(void);

GtkWidget*  mod_section_new(void);
void        mod_section_set_mod_only(ModSection*);
void        mod_section_set_param(ModSection*, PatchParamType);

void        mod_section_set_lfo_id(ModSection*, int lfo_id);
void        mod_section_set_list_global(ModSection*);
void        mod_section_set_list_all(ModSection*);

void        mod_section_set_patch(ModSection*, int patch_id);


G_END_DECLS


#endif /* __MOD_SECTION__ */

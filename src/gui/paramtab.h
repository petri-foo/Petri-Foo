#ifndef __PARAM_TAB__
#define __PARAM_TAB__

#include <gtk/gtk.h>

#include "patch.h"

G_BEGIN_DECLS

#define PARAM_TAB_TYPE \
    (param_tab_get_type())

#define PARAM_TAB(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), PARAM_TAB_TYPE, ParamTab))

#define IS_PARAM_TAB(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PARAM_TAB_TYPE))

#define PARAM_TAB_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), PARAM_TAB_TYPE, ParamTabClass))

#define IS_PARAM_TAB_CLASS(klass) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((klass), PARAM_TAB_TYPE))


typedef struct _ParamTabClass ParamTabClass;
typedef struct _ParamTab ParamTab;


struct _ParamTab
{
    GtkVBox     parent_instance;
};


struct _ParamTabClass
{
    GtkVBoxClass parent_class;
};


GType       param_tab_get_type(void);
GtkWidget*  param_tab_new(void);

void        param_tab_set_param(ParamTab*, PatchParamType);
void        param_tab_set_patch(ParamTab*, int patch);


G_END_DECLS


#endif /* __PARAM_TAB__ */

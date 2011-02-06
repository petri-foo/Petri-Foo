#ifndef __AMP_TAB__
#define __AMP_TAB__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define AMP_TAB_TYPE \
    (amp_tab_get_type())

#define AMP_TAB(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), AMP_TAB_TYPE, AmpTab))

#define AMP_TAB_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), AMP_TAB_TYPE, AmpTabClass))

#define IS_AMP_TAB(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AMP_TAB_TYPE))

#define IS_AMP_TAB_CLASS(klass) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((klass), AMP_TAB_TYPE))

typedef struct _AmpTabClass AmpTabClass;
typedef struct _AmpTab AmpTab;

struct _AmpTab
{
    GtkVBox     parent;

    /*< private >*/
    int         patch;
    guint       refresh;

    GtkWidget*  amp;
    GtkWidget*  pan;
};

struct _AmpTabClass
{
    GtkVBoxClass parent_class;

    /*< private >*/
};

GType amp_tab_get_type(void);
GtkWidget* amp_tab_new(void);
void amp_tab_set_patch(AmpTab* self, int patch);

G_END_DECLS

#endif /* __AMP_TAB__ */

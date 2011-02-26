#ifndef __FILTER_TAB__
#define __FILTER_TAB__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define FILTER_TAB_TYPE             (filter_tab_get_type())
#define FILTER_TAB(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                    FILTER_TAB_TYPE, FilterTab))

#define IS_FILTER_TAB(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                    FILTER_TAB_TYPE))

#define FILTER_TAB_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                    FILTER_TAB_TYPE, FilterTabClass))

#define IS_FILTER_TAB_CLASS(klass)  (G_TYPE_CHECK_INSTANCE_TYPE ((klass),\
                                    FILTER_TAB_TYPE))


typedef struct _FilterTabClass FilterTabClass;
typedef struct _FilterTab FilterTab;


struct _FilterTab
{
    GtkVBox parent_instance;
};


struct _FilterTabClass
{
    GtkVBoxClass parent_class;
};


GType       filter_tab_get_type(void);
GtkWidget*  filter_tab_new(void);

void        filter_tab_set_patch(FilterTab* self, int patch);


G_END_DECLS


#endif /* __FILTER_TAB__ */

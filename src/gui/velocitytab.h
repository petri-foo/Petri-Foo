#ifndef __VELOCITY_TAB__
#define __VELOCITY_TAB__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define VELOCITY_TAB_TYPE \
    (velocity_tab_get_type())

#define VELOCITY_TAB(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VELOCITY_TAB_TYPE, VelocityTab))

#define IS_VELOCITY_TAB(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VELOCITY_TAB_TYPE))

#define VELOCITY_TAB_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), VELOCITY_TAB_TYPE, VelocityTabClass))

#define IS_VELOCITY_TAB_CLASS(klass) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((klass), VELOCITY_TAB_TYPE))


typedef struct _VelocityTabClass VelocityTabClass;
typedef struct _VelocityTab      VelocityTab;


struct _VelocityTab
{
    GtkVBox parent;
};

struct _VelocityTabClass
{
    GtkVBoxClass parent_class;
};


GType       velocity_tab_get_type(void);
GtkWidget*  velocity_tab_new(void);

void        velocity_tab_set_patch(VelocityTab* self, int patch);


G_END_DECLS


#endif /* __VELOCITY_TAB__ */

#ifndef __PHIN_VKEYBOARD__
#define __PHIN_VKEYBOARD__

#include <gtk/gtk.h>

#include "phinkeyboard.h"

G_BEGIN_DECLS

#define PHIN_TYPE_VKEYBOARD (phin_vkeyboard_get_type())
#define PHIN_VKEYBOARD(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PHIN_TYPE_VKEYBOARD, PhinVKeyboard))
#define PHIN_VKEYBOARD_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), PHIN_TYPE_VKEYBOARD, PhinVKeyboardClass))
#define PHIN_IS_VKEYBOARD(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PHIN_TYPE_VKEYBOARD))
#define PHIN_IS_VKEYBOARD_CLASS(klass) (G_TYPE_CHECK_INSTANCE_TYPE ((klass), PHIN_TYPE_VKEYBOARD))

typedef struct _PhinVKeyboardClass PhinVKeyboardClass;
typedef struct _PhinVKeyboard PhinVKeyboard;

struct _PhinVKeyboard
{
    /*< private >*/
    PhinKeyboard parent;
};

struct _PhinVKeyboardClass
{
    /*< private >*/
    PhinKeyboardClass parent_class;
};

GType phin_vkeyboard_get_type(void);
GtkWidget* phin_vkeyboard_new(GtkAdjustment* adjustment, int numkeys, gboolean show_labels);

G_END_DECLS

#endif /* __PHIN_VKEYBOARD__ */

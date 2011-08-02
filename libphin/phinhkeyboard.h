#ifndef __PHIN_HKEYBOARD__
#define __PHIN_HKEYBOARD__

#include <gtk/gtk.h>

#include "phinkeyboard.h"


G_BEGIN_DECLS

#define PHIN_TYPE_HKEYBOARD (phin_hkeyboard_get_type())
#define PHIN_HKEYBOARD(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PHIN_TYPE_HKEYBOARD, PhinHKeyboard))
#define PHIN_HKEYBOARD_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), PHIN_TYPE_HKEYBOARD, PhinHKeyboardClass))
#define PHIN_IS_HKEYBOARD(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PHIN_TYPE_HKEYBOARD))
#define PHIN_IS_HKEYBOARD_CLASS(klass) (G_TYPE_CHECK_INSTANCE_TYPE ((klass), PHIN_TYPE_HKEYBOARD))

typedef struct _PhinHKeyboardClass PhinHKeyboardClass;
typedef struct _PhinHKeyboard PhinHKeyboard;

struct _PhinHKeyboard
{
    /*< private >*/
    PhinKeyboard parent;
};

struct _PhinHKeyboardClass
{
    /*< private >*/
    PhinKeyboardClass parent_class;
};

GType phin_hkeyboard_get_type(void);
GtkWidget* phin_hkeyboard_new(GtkAdjustment* adjustment, int numkeys, gboolean show_labels);

G_END_DECLS

#endif /* __PHIN_HKEYBOARD__ */

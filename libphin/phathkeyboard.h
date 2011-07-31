#ifndef __PHAT_HKEYBOARD__
#define __PHAT_HKEYBOARD__

#include <gtk/gtk.h>
#include <phat/phatkeyboard.h>

G_BEGIN_DECLS

#define PHAT_TYPE_HKEYBOARD (phat_hkeyboard_get_type())
#define PHAT_HKEYBOARD(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PHAT_TYPE_HKEYBOARD, PhatHKeyboard))
#define PHAT_HKEYBOARD_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), PHAT_TYPE_HKEYBOARD, PhatHKeyboardClass))
#define PHAT_IS_HKEYBOARD(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PHAT_TYPE_HKEYBOARD))
#define PHAT_IS_HKEYBOARD_CLASS(klass) (G_TYPE_CHECK_INSTANCE_TYPE ((klass), PHAT_TYPE_HKEYBOARD))

typedef struct _PhatHKeyboardClass PhatHKeyboardClass;
typedef struct _PhatHKeyboard PhatHKeyboard;

struct _PhatHKeyboard
{
    /*< private >*/
    PhatKeyboard parent;
};

struct _PhatHKeyboardClass
{
    /*< private >*/
    PhatKeyboardClass parent_class;
};

GType phat_hkeyboard_get_type(void);
GtkWidget* phat_hkeyboard_new(GtkAdjustment* adjustment, int numkeys, gboolean show_labels);

G_END_DECLS

#endif /* __PHAT_HKEYBOARD__ */

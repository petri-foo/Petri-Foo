#ifndef __PHAT_VKEYBOARD__
#define __PHAT_VKEYBOARD__

#include <gtk/gtk.h>
#include <phat/phatkeyboard.h>

G_BEGIN_DECLS

#define PHAT_TYPE_VKEYBOARD (phat_vkeyboard_get_type())
#define PHAT_VKEYBOARD(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PHAT_TYPE_VKEYBOARD, PhatVKeyboard))
#define PHAT_VKEYBOARD_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), PHAT_TYPE_VKEYBOARD, PhatVKeyboardClass))
#define PHAT_IS_VKEYBOARD(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PHAT_TYPE_VKEYBOARD))
#define PHAT_IS_VKEYBOARD_CLASS(klass) (G_TYPE_CHECK_INSTANCE_TYPE ((klass), PHAT_TYPE_VKEYBOARD))

typedef struct _PhatVKeyboardClass PhatVKeyboardClass;
typedef struct _PhatVKeyboard PhatVKeyboard;

struct _PhatVKeyboard
{
    /*< private >*/
    PhatKeyboard parent;
};

struct _PhatVKeyboardClass
{
    /*< private >*/
    PhatKeyboardClass parent_class;
};

GType phat_vkeyboard_get_type(void);
GtkWidget* phat_vkeyboard_new(GtkAdjustment* adjustment, int numkeys, gboolean show_labels);

G_END_DECLS

#endif /* __PHAT_VKEYBOARD__ */

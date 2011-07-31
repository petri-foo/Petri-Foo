#ifndef __PHAT_KEYBOARD__
#define __PHAT_KEYBOARD__

#include <gtk/gtk.h>
#include <libgnomecanvas/libgnomecanvas.h>

G_BEGIN_DECLS

#define PHAT_TYPE_KEYBOARD (phat_keyboard_get_type())
#define PHAT_KEYBOARD(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PHAT_TYPE_KEYBOARD, PhatKeyboard))
#define PHAT_KEYBOARD_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), PHAT_TYPE_KEYBOARD, PhatKeyboardClass))
#define PHAT_IS_KEYBOARD(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PHAT_TYPE_KEYBOARD))
#define PHAT_IS_KEYBOARD_CLASS(klass) (G_TYPE_CHECK_INSTANCE_TYPE ((klass), PHAT_TYPE_KEYBOARD))

typedef struct _PhatKeyboardClass PhatKeyboardClass;
typedef struct _PhatKeyboard PhatKeyboard;
typedef struct __Key _Key;

/* key dimensions */
enum
{
    PHAT_KEYBOARD_KEY_WIDTH  = 13,
    PHAT_KEYBOARD_KEY_LENGTH = 33,
};


struct __Key
{
    int index;
    PhatKeyboard* keyboard;     /* the keyboard we belong to */
    GnomeCanvasGroup* group;    /* the group this key belongs to */
    GnomeCanvasItem* pre;       /* prelight rectangle */
    GnomeCanvasItem* on;        /* active (depressed) rectangle */
    GnomeCanvasItem* shad;      /* active shadow */
};

struct _PhatKeyboard
{
    /*< private >*/
    GtkViewport parent;

    _Key *keys;
    int nkeys;
    int label;
    GnomeCanvas* canvas;
    GtkOrientation orientation;
};

struct _PhatKeyboardClass
{
    /*< private >*/
    GtkViewportClass parent_class;

    void (*key_pressed)(PhatKeyboard* keyboard, int key);
    void (*key_released)(PhatKeyboard* keyboard, int key);
};

GType phat_keyboard_get_type(void);
GtkAdjustment* phat_keyboard_get_adjustment(PhatKeyboard* keyboard);
void phat_keyboard_set_adjustment(PhatKeyboard* keyboard, GtkAdjustment* adjustment);

G_END_DECLS

#endif /* __PHAT_KEYBOARD__ */

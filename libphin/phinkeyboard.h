#ifndef __PHIN_KEYBOARD__
#define __PHIN_KEYBOARD__

#include <gtk/gtk.h>
#include <libgnomecanvas/libgnomecanvas.h>

G_BEGIN_DECLS

#define PHIN_TYPE_KEYBOARD (phin_keyboard_get_type())
#define PHIN_KEYBOARD(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PHIN_TYPE_KEYBOARD, PhinKeyboard))
#define PHIN_KEYBOARD_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), PHIN_TYPE_KEYBOARD, PhinKeyboardClass))
#define PHIN_IS_KEYBOARD(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PHIN_TYPE_KEYBOARD))
#define PHIN_IS_KEYBOARD_CLASS(klass) (G_TYPE_CHECK_INSTANCE_TYPE ((klass), PHIN_TYPE_KEYBOARD))

typedef struct _PhinKeyboardClass PhinKeyboardClass;
typedef struct _PhinKeyboard PhinKeyboard;
typedef struct __Key _Key;

/* key dimensions */
enum
{
    PHIN_KEYBOARD_KEY_WIDTH  = 13,
    PHIN_KEYBOARD_KEY_LENGTH = 33,
};


struct __Key
{
    int index;
    PhinKeyboard* keyboard;     /* the keyboard we belong to */
    GnomeCanvasGroup* group;    /* the group this key belongs to */
    GnomeCanvasItem* pre;       /* prelight rectangle */
    GnomeCanvasItem* on;        /* active (depressed) rectangle */
    GnomeCanvasItem* shad;      /* active shadow */
};

struct _PhinKeyboard
{
    /*< private >*/
    GtkViewport parent;

    _Key *keys;
    int nkeys;
    int label;
    GnomeCanvas* canvas;
    GtkOrientation orientation;
};

struct _PhinKeyboardClass
{
    /*< private >*/
    GtkViewportClass parent_class;

    void (*key_pressed)(PhinKeyboard* keyboard, int key);
    void (*key_released)(PhinKeyboard* keyboard, int key);
};

GType phin_keyboard_get_type(void);
GtkAdjustment* phin_keyboard_get_adjustment(PhinKeyboard* keyboard);
void phin_keyboard_set_adjustment(PhinKeyboard* keyboard, GtkAdjustment* adjustment);

G_END_DECLS

#endif /* __PHIN_KEYBOARD__ */

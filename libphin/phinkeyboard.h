/*  Phin is a fork of the PHAT Audio Toolkit.
    Phin is part of Petri-Foo. Petri-Foo is a fork of Specimen.

    Original author Pete Bessman
    Copyright 2005 Pete Bessman
    Copyright 2011 James W. Morris

    This file is part of Phin.

    Phin is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.

    Phin is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Phin.  If not, see <http://www.gnu.org/licenses/>.

    This file is a derivative of a PHAT original, modified 2011
*/
#ifndef __PHIN_KEYBOARD__
#define __PHIN_KEYBOARD__

#include <gtk/gtk.h>
#include <libgnomecanvas/libgnomecanvas.h>

G_BEGIN_DECLS

#define PHIN_TYPE_KEYBOARD          (phin_keyboard_get_type())

#define PHIN_KEYBOARD(obj)          (G_TYPE_CHECK_INSTANCE_CAST((obj), \
                                    PHIN_TYPE_KEYBOARD, PhinKeyboard))

#define PHIN_KEYBOARD_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST((klass), \
                                    PHIN_TYPE_KEYBOARD, PhinKeyboardClass))

#define PHIN_IS_KEYBOARD(obj)       (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
                                    PHIN_TYPE_KEYBOARD))

#define PHIN_IS_KEYBOARD_CLASS(klass) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((klass), PHIN_TYPE_KEYBOARD))


typedef struct _PhinKeyboard        PhinKeyboard;
typedef struct _PhinKeyboardClass   PhinKeyboardClass;


/* key dimensions */
enum
{
    PHIN_KEYBOARD_KEY_WIDTH  = 13,
    PHIN_KEYBOARD_KEY_LENGTH = 33,
};


struct _PhinKeyboard
{
    GtkViewport parent;
};


struct _PhinKeyboardClass
{
    GtkViewportClass parent_class;
    /*< private >*/
    void (*key_pressed)(PhinKeyboard* keyboard, int key);
    void (*key_released)(PhinKeyboard* keyboard, int key);
};


GType           phin_keyboard_get_type      (void);
GtkAdjustment*  phin_keyboard_get_adjustment(PhinKeyboard*);
void            phin_keyboard_set_adjustment(PhinKeyboard*, GtkAdjustment*); 

G_END_DECLS

#endif /* __PHIN_KEYBOARD__ */

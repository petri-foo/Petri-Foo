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
#ifndef __PHIN_VKEYBOARD__
#define __PHIN_VKEYBOARD__

#include <gtk/gtk.h>

#include "phinkeyboard.h"

G_BEGIN_DECLS

#define PHIN_TYPE_VKEYBOARD         (phin_vkeyboard_get_type())

#define PHIN_VKEYBOARD(obj)         (G_TYPE_CHECK_INSTANCE_CAST((obj), \
                                    PHIN_TYPE_VKEYBOARD, PhinVKeyboard))

#define PHIN_VKEYBOARD_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), \
                                    PHIN_TYPE_VKEYBOARD, \
                                    PhinVKeyboardClass))

#define PHIN_IS_VKEYBOARD(obj)      (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
                                    PHIN_TYPE_VKEYBOARD))

#define PHIN_IS_VKEYBOARD_CLASS(klass) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((klass), PHIN_TYPE_VKEYBOARD))


typedef struct _PhinVKeyboardClass  PhinVKeyboardClass;
typedef struct _PhinVKeyboard       PhinVKeyboard;

struct _PhinVKeyboard
{
    PhinKeyboard parent;
};


struct _PhinVKeyboardClass
{
    PhinKeyboardClass parent_class;
};


GType       phin_vkeyboard_get_type(void);
GtkWidget*  phin_vkeyboard_new(GtkAdjustment*,  int numkeys,
                                                gboolean show_labels);


G_END_DECLS


#endif /* __PHIN_VKEYBOARD__ */

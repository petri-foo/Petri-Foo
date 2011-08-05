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
#include <gtk/gtk.h>

#include "phinhkeyboard.h"


G_DEFINE_TYPE(PhinHKeyboard, phin_hkeyboard, PHIN_TYPE_KEYBOARD);


static void phin_hkeyboard_class_init(PhinHKeyboardClass* klass)
{
    phin_hkeyboard_parent_class = g_type_class_peek_parent(klass);
}


static void phin_hkeyboard_init(PhinHKeyboard* self)
{
    (void)self;
}

/**
 * phin_hkeyboard_new:
 * @adjustment: the #GtkAdjustment that the new keyboard will use for scrolling
 * @numkeys: number of keys to create
 * @show_labels: whether to label the C keys
 *
 * Creates a new #PhinHKeyboard.
 *
 * Returns: a newly created #PhinHKeyboard
 * 
 */
GtkWidget* phin_hkeyboard_new(GtkAdjustment* adj,   int numkeys,
                                                    gboolean show_labels)
{
    if (!adj)
        adj = (GtkAdjustment*) gtk_adjustment_new(0, 0, 0, 0, 0, 0);

    return g_object_new(PHIN_TYPE_HKEYBOARD,
                        "hadjustment",  adj,
                        "shadow-type",  GTK_SHADOW_NONE,
                        "orientation",  GTK_ORIENTATION_HORIZONTAL,
                        "numkeys",      numkeys,
                        "show-labels",  show_labels,
                        NULL);
}

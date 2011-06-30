/*  Petri-Foo is a fork of the Specimen audio sampler.

    Original Specimen author Pete Bessman
    Copyright 2005 Pete Bessman
    Copyright 2011 James W. Morris

    This file is part of Petri-Foo.

    Petri-Foo is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.

    Petri-Foo is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Petri-Foo.  If not, see <http://www.gnu.org/licenses/>.

    This file is a derivative of a Specimen original, modified 2011
*/


#ifndef __ID_SELECTOR__
#define __ID_SELECTOR__

#include <gtk/gtk.h>


#include "names.h"


G_BEGIN_DECLS


#define ID_SELECTOR_TYPE            (id_selector_get_type())
#define ID_SELECTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                    ID_SELECTOR_TYPE, IDSelector))

#define IS_ID_SELECTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                    ID_SELECTOR_TYPE))

#define ID_SELECTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                    ID_SELECTOR_TYPE, IDSelectorClass))

#define IS_ID_SELECTOR_CLASS(klass) (G_TYPE_CHECK_INSTANCE_TYPE ((klass), \
                                    ID_SELECTOR_TYPE))


typedef struct _IDSelectorClass IDSelectorClass;
typedef struct _IDSelector      IDSelector;


enum {
    ID_SELECTOR_H = 1,
    ID_SELECTOR_V
};


struct _IDSelector
{
    GtkHBox parent_instance;
};


struct _IDSelectorClass
{
    GtkHBoxClass parent_class;
    void (*changed)(IDSelector* self); /* <private> */
};


GType       id_selector_get_type(void);
GtkWidget*  id_selector_new(void);


/* second argument must be an array of pointers */
void        id_selector_set(IDSelector*,    const id_name* ids_names,
                                            int orientation);

int         id_selector_get_id(IDSelector*);
const char* id_selector_get_name(IDSelector*);
const char* id_selector_get_name_by_id(IDSelector*, int id);


G_END_DECLS

#endif /* __ID_SELECTOR__ */

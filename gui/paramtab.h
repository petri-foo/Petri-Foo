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


#ifndef __PARAM_TAB__
#define __PARAM_TAB__

#include <gtk/gtk.h>

#include "patch.h"

G_BEGIN_DECLS

#define PARAM_TAB_TYPE \
    (param_tab_get_type())

#define PARAM_TAB(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), PARAM_TAB_TYPE, ParamTab))

#define IS_PARAM_TAB(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PARAM_TAB_TYPE))

#define PARAM_TAB_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), PARAM_TAB_TYPE, ParamTabClass))

#define IS_PARAM_TAB_CLASS(klass) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((klass), PARAM_TAB_TYPE))


typedef struct _ParamTabClass ParamTabClass;
typedef struct _ParamTab ParamTab;


struct _ParamTab
{
    GtkVBox     parent_instance;
};


struct _ParamTabClass
{
    GtkVBoxClass parent_class;
};


GType       param_tab_get_type(void);
GtkWidget*  param_tab_new(void);

void        param_tab_set_param(ParamTab*, PatchParamType);
void        param_tab_set_patch(ParamTab*, int patch);


G_END_DECLS


#endif /* __PARAM_TAB__ */

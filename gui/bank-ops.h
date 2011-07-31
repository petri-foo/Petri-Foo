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


#ifndef __BANK_OPS_H__
#define __BANK_OPS_H__


#include <gtk/gtk.h>


int         bank_ops_new     (void);
int         bank_ops_open    (GtkWidget* parent_window);
int         bank_ops_save_as (GtkWidget* parent_window);
int         bank_ops_save    (GtkWidget* parent_window);
const char* bank_ops_bank    (void);
int         bank_ops_open_recent (GtkWidget* parent_window
                , char* filename);

#endif /* __BANK_OPS_H__ */

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


#ifndef __GUI_H__
#define __GUI_H__

#include "petri-foo.h"
#include "patchlist.h"

enum
{
    GUI_SPACING = 6,        /* space between widgets */
    GUI_INDENT = 18,        /* how much to indent sections by */
    GUI_SECSPACE = 18,      /* space between sections */
    GUI_SCROLLSPACE = 3,    /* space between a scrollbar and its thingie */
    GUI_TITLESPACE = 12,    /* space between section title and contents */
    GUI_TEXTSPACE = 12,     /* space between a label and its control */
    GUI_BORDERSPACE = 12,   /* space between a border and its guts */
    GUI_THRESHOLD = 20,     /* threshold used for sliderbuttons */
 GUI_REFRESH_TIMEOUT = 100, /* milliseconds controller refreshes */
   GUI_MAX_RECENT_FILES = 5 /* number of files in Open Recent menu */
};


/* returns a titlefied label */
GtkWidget*  gui_title_new(const char* msg);

/* returns a horizontal padding widget of the chosen size */
GtkWidget*  gui_hpad_new(int size);

/* returns a vertical padding widget of the chosen size */
GtkWidget*  gui_vpad_new(int size);

/* returns a newly prepared section and a box to put children in */
GtkWidget*  gui_section_new(const char* name, GtkWidget** box);

/* attaches (and shows) widget to table */
void        gui_attach(GtkTable*, GtkWidget*, guint l, guint r,
                                              guint t, guint b);

/* attaches (and shows) label  to table */
GtkWidget*  gui_label_attach(const char*, GtkTable*, guint l, guint r,
                                                     guint t, guint b);

/* packs (and shows) widget into box */
void        gui_pack(GtkBox*, GtkWidget*);

/* packs (and shows) label into box (returns label) */
GtkWidget*  gui_label_pack(const char*, GtkBox*);


/* prepare the gui for use */
int gui_init(void);

/* refresh the gui's display */
void gui_refresh(void);

/* update the recently used files */
void gui_recent_files_load(void);

/* get the gui's PatchList widget */
PatchList* gui_get_patch_list(void);

/*  add bank name to window title, or send NULL to update
    instance name in window title
 */
void gui_set_window_title_bank(const char*);

/* callbacks for use by context menu ( see patchlist.[ch] ) */
void cb_menu_patch_add(         GtkWidget* menu_item, gpointer data);
void cb_menu_patch_add_default( GtkWidget* menu_item, gpointer data);
void cb_menu_patch_duplicate(   GtkWidget* menu_item, gpointer data);
void cb_menu_patch_rename(      GtkWidget* menu_item, gpointer data);
void cb_menu_patch_remove(      GtkWidget* menu_item, gpointer data);

void cb_menu_view_log_display_showing(gboolean);


GtkWidget* gui_menu_add(GtkWidget* menu, const char* label, GCallback cb,
                                                            gpointer data);

GtkWidget*
     gui_menu_check_add(GtkWidget* menu, const char* label, gboolean active,
                                                            GCallback cb,
                                                            gpointer data);

void gui_set_session_mode(void);

extern GtkRecentManager *recent_manager;
 
#endif /* __GUI_H__ */

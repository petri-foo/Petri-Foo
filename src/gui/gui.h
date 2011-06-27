#ifndef __GUI_H__
#define __GUI_H__

#include "petri-foo.h"
#include "patchlist.h"

enum
{
    GUI_SPACING = 6, 		/* space between widgets */
    GUI_INDENT = 18,		/* how much to indent sections by */
    GUI_SECSPACE = 18, 		/* space between sections */
    GUI_SCROLLSPACE = 3,	/* space between a scrollbar and its scrollie thingie */
    GUI_TITLESPACE = 12,	/* space between a section title and its contents */
    GUI_TEXTSPACE = 12,		/* space between a label and its control */
    GUI_BORDERSPACE = 12,	/* space between a border and its guts */
    GUI_THRESHOLD = 20,		/* threshold used for sliderbuttons */
    GUI_REFRESH_TIMEOUT = 100	/* time in milliseconds between controller refreshes */
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

/* get the gui's PatchList widget */
PatchList* gui_get_patch_list(void);

/* set petri-foo window title */
void gui_set_window_title(const char* title);

/* callbacks for use by context menu ( see patchlist.[ch] ) */
void cb_menu_patch_add(         GtkWidget* menu_item, gpointer data);
void cb_menu_patch_add_default( GtkWidget* menu_item, gpointer data);
void cb_menu_patch_duplicate(   GtkWidget* menu_item, gpointer data);
void cb_menu_patch_rename(      GtkWidget* menu_item, gpointer data);
void cb_menu_patch_remove(      GtkWidget* menu_item, gpointer data);


GtkWidget* gui_menu_add(GtkWidget* menu, const char* label, GCallback cb,
                                                            gpointer data);

#endif /* __GUI_H__ */

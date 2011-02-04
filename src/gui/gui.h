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
GtkWidget* gui_title_new(const char* msg);

/* returns a horizontal padding widget of the chosen size */
GtkWidget* gui_hpad_new(int size);

/* returns a vertical padding widget of the chosen size */
GtkWidget* gui_vpad_new(int size);

/* returns a newly prepared section and a box to put children in */
GtkWidget* gui_section_new(const char* name, GtkWidget** box);

/* prepare the gui for use */
int gui_init(void);

/* refresh the gui's display */
void gui_refresh(void);

/* get the gui's PatchList widget */
PatchList* gui_get_patch_list(void);

#endif /* __GUI_H__ */

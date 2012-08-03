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


#include <gtk/gtk.h>
#include "patchlist.h"
#include "gui.h"
#include "petri-foo.h"
#include "patch_set_and_get.h"
#include "patch_util.h"


#include <string.h>


/* magic numbers */
enum
{
    LIST_WIDTH = 128,
    SPACING = GUI_SCROLLSPACE,
};

/* signals */
enum
{
    CHANGED,
    LAST_SIGNAL,
};

/* model columns */
enum
{
    PATCH_NAME,
    PATCH_ID,
    LAST_COLUMN,
};

typedef struct _PatchListPrivate PatchListPrivate;

#define PATCH_LIST_GET_PRIVATE(obj)     \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
        PATCH_LIST_TYPE, PatchListPrivate))

struct _PatchListPrivate
{
    int                 patch;
    GtkWidget*          patch_tree;
    GtkListStore*       patch_store;
    GtkWidget*          vscroll;
    GtkTreeSelection*   select;
    GtkWidget*          self;
};


static int signals[LAST_SIGNAL];

G_DEFINE_TYPE(PatchList, patch_list, GTK_TYPE_HBOX);

static void patch_list_class_init(PatchListClass* klass)
{
    GtkObjectClass *object_class = GTK_OBJECT_CLASS(klass);
    patch_list_parent_class = g_type_class_peek_parent(klass);

    signals[CHANGED] =
        g_signal_new("changed",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                        G_STRUCT_OFFSET (PatchListClass, changed),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

    klass->changed = NULL;
    g_type_class_add_private(object_class, sizeof(PatchListPrivate));
}


static void select_cb(GtkTreeSelection* selection, PatchListPrivate* p)
{
    GtkTreeIter iter;
    GtkTreeModel *model;

    if (gtk_tree_selection_get_selected(selection, &model, &iter))
        gtk_tree_model_get(model, &iter, PATCH_ID, &p->patch, -1);
    else
        p->patch = -1;

    g_signal_emit_by_name(G_OBJECT(p->self), "changed");
}


void popup_menu(GdkEventButton *event, gboolean full_menu, gpointer data)
{
    (void)data;

    GtkWidget *menu = gtk_menu_new();

    gui_menu_add(menu, "Add Patch...",
                G_CALLBACK(cb_menu_patch_add),          NULL);
    gui_menu_add(menu, "Add Default Patch",
                G_CALLBACK(cb_menu_patch_add_default),  NULL);

    if (full_menu)
    {
        gui_menu_add(menu, "Duplicate Patch",
                G_CALLBACK(cb_menu_patch_duplicate),    NULL);
        gui_menu_add(menu, "Rename Patch...",
                G_CALLBACK(cb_menu_patch_rename),       NULL);
        gui_menu_add(menu, "Remove Patch",
                G_CALLBACK(cb_menu_patch_remove),       NULL);
    }

    gtk_widget_show_all(menu);

    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
                   (event != NULL) ? event->button : 0,
                   gdk_event_get_time((GdkEvent*)event));
}


static gboolean
pressed_cb(GtkWidget *treeview, GdkEventButton *event, gpointer data)
{
    PatchListPrivate* p = (PatchListPrivate*)data;

    if (event->type == GDK_BUTTON_PRESS && event->button == 3)
    {
        GtkTreePath* path;
        GtkTreeSelection* selection;
        gboolean full_menu = FALSE;

        /* select row the click happened in */

        selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

        if (gtk_tree_selection_count_selected_rows(selection) <= 1)
        {
            if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(treeview),
                                                    (gint)event->x, 
                                                    (gint)event->y,
                                                    &path,
                                                    NULL, NULL, NULL))
            {
                g_signal_handlers_block_by_func(p->select, select_cb, p);
                gtk_tree_selection_unselect_all(selection);
                g_signal_handlers_unblock_by_func(p->select, select_cb, p);

                gtk_tree_selection_select_path(selection, path);
                full_menu = TRUE;
            }
        }

        popup_menu(event, full_menu, data);
        gtk_tree_path_free(path);

        return TRUE;
    }

    return FALSE;
}

static gboolean popup_cb(GtkWidget *treeview, gpointer data)
{
    (void)treeview;(void)data;
    popup_menu(NULL, TRUE, data);
    return TRUE;
}


static void patch_list_init(PatchList* self)
{
    PatchListPrivate* p = PATCH_LIST_GET_PRIVATE(self);
    GtkBox* box = GTK_BOX(self);
    GtkTreeViewColumn* column;
    GtkCellRenderer* renderer;

    p->self = GTK_WIDGET(self);

    gtk_box_set_spacing(box, SPACING);

    /* patch list */
    p->patch_store =
        gtk_list_store_new(LAST_COLUMN, G_TYPE_STRING, G_TYPE_INT);

    p->patch_tree = 
        gtk_tree_view_new_with_model(GTK_TREE_MODEL(p->patch_store));

    p->vscroll = gtk_vscrollbar_new(NULL);
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(  "Patch",
                                                        renderer,
                                                        "text", PATCH_NAME,
                                                        NULL);

    gtk_tree_view_append_column(GTK_TREE_VIEW(p->patch_tree), column);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(p->patch_tree), FALSE);
    gtk_tree_view_set_vadjustment(GTK_TREE_VIEW(p->patch_tree),
                        gtk_range_get_adjustment(GTK_RANGE(p->vscroll)));
    gtk_widget_set_size_request(p->patch_tree, LIST_WIDTH, 1);

    gtk_box_pack_start(box, p->patch_tree, TRUE, TRUE, 0);
    gtk_box_pack_start(box, p->vscroll, FALSE, FALSE, 0);

    gtk_widget_show(p->patch_tree);
    gtk_widget_show(p->vscroll);

    /* connect up */
    p->select = gtk_tree_view_get_selection(GTK_TREE_VIEW(p->patch_tree));
    gtk_tree_selection_set_mode(p->select, GTK_SELECTION_SINGLE);
    g_signal_connect(p->select, "changed",
                                G_CALLBACK(select_cb), (gpointer)p);

    g_signal_connect(p->patch_tree, "button-press-event",
                                G_CALLBACK(pressed_cb), (gpointer)p);

    g_signal_connect(p->patch_tree, "popup-menu",
                                G_CALLBACK(popup_cb), (gpointer)p);
}


GtkWidget* patch_list_new(void)
{
    return (GtkWidget*) g_object_new(PATCH_LIST_TYPE, NULL);
}


void patch_list_update(PatchList* self, int target, int type)
{
    PatchListPrivate* p = PATCH_LIST_GET_PRIVATE(self);
    int npatches;
    int* patches;
    GtkTreeIter iter;
    GtkTreePath* path;
    const char* str;
    int i;
    int index = 0;

    g_signal_handlers_block_by_func(p->select, select_cb, p);

    npatches = patch_dump(&patches);
    gtk_list_store_clear(p->patch_store);
    
    for (i = 0; i < npatches; ++i)
    {
        str = patch_get_name(patches[i]);
        gtk_list_store_append(p->patch_store, &iter);
        gtk_list_store_set(p->patch_store, &iter, PATCH_NAME, str,
                                                  PATCH_ID, patches[i], -1);

        if (type == PATCH_LIST_PATCH && patches[i] == target)
            index = i;
    }

    g_free(patches);

    if (type == PATCH_LIST_INDEX)
        index = target;

    path = gtk_tree_path_new_from_indices(index, -1);
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(p->patch_tree), path, NULL,
                                                                    FALSE);
    gtk_tree_path_free(path);

    g_signal_handlers_unblock_by_func(p->select, select_cb, p);

    g_signal_emit_by_name(G_OBJECT(p->select), "changed");
}


int patch_list_get_current_patch(PatchList* self)
{
    return PATCH_LIST_GET_PRIVATE(self)->patch;
}
  

int patch_list_get_current_index(PatchList* self)
{
    PatchListPrivate* p = PATCH_LIST_GET_PRIVATE(self);
    int* i;
    GtkTreePath* t;

    gtk_tree_view_get_cursor(GTK_TREE_VIEW(p->patch_tree), &t, NULL);
    i = gtk_tree_path_get_indices(t);

    return *i;
}

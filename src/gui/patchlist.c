#include <gtk/gtk.h>
#include "patchlist.h"
#include "gui.h"
#include "petri-foo.h"
#include "patch_util.h"

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

static int signals[LAST_SIGNAL];
static GtkHBoxClass* parent_class;

static void patch_list_class_init(PatchListClass* klass);
static void patch_list_init(PatchList* self);


GType patch_list_get_type(void)
{
    static GType type = 0;

    if (!type)
    {
	static const GTypeInfo info =
	    {
		sizeof (PatchListClass),
		NULL,
		NULL,
		(GClassInitFunc) patch_list_class_init,
		NULL,
		NULL,
		sizeof (PatchList),
		0,
		(GInstanceInitFunc) patch_list_init,
	    };

	/* replace PARENT_CLASS_TYPE with whatever's appropriate for your widget */
	type = g_type_register_static(GTK_TYPE_HBOX, "PatchList", &info, 0);
    }

    return type;
}


static void patch_list_class_init(PatchListClass* klass)
{
    parent_class = g_type_class_peek_parent(klass);

    signals[CHANGED] =
	  g_signal_new ("changed",
			G_TYPE_FROM_CLASS (klass),
			G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			G_STRUCT_OFFSET (PatchListClass, changed),
			NULL, NULL,
			g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

    klass->changed = NULL;
}


static void select_cb(GtkTreeSelection* selection, PatchList* self)
{
    GtkTreeIter iter;
    GtkTreeModel *model;

    if (gtk_tree_selection_get_selected(selection, &model, &iter))
    {
	gtk_tree_model_get(model, &iter, PATCH_ID, &self->patch, -1);
    }
    else
    {
	self->patch = -1;
    }

    g_signal_emit_by_name(G_OBJECT(self), "changed");
}


static void patch_list_init(PatchList* self)
{
    GtkBox* box = GTK_BOX(self);
    GtkCellRenderer* renderer;
    GtkTreeViewColumn* column;

    gtk_box_set_spacing(box, SPACING);

    /* patch list */
    self->patch_store = gtk_list_store_new(LAST_COLUMN, G_TYPE_STRING, G_TYPE_INT);
    self->patch_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(self->patch_store));
    self->vscroll = gtk_vscrollbar_new(NULL);
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Patch", renderer,
						      "text", PATCH_NAME,
						      NULL);

    gtk_tree_view_append_column(GTK_TREE_VIEW(self->patch_tree), column);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(self->patch_tree), FALSE);
    gtk_tree_view_set_vadjustment(GTK_TREE_VIEW(self->patch_tree),
				  gtk_range_get_adjustment(GTK_RANGE(self->vscroll)));
    gtk_widget_set_size_request(self->patch_tree, LIST_WIDTH, 1);

    gtk_box_pack_start(box, self->patch_tree, TRUE, TRUE, 0);
    gtk_box_pack_start(box, self->vscroll, FALSE, FALSE, 0);

    gtk_widget_show(self->patch_tree);
    gtk_widget_show(self->vscroll);

    /* connect up */
    self->select = gtk_tree_view_get_selection(GTK_TREE_VIEW(self->patch_tree));
    gtk_tree_selection_set_mode(self->select, GTK_SELECTION_SINGLE);
    g_signal_connect(G_OBJECT(self->select), "changed",
		     G_CALLBACK(select_cb), (gpointer) self);
}


GtkWidget* patch_list_new(void)
{
    return (GtkWidget*) g_object_new(PATCH_LIST_TYPE, NULL);
}


void patch_list_update(PatchList* self, int target, int type)
{
    int npatches;
    int* patches;
    GtkTreeIter iter;
    GtkTreePath* path;
    char* str;
    int i;
    int index = 0;

    g_signal_handlers_block_by_func(self->select, select_cb, self);

    npatches = patch_dump(&patches);
    gtk_list_store_clear(self->patch_store);
    
    for (i = 0; i < npatches; ++i)
    {
	str = patch_get_name(patches[i]);

	gtk_list_store_append(self->patch_store, &iter);
	gtk_list_store_set(self->patch_store, &iter,
			   PATCH_NAME, str,
			   PATCH_ID, patches[i],
			   -1);

	g_free(str);

	if (type == PATCH_LIST_PATCH && patches[i] == target)
	    index = i;
    }
    g_free(patches);

    if (type == PATCH_LIST_INDEX)
	index = target;

    path = gtk_tree_path_new_from_indices(index, -1);
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(self->patch_tree), path, NULL, FALSE);
    gtk_tree_path_free(path);

    g_signal_handlers_unblock_by_func(self->select, select_cb, self);

    g_signal_emit_by_name(G_OBJECT(self->select), "changed");
}


int patch_list_get_current_patch(PatchList* self)
{
    return self->patch;
}
  

int patch_list_get_current_index(PatchList* self)
{
    int* i;
    GtkTreePath* t;

    gtk_tree_view_get_cursor(GTK_TREE_VIEW(self->patch_tree), &t, NULL);
    i = gtk_tree_path_get_indices(t);

    return *i;
}

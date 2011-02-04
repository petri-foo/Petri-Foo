#ifndef __PATCH_LIST__
#define __PATCH_LIST__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PATCH_LIST_TYPE (patch_list_get_type())
#define PATCH_LIST(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PATCH_LIST_TYPE, PatchList))
#define PATCH_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), PATCH_LIST_TYPE, PatchListClass))
#define IS_PATCH_LIST(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PATCH_LIST_TYPE))
#define IS_PATCH_LIST_CLASS(klass) (G_TYPE_CHECK_INSTANCE_TYPE ((klass), PATCH_LIST_TYPE))

typedef struct _PatchListClass PatchListClass;
typedef struct _PatchList PatchList;

enum
{
    PATCH_LIST_INDEX,		/* target is a display index */
    PATCH_LIST_PATCH 		/* target is a patch id */
};

struct _PatchList
{
    GtkHBox parent;

    /*< private >*/
    GtkWidget* patch_tree;
    GtkListStore* patch_store;
    GtkWidget* vscroll;
    GtkTreeSelection* select;
    int patch;
};

struct _PatchListClass
{
    GtkHBoxClass parent_class;

    /*< private >*/
    void (*changed)(PatchList* self);
};

GType patch_list_get_type(void);
GtkWidget* patch_list_new(void);

/* refresh the list of available pactches; target specifies either the
 index or patch id of the item to select, depending on the value of
 type */
void patch_list_update(PatchList* self, int target, int type);

/* returns which patch we are currently looking at */
int patch_list_get_current_patch(PatchList* self);

/* returns the index of the patch we are currently looking at */
int patch_list_get_current_index(PatchList* self);
    
G_END_DECLS

#endif /* __PATCH_LIST__ */

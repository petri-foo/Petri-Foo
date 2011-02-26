#ifndef __PATCH_LIST__
#define __PATCH_LIST__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PATCH_LIST_TYPE \
    (patch_list_get_type())

#define PATCH_LIST(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), PATCH_LIST_TYPE, PatchList))

#define IS_PATCH_LIST(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PATCH_LIST_TYPE))

#define PATCH_LIST_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), PATCH_LIST_TYPE, PatchListClass))

#define IS_PATCH_LIST_CLASS(klass) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((klass), PATCH_LIST_TYPE))


typedef struct _PatchListClass  PatchListClass;
typedef struct _PatchList       PatchList;


enum
{
    PATCH_LIST_INDEX,		/* target is a display index */
    PATCH_LIST_PATCH 		/* target is a patch id */
};


struct _PatchList
{
    GtkHBox parent_instance;
};


struct _PatchListClass
{
    GtkHBoxClass parent_class;
    void (*changed)(PatchList* self); /* <private> */
};


GType       patch_list_get_type(void);
GtkWidget*  patch_list_new(void);


/* refresh the list of available pactches; target specifies either the
 index or patch id of the item to select, depending on the value of
 type */
void patch_list_update(PatchList*, int target, int type);

/* returns which patch we are currently looking at */
int patch_list_get_current_patch(PatchList*);

/* returns the index of the patch we are currently looking at */
int patch_list_get_current_index(PatchList*);
    
G_END_DECLS

#endif /* __PATCH_LIST__ */

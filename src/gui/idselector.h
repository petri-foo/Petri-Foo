#ifndef __ID_SELECTOR__
#define __ID_SELECTOR__

#include <gtk/gtk.h>


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

void        id_selector_set(IDSelector*, const char** item_names,
                                                    int orientation);

int         id_selector_get_id(IDSelector*);
const char* id_selector_get_name(IDSelector*);
const char* id_selector_get_name_by_id(IDSelector*, int id);


G_END_DECLS

#endif /* __ID_SELECTOR__ */

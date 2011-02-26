#ifndef __MASTER_SECTION__
#define __MASTER_SECTION__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MASTER_SECTION_TYPE \
    (master_section_get_type())

#define MASTER_SECTION(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), MASTER_SECTION_TYPE, MasterSection))

#define IS_MASTER_SECTION(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), MASTER_SECTION_TYPE))

#define MASTER_SECTION_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), MASTER_SECTION_TYPE, \
                                      MasterSectionClass))

#define IS_MASTER_SECTION_CLASS(klass) \
    (G_TYPE_CHECK_INSTANCE_TYPE((klass), MASTER_SECTION_TYPE))


typedef struct _MasterSectionClass MasterSectionClass;
typedef struct _MasterSection MasterSection;


struct _MasterSection
{
    GtkVBox parent_instance;
    GtkWidget* amplitude_fan; /* <private> */
};


struct _MasterSectionClass
{
    GtkVBoxClass parent_class;
};


GType       master_section_get_type(void);
GtkWidget*  master_section_new(void);


void        master_section_update(MasterSection* self);


G_END_DECLS


#endif /* __MASTER_SECTION__ */

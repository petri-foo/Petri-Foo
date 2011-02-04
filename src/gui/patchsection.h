#ifndef __PATCH_SECTION__
#define __PATCH_SECTION__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PATCH_SECTION_TYPE (patch_section_get_type())
#define PATCH_SECTION(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PATCH_SECTION_TYPE, PatchSection))
#define PATCH_SECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), PATCH_SECTION_TYPE, PatchSectionClass))
#define IS_PATCH_SECTION(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PATCH_SECTION_TYPE))
#define IS_PATCH_SECTION_CLASS(klass) (G_TYPE_CHECK_INSTANCE_TYPE ((klass), PATCH_SECTION_TYPE))

typedef struct _PatchSectionClass PatchSectionClass;
typedef struct _PatchSection PatchSection;

struct _PatchSection
{
    GtkVBox parent;

    /*< private >*/
    int patch;
    guint refresh;
    GtkWidget* title;
    GtkWidget* volume_fan;
    GtkWidget* pan_fan;
    GtkWidget* pitch_fan;
    GtkWidget* range_sb;
    GtkWidget* notebook;
    GtkWidget* sample_tab;
    GtkWidget* voice_tab;
    GtkWidget* filter_tab;
    GtkWidget* vel_tab;
    GtkWidget* env_tab;
    GtkWidget* lfo_tab;
    GtkWidget* pitch_tab;
};

struct _PatchSectionClass
{
    GtkVBoxClass parent_class;

    /*< private >*/
};

GType patch_section_get_type(void);
GtkWidget* patch_section_new(void);
void patch_section_set_patch(PatchSection* self, int patch);

G_END_DECLS

#endif /* __PATCH_SECTION__ */

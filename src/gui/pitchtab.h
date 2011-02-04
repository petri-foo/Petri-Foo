#ifndef __PITCH_TAB__
#define __PITCH_TAB__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PITCH_TAB_TYPE \
    (pitch_tab_get_type())

#define PITCH_TAB(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), PITCH_TAB_TYPE, PitchTab))

#define PITCH_TAB_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), PITCH_TAB_TYPE, PitchTabClass))

#define IS_PITCH_TAB(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PITCH_TAB_TYPE))

#define IS_PITCH_TAB_CLASS(klass) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((klass), PITCH_TAB_TYPE))

typedef struct _PitchTabClass PitchTabClass;
typedef struct _PitchTab PitchTab;

struct _PitchTab
{
    GtkVBox     parent;

    /*< private >*/
    int         patch;
    guint       refresh;
    GtkWidget*  tuning_fan;
    GtkWidget*  range_sb;
    GtkWidget*  mod1_opt;
    GtkWidget*  mod1_fan;
    GtkWidget*  mod2_opt;
    GtkWidget*  mod2_fan;

    GtkWidget*  freq_fan;
    GtkWidget*  reso_fan;


};

struct _PitchTabClass
{
    GtkVBoxClass parent_class;

    /*< private >*/
};

GType pitch_tab_get_type(void);
GtkWidget* pitch_tab_new(void);
void pitch_tab_set_patch(PitchTab* self, int patch);

G_END_DECLS

#endif /* __PITCH_TAB__ */

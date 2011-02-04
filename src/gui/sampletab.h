#ifndef __SAMPLE_TAB__
#define __SAMPLE_TAB__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define SAMPLE_TAB_TYPE (sample_tab_get_type())
#define SAMPLE_TAB(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SAMPLE_TAB_TYPE, SampleTab))
#define SAMPLE_TAB_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), SAMPLE_TAB_TYPE, SampleTabClass))
#define IS_SAMPLE_TAB(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SAMPLE_TAB_TYPE))
#define IS_SAMPLE_TAB_CLASS(klass) (G_TYPE_CHECK_INSTANCE_TYPE ((klass), SAMPLE_TAB_TYPE))

typedef struct _SampleTabClass SampleTabClass;
typedef struct _SampleTab SampleTab;

struct _SampleTab
{
    GtkVBox parent;

    /*< private >*/
    int patch;
    GtkWidget* waveform;
    GtkWidget* mode_opt;
    GtkWidget* file_label;
    GtkWidget* file_button;
    GtkWidget* reverse_check;
};

struct _SampleTabClass
{
    GtkVBoxClass parent_class;

    /*< private >*/
};

GType sample_tab_get_type(void);
GtkWidget* sample_tab_new(void);
void sample_tab_set_patch(SampleTab* self, int patch);

G_END_DECLS

#endif /* __SAMPLE_TAB__ */

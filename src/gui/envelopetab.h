#ifndef __ENVELOPE_TAB__
#define __ENVELOPE_TAB__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define ENVELOPE_TAB_TYPE (envelope_tab_get_type())
#define ENVELOPE_TAB(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENVELOPE_TAB_TYPE, EnvelopeTab))
#define ENVELOPE_TAB_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), ENVELOPE_TAB_TYPE, EnvelopeTabClass))
#define IS_ENVELOPE_TAB(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENVELOPE_TAB_TYPE))
#define IS_ENVELOPE_TAB_CLASS(klass) (G_TYPE_CHECK_INSTANCE_TYPE ((klass), ENVELOPE_TAB_TYPE))

typedef struct _EnvelopeTabClass EnvelopeTabClass;
typedef struct _EnvelopeTab EnvelopeTab;

struct _EnvelopeTab
{
    GtkVBox parent;

    /*< private >*/
    int patch;
    GtkWidget* idsel;

    GtkWidget* env_check; /* if on */

    GtkWidget* delay_fan;
    GtkWidget* attack_fan;
    GtkWidget* hold_fan;
    GtkWidget* decay_fan;
    GtkWidget* sustain_fan;
    GtkWidget* release_fan;
};

struct _EnvelopeTabClass
{
    GtkVBoxClass parent_class;

    /*< private >*/
};

GType envelope_tab_get_type(void);
GtkWidget* envelope_tab_new(void);
void envelope_tab_set_patch(EnvelopeTab* self, int patch);

G_END_DECLS

#endif /* __ENVELOPE_TAB__ */

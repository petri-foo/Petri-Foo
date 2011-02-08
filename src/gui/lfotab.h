#ifndef __LFO_TAB__
#define __LFO_TAB__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define LFO_TAB_TYPE (lfo_tab_get_type())
#define LFO_TAB(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), LFO_TAB_TYPE, LfoTab))
#define LFO_TAB_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), LFO_TAB_TYPE, LfoTabClass))
#define IS_LFO_TAB(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LFO_TAB_TYPE))
#define IS_LFO_TAB_CLASS(klass) (G_TYPE_CHECK_INSTANCE_TYPE ((klass), LFO_TAB_TYPE))

typedef struct _LfoTabClass LfoTabClass;
typedef struct _LfoTab LfoTab;

struct _LfoTab
{
    GtkHBox parent;

    /*< private >*/
    int patch_id;
    int lfo_id;
    GtkWidget* idsel;
    GtkWidget* shape_opt;
    GtkWidget* lfo_check;
    GtkWidget* free_radio;
    GtkWidget* sync_radio;
    GtkWidget* beats_sb;
    GtkWidget* pos_check;
    GtkWidget* freq_fan;
    GtkWidget* delay_fan;
    GtkWidget* attack_fan;
    GtkWidget* mod1_combo;
    GtkWidget* mod2_combo;
    GtkWidget* mod1_amount;
    GtkWidget* mod2_amount;
};

struct _LfoTabClass
{
    GtkVBoxClass parent_class;

    /*< private >*/
};

GType lfo_tab_get_type(void);
GtkWidget* lfo_tab_new(void);
void lfo_tab_set_patch(LfoTab* self, int patch);

G_END_DECLS

#endif /* __LFO_TAB__ */

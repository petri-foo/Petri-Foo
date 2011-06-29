#include <gtk/gtk.h>
#include <phat/phat.h>

#include "amptab.h"
#include "gui.h"
#include "patch.h"

#include "mod_src_gui.h"
#include "mod_section.h"


static GtkVBoxClass* parent_class;

static void amp_tab_class_init(AmpTabClass* klass);
static void amp_tab_init(AmpTab* self);
static void amp_tab_destroy(GtkObject* object);


GType amp_tab_get_type(void)
{
    static GType type = 0;

    if (!type)
    {
        static const GTypeInfo info =
        {
            sizeof (AmpTabClass),
            NULL,
            NULL,
            (GClassInitFunc) amp_tab_class_init,
            NULL,
            NULL,
            sizeof (AmpTab),
            0,
            (GInstanceInitFunc) amp_tab_init,
            NULL
        };
        type = g_type_register_static(GTK_TYPE_VBOX, "AmpTab", &info, 0);
    }

    return type;
}


static void amp_tab_class_init(AmpTabClass* klass)
{
    parent_class = g_type_class_peek_parent(klass);
    GTK_OBJECT_CLASS(klass)->destroy = amp_tab_destroy;
}


static void amp_tab_init(AmpTab* self)
{
    GtkBox* box = GTK_BOX(self);

    self->amp = mod_section_new();
    mod_section_set_param(MOD_SECTION(self->amp), PATCH_PARAM_AMPLITUDE);
    gtk_box_pack_start(box, self->amp, FALSE, FALSE, 0);
    gtk_widget_show(self->amp);

    self->pan = mod_section_new();
    mod_section_set_param(MOD_SECTION(self->pan), PATCH_PARAM_PANNING);
    gtk_box_pack_start(box, self->pan, FALSE, FALSE, 0);
    gtk_widget_show(self->pan);
}


static void amp_tab_destroy(GtkObject* object)
{
    AmpTab* self = AMP_TAB(object);
    GtkObjectClass* klass = GTK_OBJECT_CLASS(parent_class);

    if (!g_source_remove(self->refresh))
    {
        debug("failed to remove refresh function from idle loop: %u\n",
                                                        self->refresh);
    }
    else
    {
        debug("refresh function removed\n");
    }

    if (klass->destroy)
        klass->destroy(object);
}


GtkWidget* amp_tab_new(void)
{
    return (GtkWidget*) g_object_new(AMP_TAB_TYPE, NULL);
}


void amp_tab_set_patch(AmpTab* self, int patch)
{
    mod_section_set_patch(MOD_SECTION(self->amp), patch);
    mod_section_set_patch(MOD_SECTION(self->pan), patch);
}

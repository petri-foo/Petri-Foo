#include <gtk/gtk.h>
#include <phat/phat.h>
#include "pitchtab.h"
#include "gui.h"
#include "patch.h"

#include "mod_src_gui.h"
#include "mod_section.h"

static GtkVBoxClass* parent_class;

static void pitch_tab_class_init(PitchTabClass* klass);
static void pitch_tab_init(PitchTab* self);
static void pitch_tab_destroy(GtkObject* object);


GType pitch_tab_get_type(void)
{
    static GType type = 0;

    if (!type)
    {
        static const GTypeInfo info =
        {
            sizeof (PitchTabClass),
            NULL,
            NULL,
            (GClassInitFunc) pitch_tab_class_init,
            NULL,
            NULL,
            sizeof (PitchTab),
            0,
            (GInstanceInitFunc) pitch_tab_init,
        };
        type = g_type_register_static(GTK_TYPE_VBOX, "PitchTab", &info, 0);
    }

    return type;
}


static void pitch_tab_class_init(PitchTabClass* klass)
{
    parent_class = g_type_class_peek_parent(klass);
    GTK_OBJECT_CLASS(klass)->destroy = pitch_tab_destroy;
}


static void pitch_tab_init(PitchTab* self)
{
    GtkBox* box = GTK_BOX(self);

    self->pitch = mod_section_new();
    mod_section_set_param(MOD_SECTION(self->pitch), PATCH_PARAM_PITCH);
    gtk_box_pack_start(box, self->pitch, FALSE, FALSE, 0);
    gtk_widget_show(self->pitch);
}


static void pitch_tab_destroy(GtkObject* object)
{
    PitchTab* self = PITCH_TAB(object);
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


GtkWidget* pitch_tab_new(void)
{
    return (GtkWidget*) g_object_new(PITCH_TAB_TYPE, NULL);
}


void pitch_tab_set_patch(PitchTab* self, int patch)
{
    mod_section_set_patch(MOD_SECTION(self->pitch), patch);
}

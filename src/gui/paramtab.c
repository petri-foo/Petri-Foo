#include <gtk/gtk.h>
#include <phat/phat.h>

#include "paramtab.h"
#include "gui.h"
#include "patch.h"

#include "mod_src.h"
#include "mod_section.h"


static GtkVBoxClass* parent_class;

static void param_tab_class_init(ParamTabClass* klass);
static void param_tab_init(ParamTab* self);
static void param_tab_destroy(GtkObject* object);


GType param_tab_get_type(void)
{
    static GType type = 0;

    if (!type)
    {
        static const GTypeInfo info =
        {
            sizeof (ParamTabClass),
            NULL,
            NULL,
            (GClassInitFunc) param_tab_class_init,
            NULL,
            NULL,
            sizeof (ParamTab),
            0,
            (GInstanceInitFunc) param_tab_init,
        };
        type = g_type_register_static(GTK_TYPE_VBOX, "ParamTab", &info, 0);
    }

    return type;
}


static void param_tab_class_init(ParamTabClass* klass)
{
    parent_class = g_type_class_peek_parent(klass);
    GTK_OBJECT_CLASS(klass)->destroy = param_tab_destroy;
}


static void param_tab_init(ParamTab* self)
{
    self->patch_id = -1;
    self->param = PATCH_PARAM_INVALID;
    self->modsect1 = 0;
    self->modsect2 = 0;
}


static void param_tab_destroy(GtkObject* object)
{
    ParamTab* self = PARAM_TAB(object);
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


GtkWidget* param_tab_new(void)
{
    return (GtkWidget*) g_object_new(PARAM_TAB_TYPE, NULL);
}

void param_tab_set_param(ParamTab* self, PatchParamType param)
{
    GtkBox* box = GTK_BOX(self);

    PatchParamType ms1 = PATCH_PARAM_INVALID;
    PatchParamType ms2 = PATCH_PARAM_INVALID;

    switch(param)
    {
    case PATCH_PARAM_AMPLITUDE:
    case PATCH_PARAM_PANNING:
        ms1 = PATCH_PARAM_AMPLITUDE;
        ms2 = PATCH_PARAM_PANNING;
        break;

    case PATCH_PARAM_CUTOFF:
    case PATCH_PARAM_RESONANCE:
        ms1 = PATCH_PARAM_CUTOFF;
        ms2 = PATCH_PARAM_RESONANCE;
        break;

    case PATCH_PARAM_PITCH:
        ms1 = PATCH_PARAM_PITCH;
        break;

    case PATCH_PARAM_LFO_FREQ:
        ms1 = PATCH_PARAM_LFO_FREQ;
        break;

    default:
        debug ("unrecognised parameter for param tab\n");
        return;
    }

    if (ms1 != PATCH_PARAM_INVALID)
    {
        self->modsect1 = mod_section_new();
        mod_section_set_param(MOD_SECTION(self->modsect1), ms1);
        gtk_box_pack_start(box, self->modsect1, FALSE, FALSE, 0);
        gtk_widget_show(self->modsect1);
    }

    if (ms2 != PATCH_PARAM_INVALID)
    {
        self->modsect2 = mod_section_new();
        mod_section_set_param(MOD_SECTION(self->modsect2), ms2);
        gtk_box_pack_start(box, self->modsect2, FALSE, FALSE, 0);
        gtk_widget_show(self->modsect2);
    }
}

void param_tab_set_patch(ParamTab* self, int patch)
{
    if (self->modsect1)
        mod_section_set_patch(MOD_SECTION(self->modsect1), patch);

    if (self->modsect2)
        mod_section_set_patch(MOD_SECTION(self->modsect2), patch);
}

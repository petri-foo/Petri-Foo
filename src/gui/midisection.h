#ifndef __MIDI_SECTION__
#define __MIDI_SECTION__

#include <gtk/gtk.h>
#include <libgnomecanvas/libgnomecanvas.h>

G_BEGIN_DECLS

#define MIDI_SECTION_TYPE (midi_section_get_type())
#define MIDI_SECTION(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), MIDI_SECTION_TYPE, MidiSection))
#define MIDI_SECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), MIDI_SECTION_TYPE, MidiSectionClass))
#define IS_MIDI_SECTION(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MIDI_SECTION_TYPE))
#define IS_MIDI_SECTION_CLASS(klass) (G_TYPE_CHECK_INSTANCE_TYPE ((klass), MIDI_SECTION_TYPE))

typedef struct _MidiSectionClass MidiSectionClass;
typedef struct _MidiSection MidiSection;

struct _MidiSection
{
    GtkVBox parent;

    /*< private >*/
    int patch;
    GtkAdjustment* adj;
    GtkWidget* keyboard;
    GnomeCanvasItem* range;
    GnomeCanvasItem* note;
    GnomeCanvas* canvas;
    gboolean ignore;
};

struct _MidiSectionClass
{
    /*< private >*/
    GtkVBoxClass parent_class;
};

GType midi_section_get_type(void);
GtkWidget* midi_section_new(void);
void midi_section_set_patch(MidiSection* self, int patch);

G_END_DECLS

#endif /* __MIDI_SECTION__ */

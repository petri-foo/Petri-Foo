#ifndef __CHANNEL_SECTION__
#define __CHANNEL_SECTION__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define CHANNEL_SECTION_TYPE \
    (channel_section_get_type())

#define CHANNEL_SECTION(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CHANNEL_SECTION_TYPE, \
                                        ChannelSection))

#define IS_CHANNEL_SECTION(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CHANNEL_SECTION_TYPE))

#define CHANNEL_SECTION_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CHANNEL_SECTION_TYPE, \
                                       ChannelSectionClass))

#define IS_CHANNEL_SECTION_CLASS(klass) \
    (G_TYPE_CHECK_INSTANCE_TYPE((klass), CHANNEL_SECTION_TYPE))


typedef struct _ChannelSectionClass ChannelSectionClass;
typedef struct _ChannelSection      ChannelSection;


struct _ChannelSection
{
    GtkVBox parent;

    /* <private> */
    int patch;
    GtkWidget* chan_sb;
};


struct _ChannelSectionClass
{
    GtkVBoxClass parent_class;
};


GType       channel_section_get_type(void);

GtkWidget*  channel_section_new(void);

void        channel_section_set_patch(ChannelSection* self, int patch);
int         channel_section_get_channel(ChannelSection* self);

G_END_DECLS

#endif /* __CHANNEL_SECTION__ */

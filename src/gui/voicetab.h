#ifndef __VOICE_TAB__
#define __VOICE_TAB__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define VOICE_TAB_TYPE \
    (voice_tab_get_type())

#define VOICE_TAB(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VOICE_TAB_TYPE, VoiceTab))

#define IS_VOICE_TAB(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VOICE_TAB_TYPE))

#define VOICE_TAB_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), VOICE_TAB_TYPE, VoiceTabClass))

#define IS_VOICE_TAB_CLASS(klass) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((klass), VOICE_TAB_TYPE))


typedef struct _VoiceTabClass VoiceTabClass;
typedef struct _VoiceTab      VoiceTab;


struct _VoiceTab
{
    GtkVBox parent;
};

struct _VoiceTabClass
{
    GtkVBoxClass parent_class;
};

GType       voice_tab_get_type(void);
GtkWidget*  voice_tab_new(void);
void        voice_tab_set_patch(VoiceTab* self, int patch);


G_END_DECLS


#endif /* __VOICE_TAB__ */

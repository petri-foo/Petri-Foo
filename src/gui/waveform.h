#ifndef __WAVEFORM_H__
#define __WAVEFORM_H__

#include <gtk/gtk.h>

#define WAVEFORM_TYPE           (waveform_get_type ())
#define WAVEFORM(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                    WAVEFORM_TYPE, Waveform))

#define IS_WAVEFORM(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                    WAVEFORM_TYPE))

#define WAVEFORM_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                    WAVEFORM_TYPE, WaveformClass))

#define IS_WAVEFORM_CLASS(klass)(G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                    WAVEFORM_TYPE))

#define WAVEFORM_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                    WAVEFORM_TYPE,  WaveformClass))


typedef struct _Waveform      Waveform;
typedef struct _WaveformClass WaveformClass;


struct _Waveform
{
     GtkDrawingArea parent_instance;
};


struct _WaveformClass
{
    GtkDrawingAreaClass parent_class;
    void (*changed)(Waveform* self); /* <private> */
};


GType      waveform_get_type        (void);

GtkWidget* waveform_new             (void);

void       waveform_set_patch       (Waveform* wf, int id);
void       waveform_set_size        (Waveform* wf, int w, int h);
void       waveform_set_range       (Waveform* wf, float start, float stop);
void       waveform_set_interactive (Waveform* wf, gboolean interactive);
int        waveform_get_patch       (Waveform* wf);
void       waveform_get_size        (Waveform* wf, int* w, int* h);
void       waveform_get_range       (Waveform* wf, float* start,
                                                   float* stop);

gboolean   waveform_get_interactive (Waveform* wf);

#endif /* __WAVEFORM_H__ */

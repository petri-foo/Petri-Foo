#ifndef __WAVEFORM_H__
#define __WAVEFORM_H__

#include <gtk/gtk.h>

#define WAVEFORM(obj)         GTK_CHECK_CAST(obj, waveform_get_type( ), Waveform)
#define WAVEFORM_CLASS(klass) GTK_CHECK_CLASS_CAST(klass, waveform_get_type( ), WaveformClass)
#define IS_WAVEFORM(obj)      GTK_CHECK_TYPE(obj, waveform_get_type( ))

typedef struct _waveform       Waveform;
typedef struct _waveform_class WaveformClass;

/* instance definition */
struct _waveform
{
     GtkWidget parent_object;

     gboolean interactive;
     int width, height;
     float range_start, range_stop;
     int patch;
};

/* class definition */
struct _waveform_class
{
     GtkWidgetClass parent_class;
};

GtkWidget* waveform_new             (int id, int w, int h, gboolean interactive);
GType      waveform_get_type        ( );
void       waveform_draw            (Waveform* wf);
void       waveform_set_patch       (Waveform* wf, int id);
void       waveform_set_size        (Waveform* wf, int w, int h);
void       waveform_set_range       (Waveform* wf, float start, float stop);
void       waveform_set_interactive (Waveform* wf, gboolean interactive);
int        waveform_get_patch       (Waveform* wf);
void       waveform_get_size        (Waveform* wf, int* w, int* h);
void       waveform_get_range       (Waveform* wf, float* start, float* stop);
gboolean   waveform_get_interactive (Waveform* wf);

#endif /* __WAVEFORM_H__ */

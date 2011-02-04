#ifndef __DRIVER_H__
#define __DRIVER_H__

#include <gtk/gtk.h>

/* error codes */
enum
{
     DRIVER_ERR_ID = -1,
     DRIVER_ERR_OTHER = -2
};

/* magic numbers */
enum
{
     DRIVER_DEFAULT_SAMPLERATE = 44100
};
     
/* public class definition for drivers */
typedef struct _Driver
{
     void        (*init)          ( );
     int         (*start)         ( );
     int         (*stop)          ( );
     int         (*getrate)       ( );
     int         (*getperiodsize) ( );
     GtkWidget*  (*getwidget)     ( );
     const char* (*getname)       ( );
     void*       (*getid)         ( );
}
Driver;

void        driver_init          ( );
int         driver_start         (int id);
void        driver_stop          ( );
int         driver_get_count     ( );
const char* driver_get_name      (int id);
GtkWidget*  driver_get_widget    (int id);
void*       driver_get_client_id (int id);

/* this function should only be called by drivers when they start
 * and/or their samplerate changes */
int driver_set_samplerate (int rate);

/* this function should only be called by drivers when they start
 * and/or ther buffersize changes */
int driver_set_buffersize (int nframes);

#endif /* __DRIVER_H__ */

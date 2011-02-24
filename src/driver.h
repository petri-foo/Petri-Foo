#ifndef __DRIVER_H__
#define __DRIVER_H__


/*  driver has been fixed to only handle JACK
    more work should be done to clear the debris scattered
    from when there was a choice between JACK and ALSA, but
    for now, i'd rather have things working.
*/


enum
{
     DRIVER_ERR_ID = -1,
     DRIVER_ERR_OTHER = -2
};


enum {
     DRIVER_DEFAULT_SAMPLERATE = 44100
};


/* public class definition for drivers */
typedef struct _Driver
{
     void        (*init)          (void);
     int         (*start)         (void);
     int         (*stop)          (void);
     int         (*getrate)       (void);
     int         (*getperiodsize) (void);
     const char* (*getname)       (void);
     void*       (*getid)         (void);

} Driver;


void        driver_init           (void);
int         driver_restart        (void);
int         driver_start          (void);
void        driver_stop           (void);
int         driver_get_count      (void);
const char* driver_get_name       (void);
const char* driver_get_client_name(void);

/* this function should only be called by drivers when they start
 * and/or their samplerate changes */
int driver_set_samplerate (int rate);


/* this function should only be called by drivers when they start
 * and/or ther buffersize changes */
int driver_set_buffersize (int nframes);


#endif /* __DRIVER_H__ */

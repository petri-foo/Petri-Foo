/*  Petri-Foo is a fork of the Specimen audio sampler.

    Original Specimen author Pete Bessman
    Copyright 2005 Pete Bessman
    Copyright 2011 James W. Morris

    This file is part of Petri-Foo.

    Petri-Foo is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.

    Petri-Foo is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Petri-Foo.  If not, see <http://www.gnu.org/licenses/>.

    This file is a derivative of a Specimen original, modified 2011
*/


#ifndef __DRIVER_H__
#define __DRIVER_H__


#include <stdbool.h>


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
bool        driver_running        (void);
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

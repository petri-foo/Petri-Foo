#ifndef __TICKS_H__
#define __TICKS_H__


#include <jack/jack.h>


typedef jack_nframes_t Tick;


void    ticks_set_samplerate(int rate);
Tick    ticks_secs_to_ticks(float secs);


#endif /* __TICKS_H__ */

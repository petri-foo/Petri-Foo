#include <sys/time.h>
#include "petri-foo.h"
#include "driver.h"
#include "ticks.h"


static int samplerate = DRIVER_DEFAULT_SAMPLERATE;


Tick ticks_secs_to_ticks (float secs)
{
     return samplerate * secs;
}


void ticks_set_samplerate (int rate)
{
     samplerate = rate;
}

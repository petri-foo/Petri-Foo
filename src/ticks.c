#include <sys/time.h>
#include "petri-foo.h"
#include "driver.h"
#include "ticks.h"


static int samplerate = -1;


Tick ticks_secs_to_ticks (float secs)
{
     return samplerate * secs;
}


void ticks_set_samplerate (int rate)
{
     samplerate = rate;
}

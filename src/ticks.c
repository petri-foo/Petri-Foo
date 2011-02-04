#include <sys/time.h>
#include "petri-foo.h"
#include "driver.h"
#include "ticks.h"

static int samplerate = DRIVER_DEFAULT_SAMPLERATE;

/* gets the current tick (sample count).  N.B., we divide tv_usec by
   1,000,000 to convert to full seconds */
Tick ticks_get_ticks ( )
{
     struct timeval time;
     Tick val;
     static Tick offset = -1;

     if (offset == -1)
     {
	  if (gettimeofday (&time, NULL) < 0)
	  {
	       offset = 0;
	  }
	  else
	  {
	       offset = (time.tv_sec + time.tv_usec / 1000000.0);
	  }
     }

     if (gettimeofday (&time, NULL) < 0)
     {
	  errmsg ("Failed to get time\n");
	  return 0;
     }

     val = (time.tv_sec + time.tv_usec / 1000000.0 - offset) * samplerate;
     
     return val;
}

/* convert seconds to ticks */
Tick ticks_secs_to_ticks (float secs)
{
     return samplerate * secs;
}

/* convert ticks to seconds */
float ticks_ticks_to_secs (Tick tick)
{
     return tick * 1.0 / samplerate;
}

void ticks_set_samplerate (int rate)
{
     samplerate = rate;
}

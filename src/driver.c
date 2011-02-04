#include "petri-foo.h"
#include "driver.h"
#include "lfo.h"
#include "patch.h"
#include "mixer.h"
#include "ticks.h"

/* available drivers */
extern Driver  jack_driver;
extern Driver  alsa_driver;
static Driver* drivers[] = { &jack_driver, &alsa_driver, NULL };

/* number of drivers available (we set this var in driver_init ( ) */
static int ndrivers = 0;

/* which driver we are currently using */
static int curdriver = -1;

void driver_init ( )
{
     int i;

     for (i = 0; drivers[i] != NULL; i++)
	  drivers[i]->init ( );

     if ((ndrivers = i) < 0)
	  ndrivers = 0;
}

int driver_start (int id)
{
     if (id < 0 || id >= ndrivers)
	  return DRIVER_ERR_ID;

     if (ndrivers <= 0)
	  return DRIVER_ERR_OTHER;

     if (curdriver >= 0)
	  drivers[curdriver]->stop ( );

     curdriver = id;

     if(id==0){
     	return drivers[id]->start ( );
     }
     return drivers[id]->start ( );
}

void driver_stop ( )
{
     if (curdriver < 0)
	  return;

     drivers[curdriver]->stop ( );
}

int driver_get_count ( )
{
     return ndrivers;
}

const char* driver_get_name (int id)
{
     if (id < 0 || id >= ndrivers)
	  return NULL;

     return drivers[id]->getname ( );
}

GtkWidget* driver_get_widget (int id)
{
     if (id < 0 || id >= ndrivers)
	  return NULL;

     return drivers[id]->getwidget ( );
}

int driver_set_samplerate (int rate)
{
     if (rate <= 0)
     {
	  debug ("can't accept samplerate %d\n", rate);
	  return DRIVER_ERR_OTHER;
     }

     /* tell everybody our (potentially new) samplerate */
       lfo_set_samplerate (rate);
     patch_set_samplerate (rate);
     mixer_set_samplerate (rate);
     ticks_set_samplerate (rate);
     return 0;
}

int driver_set_buffersize (int nframes)
{
     if (nframes <= 0)
     {
	  debug ("can't accept buffersize %d\n", nframes);
	  return DRIVER_ERR_OTHER;
     }

     /* tell everybody our (potentially new) buffersize */
     patch_set_buffersize (nframes);
     return 0;
}

void* driver_get_client_id (int id)
{
	return drivers[id]->getid ( );
}

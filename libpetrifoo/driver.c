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


#include "petri-foo.h"
#include "driver.h"
#include "lfo.h"
#include "patch.h"
#include "mixer.h"
#include "ticks.h"
#include "patch_util.h"


#include <strings.h> /* strcasecmp */


/* available drivers */
extern Driver  jack_driver;
static Driver* drivers[] = { &jack_driver, NULL };
static int ndrivers = 0;
static int curdriver = -1;


void driver_init(void)
{
    int i;

    for (i = 0; drivers[i] != NULL; i++)
        drivers[i]->init();

    if ((ndrivers = i) < 0)
        ndrivers = 0;
}

int driver_restart(void)
{
    driver_stop();
    return driver_start();
}

int driver_start(void)
{
    if (ndrivers <= 0)
        return DRIVER_ERR_OTHER;

    if (curdriver >= 0)
        drivers[curdriver]->stop();

    curdriver = 0;

    return drivers[curdriver]->start();
}

void driver_stop(void)
{
    if (curdriver < 0)
        return;

    drivers[curdriver]->stop();
    curdriver = -1;
}


bool driver_running(void)
{
    return (curdriver >= 0);
}


int driver_get_count(void)
{
    return ndrivers;
}

const char* driver_get_name(void)
{
    if (curdriver < 0)
        return 0;

    return drivers[curdriver]->getname();
}


int driver_set_samplerate(int rate)
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

int driver_set_buffersize(int nframes)
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

const char* driver_get_client_name(void)
{
    if (curdriver < 0)
        return 0;

    return drivers[curdriver]->getid();
}


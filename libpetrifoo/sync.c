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
#include "midi.h"
#include "patch.h"
#include "patch_util.h"
#include "sync.h"

static SyncMethod sync_method = SYNC_METHOD_MIDI;

void sync_start_midi (float bpm)
{
    if (bpm <= 0)
    {
        debug("ignoring midi sync start with invalid bpm: %f\n", bpm);
        return;
    }

    if (sync_method == SYNC_METHOD_MIDI)
    {
        debug("midi sync started, bpm: %f\n", bpm);
        patch_sync (bpm);
    }
    else
    {
        debug("ignoring midi sync start\n");
    }
}

void sync_start_jack (float bpm)
{
    if (bpm <= 0)
    {
        debug("ignoring jack sync start with invalid bpm: %f\n", bpm);
        return;
    }

    if (sync_method == SYNC_METHOD_JACK)
    {
        debug("jack sync started, bpm: %f\n", bpm);
        patch_sync (bpm);
    }
    else
    {
        debug("ignoring jack sync start\n");
    }
}

void sync_set_method (SyncMethod method)
{
    if (method == SYNC_METHOD_JACK)
    {
        debug("setting sync to JACK\n");
        sync_method = SYNC_METHOD_JACK;
    }
    else
    {
        debug("setting sync to MIDI\n");
        sync_method = SYNC_METHOD_MIDI;
    }
}

SyncMethod sync_get_method(void)
{
    return sync_method;
}

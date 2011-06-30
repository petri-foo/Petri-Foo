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


#ifndef __SYNC_H__
#define __SYNC_H__

enum
{
     SYNC_DEFAULT_TEMPO = 120,
};

typedef enum
{
     SYNC_METHOD_MIDI,
     SYNC_METHOD_JACK,
}
SyncMethod;

void sync_start_midi (float bpm);
void sync_start_jack (float bpm);
void sync_set_method (SyncMethod method);

#endif /* __SYNC_H__ */


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


#ifndef __MIDI_H__
#define __MIDI_H__

/* magic numbers */
enum
{
    MIDI_ERR_SEQ = -1,	/* error opening sequencer */
    MIDI_ERR_PORT = -2,	/* error creating port */
    MIDI_NOTES = 128,	/* number of notes */
    MIDI_CHANS = 16,	/* number of channels */
};

int  midi_start         (void);
int  midi_get_client_id (void);
void midi_stop          (void);

#endif /* __MIDI_H__ */

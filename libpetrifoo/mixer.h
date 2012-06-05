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


#ifndef __MIXER_H__
#define __MIXER_H__

#include "ticks.h"


#include <jack/jack.h>


void    mixer_flush             (void);
void    mixer_init              (void);

void    mixer_set_jack_client   (jack_client_t*);

void    mixer_mixdown           (float* buf, int frames);
void    mixer_note_off          (int chan, int note);
void    mixer_note_off_with_id  (int id,   int note);
void    mixer_note_on           (int chan, int note,  float vel);
void    mixer_note_on_with_id   (int id,   int note,  float vel);
void    mixer_control           (int chan, int param, float value);
void    mixer_direct_note_off   (int chan, int note,  Tick tick);
void    mixer_direct_note_on    (int chan, int note,  float vel, Tick tick);
void    mixer_direct_control    (int chan, int param, float value, Tick tick);
void    mixer_preview           (char* name,
        /* zero for non-raw data */ int raw_samplerate,
        /* zero for non-raw data */ int raw_channels,
        /* zero for non-raw data */ int sndfile_format,
                                    int resample_sndfile);

int     mixer_set_amplitude     (float amplitude);
float   mixer_get_amplitude     (void);
void    mixer_set_samplerate    (int rate);
void    mixer_shutdown          (void);


#endif /* __MIXER_H__ */

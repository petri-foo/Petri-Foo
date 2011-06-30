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


#ifndef __SAMPLE_H__
#define __SAMPLE_H__


#include <stdbool.h>


typedef struct _RAW_FORMAT
{
    const int format;
    const char* str;

} raw_format;


typedef struct _Sample Sample;


struct _Sample
{
    /* Public */
    float* sp;          /* samples pointer */
    int frames;         /* number of frames (not samples). 
                         * samples whose length exceeds int on the
                         * system they're to run on are disallowed.
                         * (on 64bit, this still is OTT long).
                         */

    int raw_samplerate; /* if the sample was a regular sound file ie */
    int raw_channels;   /* with a header, then these fields will be  */
    int sndfile_format; /* zero. if raw, they will be non-zero       */

    char*   filename;

    bool    default_sample;
};


Sample*     sample_new      (void);
void        sample_free     (Sample*);

/* sample_shallow_copy does copy filename */
void        sample_shallow_copy(Sample* dest, const Sample* src);

/*  sample_deep_copy, ie copy (possibly resampled) audio data aswell */
int         sample_deep_copy(Sample* dest, const Sample* src);


int         sample_load_file(Sample*, const char* name, int rate,
    /* zero for non-raw data */         int raw_samplerate,
    /* zero for non-raw data */         int raw_channels,
    /* zero for non-raw data */         int sndfile_format);


void        sample_free_data(Sample*); /* free's samples and filename */
int         sample_default  (Sample*, int rate);


#endif /* __SAMPLE_H__ */

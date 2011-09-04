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


#ifndef __PATCH_UTIL_H__
#define __PATCH_UTIL_H__


#include "patch.h"
#include "sample.h"


enum { USER_PATCH, DEFAULT_PATCH };


/* utility functions */
int         patch_create          (void);
int         patch_create_default  (void);

void        patch_destroy         (int id);
void        patch_destroy_all     (void);

int         patch_count           (void);
int         patch_dump            (int** dump);
int         patch_duplicate       (int id);
int         patch_flush           (int id);
void        patch_flush_all       (void);
const char* patch_strerror        (int error);

int         patch_sample_load     (int id, const char* file,
            /* 0 for non-raw data */    int raw_samplerate,
            /* 0 for non-raw data */    int raw_channels,
            /* 0 for non-raw data */    int sndfile_format);

int         patch_sample_load_from(int dest_id, int src_id);

const Sample* patch_sample_data(int id);

void        patch_sample_unload   (int id);


void        patch_set_buffersize  (int nframes);
void        patch_set_samplerate  (int rate);
int         patch_get_samplerate  (void);

void        patch_shutdown        (void);
void        patch_sync            (float bpm);
int         patch_verify          (int id);


#endif /* __PATCH_UTIL_H__ */

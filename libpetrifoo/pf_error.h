/*  Petri-Foo is a fork of the Specimen audio sampler.

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
#ifndef PF_ERROR_H
#define PF_ERROR_H


#include <assert.h>


enum
{
    PF_ERR_INVALID_ERROR = -1,

    /* JACK errors */
    PF_ERR_JACK_OPEN_CLIENT,
    PF_ERR_JACK_ACTIVATE,
    PF_ERR_JACK_SESSION_CB,
    PF_ERR_JACK_BUF_ALLOC,
    PF_ERR_JACK_BUF_SIZE_CHANGE,

    /* Patch errors */
    /*  (note: ID errors are only introducible via dish_file_read) */
    PF_ERR_PATCH_ID,
    PF_ERR_PATCH_COUNT,
    PF_ERR_PATCH_ALLOC,
    PF_ERR_PATCH_DUMP_ALLOC,
    PF_ERR_PATCH_PARAM_ID,
    PF_ERR_PATCH_BOOL_ID,
    PF_ERR_PATCH_FLOAT_ID,
    PF_ERR_PATCH_ENV_ID,
    PF_ERR_PATCH_LFO_ID,
    PF_ERR_PATCH_MOD_SRC_ID,
    PF_ERR_PATCH_MOD_SLOT,
    PF_ERR_PATCH_CHANNEL,
    PF_ERR_PATCH_NOTE,

    PF_ERR_PATCH_VALUE_NEGATIVE,
    PF_ERR_PATCH_VALUE_LEVEL,

    PF_ERR_PATCH_PARAM_VALUE,

    /* Sample errors */
    PF_ERR_SAMPLE_ALLOC,
    PF_ERR_SAMPLE_DEFAULT_ALLOC,
    PF_ERR_SAMPLE_ALLOC_COPY,
    PF_ERR_SAMPLE_MAX_FRAMES,
    PF_ERR_SAMPLE_RESAMPLE_MAX_FRAMES,
    PF_ERR_SAMPLE_RESAMPLE_ALLOC,
    PF_ERR_SAMPLE_CHANNEL_COUNT,
    PF_ERR_SAMPLE_CHANNEL_ALLOC,
    PF_ERR_SAMPLE_SNDFILE_FORMAT,
    PF_ERR_SAMPLE_SNDFILE_OPEN,
    PF_ERR_SAMPLE_SNDFILE_READ,
    PF_ERR_SAMPLE_SRC_SIMPLE,
};


int         pf_error_get(void);
const char* pf_error_str(int pf_error_no);

#define     pf_error(pf_error_no) \
            assert(pf_error_set(pf_error_no) == 0)

/*  pf_error_set should be called via the macro above */
int         pf_error_set(int pf_error_no);


#endif

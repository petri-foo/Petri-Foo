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

#include "pf_error.h"
#include "petri-foo.h"

#include <stdio.h>

static int last_error_no = PF_ERR_INVALID_ERROR;

/* FIXME: still not happy with this... */


int pf_error_set(int pf_error_no)
{
    int ret = 0;

    if (last_error_no != PF_ERR_INVALID_ERROR)
    {
        ret = 1;
        debug("previous error (%d:'%s') not reset, about to abort...\n",
                            last_error_no, pf_error_str(last_error_no));
    }

    last_error_no = pf_error_no;

    return ret;
}


int pf_error_get(void)
{
    int errno = last_error_no;
    last_error_no = PF_ERR_INVALID_ERROR;
    return errno;
}


const char* pf_error_str(int pf_error_no)
{
    switch(pf_error_no)
    {
    /* JACK errors */
    case PF_ERR_JACK_OPEN_CLIENT:
        return "JACK failed to open client";
    case PF_ERR_JACK_ACTIVATE:
        return "JACK failed to activate client";
    case PF_ERR_JACK_SESSION_CB:
        return "JACK failed to set session callback";
    case PF_ERR_JACK_BUF_ALLOC:
        return "JACK failed to allocate buffer";
    case PF_ERR_JACK_BUF_SIZE_CHANGE:
        return "JACK failed buffer size change";

    /* Patch errors */
    case PF_ERR_PATCH_ID:
        return "Invalid patch ID";
    case PF_ERR_PATCH_COUNT:
        return "Maximum patch count exceeded";
    case PF_ERR_PATCH_ALLOC:
        return "Patch allocation failed";
    case PF_ERR_PATCH_DUMP_ALLOC:
        return "Patch dump allocation failed";
    case PF_ERR_PATCH_PARAM_ID:
        return "Invalid param ID";
    case PF_ERR_PATCH_BOOL_ID:
        return "Invalid bool ID";
    case PF_ERR_PATCH_FLOAT_ID:
        return "Invalid float ID";
    case PF_ERR_PATCH_ENV_ID:
        return "Invalid ADSR ID";
    case PF_ERR_PATCH_LFO_ID:
        return "Invalid LFO ID";
    case PF_ERR_PATCH_MOD_SRC_ID:
        return "Invalid modulation-source ID";
    case PF_ERR_PATCH_MOD_SLOT:
        return "Invalid modulation-slot ID";
    case PF_ERR_PATCH_CHANNEL:
        return "Invalid channel number";
    case PF_ERR_PATCH_NOTE:
        return "Invalid note";
    case PF_ERR_PATCH_VALUE_NEGATIVE:
        return "Invalid negative value";
    case PF_ERR_PATCH_VALUE_LEVEL:
        return "Invalid value out of range 0.0 ~ 1.0";

    case PF_ERR_PATCH_PARAM_VALUE:
        return "Invalid parameter value";

    /* Sample errors */
    case PF_ERR_SAMPLE_ALLOC:
        return "Failed to allocate sample";
    case PF_ERR_SAMPLE_DEFAULT_ALLOC:
        return "Failed to allocate default sample";
    case PF_ERR_SAMPLE_ALLOC_COPY:
        return "Failed to allocate duplicate sample";
    case PF_ERR_SAMPLE_MAX_FRAMES:
        return "Sample is too long";
    case PF_ERR_SAMPLE_RESAMPLE_MAX_FRAMES:
        return "Sample (after resampling) is too long";
    case PF_ERR_SAMPLE_RESAMPLE_ALLOC:
        return "Failed to allocate resampling data";
    case PF_ERR_SAMPLE_CHANNEL_COUNT:
        return "Sample contains too many channels";
    case PF_ERR_SAMPLE_CHANNEL_ALLOC:
        return "Allocation for mono to stereo conversion failed";
    case PF_ERR_SAMPLE_SNDFILE_FORMAT:
        return "Sndfile format error";
    case PF_ERR_SAMPLE_SNDFILE_OPEN:
        return "Sndfile could not open sample";
    case PF_ERR_SAMPLE_SNDFILE_READ:
        return "Sndfile could not read sample";
    case PF_ERR_SAMPLE_SRC_SIMPLE:
        return "Secret Rabbit Code resample failed";
    default:
        printf("uncategorized error number: %d\n", pf_error_no);
        return "uncategorized error";
    }
}

#include "pf_error.h"

#include <assert.h>
#include "petri-foo.h"

static int last_error_no = PF_ERR_INVALID_ERROR;


void pf_error(int pf_error_no)
{
    #if DEBUG
    if (last_error_no != PF_ERR_INVALID_ERROR)
        debug("previous error not reset, about to abort...\n");
    #endif
    assert (last_error_no == PF_ERR_INVALID_ERROR);
    last_error_no = pf_error_no;
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
    case PF_ERR_PATCH_PARAM_VALUE:
        return "Invalud parameter value";

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
        return "uncategorized error";
    }
}

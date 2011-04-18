#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sndfile.h>
#include <samplerate.h>
#include "petri-foo.h"
#include "sample.h"


#include <stdbool.h>


Sample* sample_new(void)
{
    Sample* sample = malloc(sizeof(*sample));

    if (!sample)
        return 0;

    sample->sp = 0;
    sample->frames = 0;
    sample->filename = 0;

    sample->raw_samplerate = 0;
    sample->raw_channels = 0;
    sample->sndfile_format = 0;

    return sample;
}

void sample_free (Sample* sample)
{
    free(sample->filename);
    free(sample->sp);
    free(sample);
}


void sample_shallow_copy(Sample* dest, const Sample* src)
{
    dest->sp =              0;
    dest->frames =          src->frames;
    dest->raw_samplerate =  src->raw_samplerate;
    dest->raw_channels =    src->raw_channels;
    dest->sndfile_format =  src->sndfile_format;
    dest->filename =        (!src->filename) ? 0 : strdup(src->filename);
}


int sample_default(Sample* sample, int rate)
{
    int frames = rate / 8;
    float freq_st = M_PI * 2.0 / (rate / 523.251);
    float rad = 0;
    int i;
    float* tmp = malloc(frames * 2 * sizeof(float));

    if (!tmp)
    {
        errmsg("Unable to allocate space for (default) samples!\n");
        return -1;
    }

    sample->frames = frames;
    sample->sp = tmp;

    for (i = 0; i < frames; ++i)
    {
        float s = sin(rad);
        *tmp++ = s;
        *tmp++ = s;
        rad += freq_st;
    }

    sample->filename = strdup("Default");
    return 0;
}


static float* resample(float* samples,  int rate,
                                        SF_INFO* sfinfo,
                                        int* output_frames)
{
    double ratio;
    int frames;

    ratio = rate / (sfinfo->samplerate * 1.0);
    frames = (int)sfinfo->frames;

    debug ("Resampling...\n");

    int err;
    SRC_DATA src;
    float* tmp = malloc(sizeof(float) * sfinfo->frames
                                      * sfinfo->channels
                                      * ratio);
    if (!tmp)
    {
        errmsg ("Out of memory for resampling\n");
        return 0;
    }

    src.src_ratio = ratio;
    src.data_in = samples;
    src.data_out = tmp;
    src.input_frames = sfinfo->frames;
    src.output_frames = sfinfo->frames * ratio;

    frames = (int)src.output_frames;

    if (frames != src.output_frames)
    {
        errmsg("resampled sample would be too long\n");
        free(tmp);
        return 0;
    }

    err = src_simple(&src, SRC_SINC_BEST_QUALITY, sfinfo->channels);
    if (err)
    {
        errmsg("Failed to resample (%s)\n", src_strerror(err));
        free(tmp);
        return 0;
    }

    *output_frames = frames;
    return tmp;
}


static float* mono_to_stereo(float* samples, SF_INFO* sfinfo)
{
    debug ("Converting mono to stereo...\n");

    int i;
    float* tmp = malloc(sizeof(float) * sfinfo->frames * 2);

    if (!tmp)
    {
        errmsg ("Out of memory for mono to stereo conversion.\n");
        return 0;
    }

    for (i = 0; i < sfinfo->frames; i++)
        tmp[2 * i] = tmp[2 * i + 1] = samples[i];

    return tmp;
}


static float* read_audio(SNDFILE* sfp, SF_INFO* sfinfo)
{
    float* tmp;
    int frames = (int)sfinfo->frames;

    if (frames != sfinfo->frames)
    {
        errmsg("sample is too long\n");
        return 0;
    }

    if (sfinfo->channels > 2)
    {
        errmsg ("Data can't have more than 2 channels\n");
        sf_close (sfp);
        return 0;
    }

    /* set aside space for samples */
    if (!(tmp = malloc(sfinfo->frames * sfinfo->channels * sizeof(*tmp))))
    {
        errmsg ("Unable to allocate space for samples!\n");
        sf_close (sfp);
        return 0;
    }

    /* load sample file into memory */
    if (sf_readf_float(sfp, tmp, sfinfo->frames) != sfinfo->frames)
    {
        errmsg("libsndfile had problems reading file, aborting\n");
        free(tmp);
        return 0;
    }

    debug ("Read %d frames into memory.\n", (int) sfinfo->frames);

    return tmp;
}


int sample_load_file(Sample* sample, const char* name,
                                        int rate,
                                        int raw_samplerate,
                                        int raw_channels,
                                        int sndfile_format)
{
    SNDFILE* sfp;
    SF_INFO sfinfo = { 0, 0, 0, 0, 0, 0 };
    float* tmp;
    int frames;
    bool raw = (raw_samplerate || raw_channels || sndfile_format);

    if (raw)
    {
        sfinfo.samplerate = raw_samplerate;
        sfinfo.channels = raw_channels;
        sfinfo.format = sndfile_format;

        if (!sf_format_check(&sfinfo))
        {
            debug("LIBSNDFILE found error in format. aborting.\n");
            return -1;
        }
    }

    if ((sfp = sf_open (name, SFM_READ, &sfinfo)) == NULL)
    {
        debug ("libsndfile doesn't like %s\n", name);
        return -1;
    }

    if (!(tmp = read_audio(sfp, &sfinfo)))
        return -1;

    sf_close(sfp);

    if (sfinfo.samplerate != rate)
    {
        float* tmp2 = resample(tmp, rate, &sfinfo, &frames);

        if (!tmp2)
        {
            free(tmp);
            debug("failed to resample file\n");
            return -1;
        }

        free(tmp);
        tmp = tmp2;
    }
    else
        frames = sfinfo.frames;

    if (sfinfo.channels == 1)
    {
        float* tmp2 = mono_to_stereo(tmp, &sfinfo);

        if (!tmp2)
        {
            free(tmp);
            debug("failed to convert mono to stereo\n");
            return -1;
        }

        free(tmp);
        tmp = tmp2;
    }

    free(sample->sp);
    free(sample->filename);

    sample->filename = strdup(name);

    sample->sp = tmp;
    sample->frames = frames;

    if (raw)
    {
        sample->raw_samplerate = raw_samplerate;
        sample->raw_channels = raw_channels;
        sample->sndfile_format = sndfile_format;
    }
    else
    {
        sample->raw_samplerate = 0;
        sample->raw_channels = 0;
        sample->sndfile_format = 0;
    }

    return 0;
}


void sample_free_data(Sample* sample)
{
    free(sample->sp);
    free(sample->filename);
    sample->sp = 0;
    sample->filename = 0;
}

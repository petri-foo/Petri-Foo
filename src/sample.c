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


#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sndfile.h>
#include <samplerate.h>
#include "petri-foo.h"
#include "sample.h"
#include "lfo.h"


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

    sample->default_sample = false;

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

    debug("src->filename:[%p] \n", src->filename);

    dest->filename =        (!src->filename) ? 0 : strdup(src->filename);
    dest->default_sample =  src->default_sample;
}


int sample_default(Sample* sample, int rate)
{
    int         frames = rate / 8;
    float*      tmp;
    LFO*        lfo;
    LFOParams   lfopar;
    int         i;

    float const*    lfo_out;

    if (!(tmp = malloc(frames * 2 * sizeof(*tmp))))
    {
        errmsg("Unable to allocate space for (default) samples!\n");
        return -1;
    }

    sample->frames = frames;
    sample->sp = tmp;

    lfo = lfo_new();
    lfo_init(lfo);
    lfo_params_init(&lfopar, 523.251, LFO_SHAPE_SAW);
    lfo_trigger(lfo, &lfopar);

    lfo_out = lfo_output(lfo);

    for (i = 0; i < frames; ++i)
    {
        lfo_tick(lfo);
        *tmp++ = *lfo_out;
        *tmp++ = *lfo_out;

        if (*lfo_out < -1.0 || *lfo_out > 1.0)
        {
            debug("lfo output %1.3f clips -1.0 || 1.0\n", *lfo_out);
        }
    }

    lfo_free(lfo);

    sample->filename = strdup("Default");
    sample->default_sample = true;

    return 0;
}


static float* resample(float* samples, int rate, SF_INFO* sfinfo)
{
    double ratio;
    int frames;

    ratio = rate / (sfinfo->samplerate * 1.0);
    frames = (int)sfinfo->frames;

    debug("Resampling...\n");

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

    sfinfo->frames = frames;

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

    if ((sfp = sf_open(name, SFM_READ, &sfinfo)) == NULL)
    {
        debug ("libsndfile doesn't like %s\n", name);
        return -1;
    }

    if (!(tmp = read_audio(sfp, &sfinfo)))
        return -1;

    sf_close(sfp);

    if (sfinfo.samplerate != rate)
    {
        float* tmp2 = resample(tmp, rate, &sfinfo);

        if (!tmp2)
        {
            free(tmp);
            debug("failed to resample file\n");
            return -1;
        }

        free(tmp);
        tmp = tmp2;
    }

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

debug("freeing old sample data\n");

    free(sample->sp);
    free(sample->filename);

debug("setting new sample data\n");

    sample->filename = strdup(name);

    sample->sp = tmp;
    sample->frames = sfinfo.frames;

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

    sample->default_sample = false;

debug("sample loaded\n");

    return 0;
}


void sample_free_data(Sample* sample)
{
    free(sample->sp);
    free(sample->filename);
    sample->sp = 0;
    sample->filename = 0;
    sample->default_sample = false;
}


int sample_deep_copy(Sample* dest, const Sample* src)
{
    size_t bytes = sizeof(float) * src->frames * 2 /* channels */;

    sample_shallow_copy(dest, src);

    dest->sp = malloc(bytes);

    if (!dest->sp)
    {
        debug("Failed to allocate memory for sample data copy\n");
        return -1;
    }

    memcpy(dest->sp, src->sp, bytes);

    return 0;
}

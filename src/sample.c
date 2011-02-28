#include <stdlib.h>
#include <string.h>
#include <sndfile.h>
#include <samplerate.h>
#include "petri-foo.h"
#include "sample.h"

Sample* sample_new ( )
{
     Sample* sample;

     sample = g_new0 (Sample, 1);

     sample->sp     = NULL;
     sample->frames = 0;
     sample->file   = g_string_new ("");

     return sample;
}

void sample_free (Sample* sample)
{
     g_string_free (sample->file, TRUE);
     g_free (sample->sp);
     g_free (sample);
}

int sample_load_file (Sample* sample, const char* name, int rate)
{
     SNDFILE* sfp;
     SF_INFO sfinfo;
     SRC_DATA data;
     float* tmp;
     double ratio;
     int err, i;

     if ((sfp = sf_open (name, SFM_READ, &sfinfo)) == NULL)
     {
	  debug ("libsndfile doesn't like %s\n", name);
	  return -1;
     }

    if ((sf_count_t)(int)sfinfo.frames != sfinfo.frames)
    {
        errmsg("sample is too long\n");
        return -1;
    }

     if (sfinfo.channels > 2)
     {
	  errmsg ("Data can't have more than 2 channels\n");
	  sf_close (sfp);
	  return -1;
     }

     /* set aside space for samples */
     if ((tmp =  malloc (sfinfo.frames * sfinfo.channels * sizeof (float))) == NULL)
     {
	  errmsg ("Unable to allocate space for samples!\n");
	  sf_close (sfp);
	  return -1;
     }

     /* free existing samples, if any, and point to new space */
     if (sample->sp != NULL)
	  free (sample->sp);
     sample->sp = tmp;
     sample->frames = (int) sfinfo.frames;
     g_string_assign (sample->file, name);

     /* load sample file into memory */
     if (sf_readf_float (sfp, sample->sp, sfinfo.frames) != sfinfo.frames)
     {
	  errmsg ("Didn't read correct number of frames into memory... this is bad...\n");
     }
     else
     {
	  debug ("Read %d frames into memory.\n", (int) sfinfo.frames);
     }

     /* resample if necessary */
     if ((ratio = rate / (sfinfo.samplerate * 1.0)) != 1.0)
     {
	  debug ("Resampling... \n");
	  if ((tmp =
	       malloc (sizeof (float) * sfinfo.frames * sfinfo.channels *
		       ratio)) == NULL)
	  {
	       errmsg ("Couldn't malloc( ) output buffer, unable to samplerate convert\n");
	       sf_close (sfp);
	       return -1;
	  }
	  data.src_ratio = ratio;
	  data.data_in = sample->sp;
	  data.data_out = tmp;
	  data.input_frames = sfinfo.frames;
	  data.output_frames = sfinfo.frames * ratio;
	  if ((err =
	       src_simple (&data, SRC_SINC_BEST_QUALITY,
			   sfinfo.channels)) != 0)
	  {
	       errmsg ("Failed to resample (%s)\n", src_strerror (err));
	       free (tmp);
	       sf_close (sfp);
	       return -1;
	  }
	  if (sample->sp != NULL)
	       free (sample->sp);
	  sample->sp = tmp;
	  sample->frames = data.output_frames_gen;
	  debug ("done.\n");
     }

     /* convert mono samples to stereo if necessary */
     if (sfinfo.channels == 1)
     {
	  debug ("Converting mono to stereo... ");
	  if ((tmp = malloc (sizeof (float) * sample->frames * 2)) == NULL)
	  {
	       errmsg ("Couldn't malloc( ) output buffer, unable to convert to stereo\n");
	       sf_close (sfp);
	       return -1;
	  }
	  for (i = 0; i < sample->frames; i++)
	  {
	       tmp[2 * i] = sample->sp[i];
	       tmp[2 * i + 1] = sample->sp[i];
	  }
	  if (sample->sp != NULL)
	       free (sample->sp);
	  sample->sp = tmp;
	  debug ("done\n");
     }

     sf_close (sfp);
     return 0;
}

void sample_free_file (Sample* sample)
{
     g_free (sample->sp);
     sample->sp = NULL;
     g_string_assign (sample->file, "");
}

const char* sample_get_file (Sample* sample)
{
     return sample->file->str;
}



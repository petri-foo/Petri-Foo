#ifndef __SAMPLE_H__
#define __SAMPLE_H__



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

    char* filename;
};


Sample*     sample_new      (void);
void        sample_free     (Sample*);

/* sample_shallow_copy does copy filename */
void        sample_shallow_copy(Sample* dest, const Sample* src);


int         sample_load_file(Sample*, const char* name, int rate,
    /* zero for non-raw data */         int raw_samplerate,
    /* zero for non-raw data */         int raw_channels,
    /* zero for non-raw data */         int sndfile_format);

void        sample_free_data(Sample*); /* free's samples and filename */
int         sample_default  (Sample*, int rate);


#endif /* __SAMPLE_H__ */

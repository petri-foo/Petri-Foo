#ifndef __SAMPLE_H__
#define __SAMPLE_H__

#include <glib.h>


typedef struct _Sample Sample;


struct _Sample
{
     /* Public */
     float* sp;         /* samples pointer */
     int frames;        /* number of frames (not samples) 
                         * sorry, samples whose length exceeds int on
                         * the system they're to run on are disallowed.
                         */

     /* Private */
     GString* file;     /* name of current file */
};


Sample*     sample_new       (void);
void        sample_free      (Sample*);
int         sample_load_file (Sample* sample, const char* name, int rate);
void        sample_free_file (Sample* sample);
const char* sample_get_file  (Sample* sample);
int         sample_default   (Sample* sample, int rate);


#endif /* __SAMPLE_H__ */

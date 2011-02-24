#ifndef __SAMPLE_H__
#define __SAMPLE_H__

#include <glib.h>

typedef struct _Sample
{
     /* Public */
     float* sp;			/* samples pointer */
     int    frames;		/* number of frames (not samples)
				 * pointed to by sp */

     /* Private */
     GString* file;		/* name of current file */
}
Sample;

Sample*     sample_new       (void);
void        sample_free      (Sample* );
int         sample_load_file (Sample* sample, const char* name, int rate);
void        sample_free_file (Sample* sample);
const char* sample_get_file  (Sample* sample);

#endif /* __SAMPLE_H__ */

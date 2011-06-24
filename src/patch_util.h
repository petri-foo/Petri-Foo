#ifndef __PATCH_UTIL_H__
#define __PATCH_UTIL_H__


#include "patch.h"
#include "sample.h"


enum { USER_PATCH, DEFAULT_PATCH };


/* utility functions */
int         patch_create          (void);
int         patch_create_default  (void);

int         patch_destroy         (int id);
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
void        patch_shutdown        (void);
void        patch_sync            (float bpm);
int         patch_verify          (int id);


#endif /* __PATCH_UTIL_H__ */

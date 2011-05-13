#ifndef NAMES_H
#define NAMES_H


#include <stdbool.h>


typedef struct _id_name
{
    int id;
    char* name;

} id_name;


id_name*        id_name_new(int id, const char* name);
void            id_name_init(id_name*, int id, const char* name);
id_name*        id_name_dup(const id_name*);
void            id_name_free(id_name*);

/*  create a sequence of id/name pairs. if name_fmt has a '%d' within it,
 *  then the names will be sequentially numbered beginning at 1.
 */
id_name*        id_name_sequence(id_name* start, int first_id, int count,
                                                    const char* name_fmt);


void            names_create(void);
void            names_destroy(void);


const char**    names_lfo_shapes_get(void);
int             names_lfo_shapes_id_from_str(const char*);

const char**    names_params_get(void);
int             names_params_id_from_str(const char*);


/*  a list of supported sample file formats along with their
    libsoundfile format ID. The function id_name_array_free
    is provided for your convenience in order to free the
    return value of this function.
 */
id_name* names_sample_raw_format_get(void);

#endif

#ifndef DISH_FILE_H
#define DISH_FILE_H


/*  recommended file extension for petri-foo data files:
    (includes dot)
 */

const char* dish_file_extension(void);

int         dish_file_read(const char* name);
int         dish_file_write(const char* name);


#endif

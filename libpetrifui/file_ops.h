/*  Petri-Foo is a fork of the Specimen audio sampler.

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
*/


#ifndef FILE_OPS_H
#define FILE_OPS_H


/*  file_ops_join_str:
        joins two strings ensuring only a single ocurrence of
        the join between them.

    return value:
            new string on success
            0 on failure.

    notes:
        if str1 is 0 then return duplicate of str2 if set
        if str2 is 0 then return duplicate of str1 if set
 */
char*   file_ops_join_str(const char* str1, char join, const char* str2);

int     file_ops_path_split(const char* path,   char** return_dir,
                                                char** return_file);


#define file_ops_make_path(dir, file) \
        file_ops_join_str((dir), '/', (file))

#define file_ops_add_ext(file, ext) \
        file_ops_join_str((file), '.', (ext))

/*
char*   file_ops_make_path(const char* dir, const char* file);
char*   file_ops_add_extension(const char* file, const char* ext);
*/
char*   file_ops_make_relative(const char* path, const char* parent);
char*   file_ops_dir_to_hash(const char* path);

char*   file_ops_mkdir(const char* dir, const char* parent);

/*  file_ops_hash_mkdir:
        creates a hash of path 'path', and creates a directory
        named with the hash, in dir 'parent'. additionally, it
        writes path 'path' into file 'pathinfo.txt' written
        inside the dir created.

    returns
        path to directory created on sucess
        NULL on failure.
 */
char*   file_ops_hash_mkdir(const char* path, const char* parent);


/*  file_ops_sample_path_mangle:
        calls file_ops_hash_mkdir before creating symlink-to-sample
        within hash_dir. returns path to symlink relative to bank_dir.
 */
char*   file_ops_sample_path_mangle(const char* samplepath,
                                    const char* bank_dir,
                                    const char* samples_dir);

char*   file_ops_read_link(const char* path);

#endif

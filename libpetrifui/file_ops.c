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


#include "file_ops.h"

#include <assert.h>
#include <limits.h>
#include <gcrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "msg_log.h"
#include "petri-foo.h"


char* file_ops_join_str(const char* str1, char join, const char* str2)
{
    const char* s1lc = 0; /* dir last character */
    size_t newlen = 0;
    char* newstr = 0;

    if (!str1)
        return (str2 != 0) ? strdup(str2) : 0;

    if (!str2)
        return strdup(str1);

    s1lc = strrchr(str1, '\0');
    --s1lc;

    if (s1lc < str1)
        return strdup(str2);

    newlen = strlen(str1) + strlen(str2);

    if (*s1lc == join)
    {
        if (*str2 == join)
        {
            ++str2;
            --newlen;
        }
    }
    else if (*str2 != join)
    {
        ++newlen;
    }

    newstr = malloc(newlen + 1);
    strcpy(newstr, str1);

    if (*s1lc != join && *str2 != join)
    {
        char* n = newstr + strlen(newstr);
        *n++ = join;
        *n = '\0';
    }

    strcat(newstr, str2);
    return newstr;
}


int file_ops_split_str(const char* str, char split, char** retstr1,
                                                    char** retstr2,
                                                    int bias)
{
    char* str2 = 0;
    ptrdiff_t d;

    assert(str != 0);
    assert(strlen(str) > 0);
    assert(retstr1 != 0 || retstr2 != 0);

    if (retstr1)
        *retstr1 = 0;

    if (retstr2)
        *retstr2 = 0;

    str2 = strrchr(str, split);

    if (!str2)
        return -1;

    if (bias < 0 || bias == 0)
    {
        ++str2;

        if (retstr2)
            *retstr2 = (*str2 != '\0') ? strdup(str2) : 0;
    }
    else if (bias > 0)
    {
        if (retstr2)
            *retstr2 = strdup(str2);
    }

    if (!retstr1)
        return 0;


    d = str2 - str;

    int dtest = 0;
    int index = 0;

    if (bias < 0)
    {
        dtest = 1;
        index = 1;
    }

    if (d > dtest)
    {
        *retstr1 = malloc(d + 1);
        strncpy(*retstr1, str, d);
        (*retstr1)[d] = '\0';
    }
    else
    {
        *retstr1 = malloc(index + 1);
        if (index)
            (*retstr1)[0] = split;
        (*retstr1)[index] = '\0';
    }

    return 0;
}


char* file_ops_parent_dir(const char* _path)
{
    char* parent = 0;
    char* path = strdup(_path);
    size_t lc = strlen(path) - 1;

    if (*(path + lc) == '/')
        *(path + lc) = '\0';

    if (file_ops_split_path(path, &parent, 0) == -1)
    {
        debug("failed parent_dir split\n");
    }

    free(path);
    return parent;
}


char* file_ops_make_relative(const char* path, const char* parent)
{
    const char* rel;
    size_t parentlen;

    if (strstr(path, parent) != path)
        return 0;

    parentlen = strlen(parent);

    rel = path + parentlen;

    if (*rel == '/')
        ++rel;

    return strdup(rel);
}


char* file_ops_dir_to_hash(const char* path)
{
    size_t len;
    unsigned char* h;
    size_t h_len;
    char* buf;
    char* b;
    int n;

    assert(path != NULL);

    if (path[0] != '/')
        return 0;

    len = strlen(path);

    if (path[len - 1] == '/')
        --len;

    h_len = gcry_md_get_algo_dlen(GCRY_MD_SHA1);
    h = malloc(sizeof(char) * h_len);
    gcry_md_hash_buffer(GCRY_MD_SHA1, h, path, len);

    buf = malloc(sizeof(*b) * h_len * 2 + 1);

    for (n = 0, b = buf; n < h_len; ++n, b += 2, ++h)
        snprintf(b, 3, "%02x", (unsigned)*h);

    free(h);

    return buf;
}


char* file_ops_mkdir(const char* dir, const char* parent)
{
    char* newdir = file_ops_join_path(parent, dir);

    if (newdir)
    {
        struct stat st;

        if (stat(newdir, &st) != 0)
            mkdir(newdir, 0777);

        if (stat(newdir, &st) == 0)
            return newdir;
    }

    msg_log(MSG_ERROR, "failed to create dir '%s' within '%s'\n",
                                                    dir, parent);
    return 0;
}


char* file_ops_hash_mkdir(const char* path, const char* parent)
{
    char* hash = 0;
    char* hash_dir = 0;
    struct stat st;

    if (!path || !parent)
        return 0;

    if ((hash = file_ops_dir_to_hash(path)))
    {
        debug("hash:'%s' path:'%s'\n", hash, path);

        if ((hash_dir = file_ops_join_path(parent, hash)))
        {
            debug("hash dir path:'%s'\n", hash_dir);

            if (stat(hash_dir, &st) != 0)
            {
                debug("must create hash dir\n");

                if (mkdir(hash_dir, 0777) == 0)
                {
                    char* dfilename;
                    FILE* dfile;

                    if ((dfilename = file_ops_join_path(hash_dir,
                                                        "pathinfo.txt")))
                    {
                        if ((dfile = fopen(dfilename, "w")))
                        {
                            debug("writing '%s' as path to pathinfo.txt\n",
                                                                    path);
                            fprintf(dfile, "%s\n", path);
                            fclose(dfile);
                        }
                        free(dfilename);
                    }
                }
                else{
                    debug("failed to create hash dir\n");}
            }

            if (stat(hash_dir, &st) == 0)
            {
                free(hash);
                return hash_dir;
            }

            free(hash_dir);

        }
        free(hash);
    }

    msg_log(MSG_ERROR, "failed to create hash dir in '%s' for '%s'\n",
                                                        parent, path);
    return 0;
}


char* file_ops_sample_path_mangle(  const char* samplepath,
                                    const char* bank_dir,
                                    const char* samples_dir)
{
    char* sdir;     /* sample dir */
    char* sfile;    /* sample filename */
    char* hash_dir; /* full path of hash dir */

    if (strstr(samplepath, bank_dir) == samplepath)
    {   /*  sample is located inline within bank_dir */
        return file_ops_make_relative(samplepath, bank_dir);
    }

    file_ops_split_path(samplepath, &sdir, &sfile);

    if (!sdir || !sfile)
    {
        free(sdir);
        free(sfile);
        msg_log(MSG_ERROR, "non archivable sample path '%s'\n", samplepath);
        return 0;
    }

    if ((hash_dir = file_ops_hash_mkdir(sdir, samples_dir)))
    {
        char* link = file_ops_join_path(hash_dir, sfile);

        if (link)
        {
            struct stat st;

            if (stat(link, &st) != 0)
                symlink(samplepath, link);

            if (stat(link, &st) == 0)
            {
                char* link_rel = file_ops_make_relative(link, bank_dir);

                if (link_rel)
                {
                    free(sdir);
                    free(sfile);
                    free(hash_dir);
                    free(link);
                    return link_rel;
                }
            }

            free(link);
        }

        free(hash_dir);
    }

    msg_log(MSG_ERROR, "symlinking sample '%s' failed\n", samplepath);

    free(sdir);
    free(sfile);

    return 0;
}


char* file_ops_read_link(const char* path)
{
    struct stat st;
    size_t maxlen = 0;

    char* buf = 0;
    ssize_t len = 0;

    char* next = 0;

    if (lstat(path, &st) != 0 || !S_ISLNK(st.st_mode))
        return 0;

    while((size_t)len == maxlen)
    {
        free(buf);
        maxlen += 256;

        if (maxlen > SSIZE_MAX)
            return 0;

        if (!(buf = malloc(maxlen)))
            return 0;

        if ((len = readlink(path, buf, maxlen)) == -1)
        {
            free(buf);
            return 0;
        }
    }

    buf[len] = '\0';

    if (!(next = file_ops_read_link(buf)))
        return buf;

    free(buf);
    return next;
}

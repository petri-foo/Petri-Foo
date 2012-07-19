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
#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "msg_log.h"
#include "petri-foo.h"



int file_ops_path_split(const char* path, char** retdir, char** retfile)
{
    char* file;
    ptrdiff_t d;

    assert(path != 0);
    assert(strlen(path) > 0);

    if (retdir)
        *retdir = 0;

    if (retfile)
        *retfile = 0;

    file = strrchr(path, '/');

    if (!file)
    {
        if (retfile)
            *retfile = strdup(path);

        return 0;
    }

    ++file;

    if (retfile)
        *retfile = (*file != '\0') ? strdup(file) : 0;

    if (!retdir)
        return 0;

    d = file - path;

    assert(d > 0);

    if (d > 1)
    {
        *retdir = malloc(d + 1);
        strncpy(*retdir, path, d);
        (*retdir)[d] = '\0';
    }
    else
    {
        *retdir = malloc(2);
        (*retdir)[0] = '/';
        (*retdir)[1] = '\0';
    }

    return 0;
}


char* file_ops_make_path(const char* dir, const char* file)
{
    char* path = 0;
    const char* dlc; /* dir last character */
    size_t len;

    if (!dir)
        return (file != 0) ? strdup(file) : 0;

    if (!file)
        return strdup(dir);

    dlc = strrchr(dir, '\0');
    --dlc;

    if (dlc < dir)
        return strdup(file);

    len = strlen(dir) + strlen(file);

    if (*dlc == '/')
    {
        if (*file == '/')
        {
            ++file;
            --len;
        }
    }
    else if (*file != '/')
    {
        ++len;
    }

    path = malloc(len + 1);
    strcpy(path, dir);

    if (*dlc != '/' && *file != '/')
    {
        char* n = path + strlen(path);
        *n++ = '/';
        *n = '\0';
    }

    strcat(path, file);
    return path;
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
    char* buf;
    char* b;
    int n;

    assert(path != NULL);

    if (path[0] != '/')
        return 0;

    len = strlen(path);

    if (path[len - 1] == '/')
        --len;

    h = SHA1((unsigned char*)path, len, NULL);
    buf = malloc(sizeof(*b) * SHA_DIGEST_LENGTH * 2 + 1);

    for (n = 0, b = buf; n < SHA_DIGEST_LENGTH; ++n, b += 2, ++h)
        snprintf(b, 3, "%02x", (unsigned)*h);

    return buf;
}


char* file_ops_mkdir(const char* dir, const char* parent)
{
    char* newdir = file_ops_make_path(parent, dir);

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

        if ((hash_dir = file_ops_make_path(parent, hash)))
        {
            debug("hash dir path:'%s'\n", hash_dir);

            if (stat(hash_dir, &st) != 0)
            {
                debug("must create hash dir\n");

                if (mkdir(hash_dir, 0777) == 0)
                {
                    char* dfilename;
                    FILE* dfile;

                    if ((dfilename = file_ops_make_path(hash_dir,
                                                        "pathinfo.txt")))
                    {
                        if ((dfile = fopen(dfilename, "w")))
                        {
                            fprintf(dfile, "%s\n", parent);
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
    /* this should not be called for samples already within bank_dir */
    assert(strstr(samplepath, bank_dir) != samplepath);

    char* sdir;     /* sample dir */
    char* sfile;    /* sample filename */
    char* hash_dir; /* full path of hash dir */

    file_ops_path_split(samplepath, &sdir, &sfile);

    if (!sdir || !sfile)
    {
        free(sdir);
        free(sfile);
        msg_log(MSG_ERROR, "non archivable sample path '%s'\n", samplepath);
        return 0;
    }

    if ((hash_dir = file_ops_hash_mkdir(sdir, samples_dir)))
    {
        char* link = file_ops_make_path(hash_dir, sfile);

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
    return buf;
}

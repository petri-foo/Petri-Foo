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


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>


#include "petri-foo.h"
#include "msg_log.h"


static bool         msg_log_notification_state = false;
static msg_log_cb   msg_log_callback = 0;


void timestamp(char* buf, int buflen)
{
    time_t ltime;
    struct tm *Tm;
    struct timeval detail_time;
    int n;

    if (!buf)
        return;

    if (buflen < 13)
    {
        *buf = '\0';
        return;
    }

    gettimeofday(&detail_time,NULL);
    ltime = time(NULL);
    Tm=localtime(&ltime);

    n = snprintf(buf, buflen, "%02d:%02d:%02d.%03d",
                    Tm->tm_hour, Tm->tm_min, Tm->tm_sec,
                    (int)(detail_time.tv_usec / 1000.0f));

    if (n >= buflen)
        snprintf(buf, buflen, "--:--:--.---");
}


char* strconcat(const char* str1, const char* str2)
{
    char* str = malloc(strlen(str1) + strlen(str2) + 1);

    if (!str)
        return 0;

    strcpy(str, str1);
    strcat(str, str2);

    return str;
}


int msg_log(int type, const char* fmt, ...)
{
    const char* types[] = {
        "Debug",
        "Message",
        "Warning",
        "ERROR",
        "CRITICAL!"
    };

    char    msg[1024];
    char    tm[20];
    char    tmp[1024];

    va_list ap;

    int rc = 0;
    int base_type = type & MSG_TYPE_MASK;
    int out_type = type & MSG_FLAG_OUTPUT_MASK;

    if (!out_type)
        goto skip;

    if (base_type <= MSG_INVALID || base_type >= MSG_TYPE_XXX)
    {
        char* newfmt;
        newfmt = strconcat("Invalid log message: ", fmt);

        va_start(ap, fmt);
        msg_log(MSG_CRITICAL, newfmt, ap);
        va_end(ap);

        free(newfmt);

        return -1;
    }

    timestamp(tm, 20);

    va_start(ap, fmt);

    if (vsnprintf(tmp, 1023, fmt, ap) >= 1023)
        tmp[1023] = '\0';

    va_end(ap);

    rc = snprintf(msg, 1023, "%s %s: %s ", tm, types[base_type], tmp);

    if (rc >= 1023)
    {
        rc = 1023;
        msg[rc] = '\0';
    }

    if (out_type & MSG_FLAG_STDOUT)
        fprintf(stdout, "%s", msg);

    if (out_type & MSG_FLAG_STDERR)
        fprintf(stderr, "%s", msg);

    if ((out_type & MSG_FLAG_STDUI) && msg_log_callback)
        msg_log_callback(msg, base_type);

skip:

    if (type & MSG_FLAG_NOTIFY)
        msg_log_notification_state = true;

    return rc;
}


/*  were there any errors? */
bool msg_log_get_notification_state(void)
{
    return msg_log_notification_state;
}


/*  reset error status */
void msg_log_reset_notification_state(void)
{
    msg_log_notification_state = false;
}


void msg_log_set_message_cb(msg_log_cb msgcb)
{
    msg_log_callback = msgcb;
}


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


#ifndef MSG_LOG_H
#define MSG_LOG_H

/*  MSG_LOG

    Logging of messages useful to Petri-Foo user. Not to be confused with
    messages useful to developers. Messages such as errors when user asks
    Petri-Foo to load a bank or sample, etc.

    *** NOT for usage by RT thread ***
 */

#ifdef errmsg
#undef errmsg
#define errmsg do_not_use_errmsg_here
#endif

#include <stdbool.h>


/*  type of the callback that will handle the string formed by
    logging a message:
 */
typedef void (*msg_log_cb)(const char* msg, int msg_base_type);


enum
{
/* invalid message type */
    MSG___INVALID =     -1,

/* base message types: */
    MSG___TYPE_DEBUG =  0,
    MSG___TYPE_MESSAGE,
    MSG___TYPE_WARNING,
    MSG___TYPE_ERROR,
    MSG___TYPE_CRITICAL,

/* used for checking valid base message types */
    MSG___TYPE_XXX,

/* message type mask */
    MSG___TYPE_MASK =       0x000f,

/* message flags: */
    MSG___FLAG_NOTIFY =     0x0010, /* sets notification state */

    MSG___FLAG_STDOUT =     0x0100, /* output to stdout */
    MSG___FLAG_STDERR =     0x0200, /* output to stderr */
    MSG___FLAG_STDUI =      0x0400, /* output to ie gui */

    MSG___FLAG_OUTPUT_MASK = 0x0f00,

/* the actual message types generally used */
    MSG_DEBUG =     MSG___TYPE_DEBUG |      MSG___FLAG_STDOUT,

    MSG_MESSAGE =   MSG___TYPE_MESSAGE |    MSG___FLAG_STDOUT
                  | MSG___FLAG_STDUI,

    MSG_WARNING =   MSG___TYPE_WARNING |    MSG___FLAG_STDOUT
                  | MSG___FLAG_STDUI,

    MSG_ERROR =     MSG___TYPE_ERROR    | MSG___FLAG_NOTIFY
                  | MSG___FLAG_STDERR   | MSG___FLAG_STDUI,

    MSG_CRITICAL =  MSG___TYPE_CRITICAL | MSG___FLAG_NOTIFY
                  | MSG___FLAG_STDERR   | MSG___FLAG_STDUI,
};


void    timestamp(char* buf, int buflen);
char*   strconcat(const char*, const char*);


int     msg_log(int type, const char* format, ...);
void    msg_log_set_message_cb(msg_log_cb);

bool    msg_log_get_notification_state(void);
void    msg_log_reset_notification_state(void);

#endif

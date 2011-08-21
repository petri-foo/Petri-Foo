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


#include <stdbool.h>



/*  type of the callback that will handle the string formed by
    logging a message:
 */
typedef void (*msg_log_cb)(const char* msg, int msg_base_type);


enum
{
/* invalid message type */
    MSG_INVALID =       -1,

/* base message types: */
    MSG_TYPE_DEBUG =    0,
    MSG_TYPE_MESSAGE,
    MSG_TYPE_WARNING,
    MSG_TYPE_ERROR,
    MSG_TYPE_CRITICAL,

/* used for checking valid base message types */
    MSG_TYPE_XXX,

/* message type mask */
    MSG_TYPE_MASK =         0x00ff,

/* message flags: */
    MSG_FLAG_NOTIFY =       0x0100,

/* the actual message types generally used */
    MSG_DEBUG =     MSG_TYPE_DEBUG,
    MSG_MESSAGE =   MSG_TYPE_MESSAGE,
    MSG_WARNING =   MSG_TYPE_WARNING,
    MSG_ERROR =     MSG_TYPE_ERROR | MSG_FLAG_NOTIFY,
    MSG_CRITICAL =  MSG_TYPE_CRITICAL | MSG_FLAG_NOTIFY
};


void    timestamp(char* buf, int buflen);
char*   strconcat(const char*, const char*);


int     msg_log(int type, const char* format, ...);
void    msg_log_set_message_cb(msg_log_cb);

bool    msg_log_get_notification_state(void);
void    msg_log_reset_notification_state(void);

#endif

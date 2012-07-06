/*  Petri-Foo is a fork of the Specimen audio sampler.

    Original Specimen author Pete Bessman
    Copyright 2005 Pete Bessman
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


#ifndef __SESSION_H__
#define __SESSION_H__


enum {
    SESSION_TYPE_NONE,
    SESSION_TYPE_JACK,
    SESSION_TYPE_NSM,
    SESSION_STATE_OPEN,
    SESSION_STATE_CLOSED
};


/* session_init returns 0 on sucess */
int     session_init(int argc, char* argv[]);

int     session_cleanup(void);

int     session_get_type(void);
char*   session_get_path(void);


#endif /* __SESSION_H__ */

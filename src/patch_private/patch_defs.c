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

    This file is a derivative of a Specimen original, modified 2011
*/


/*
#include "adsr.h"
#include "sample.h"
*/

#include "patch_defs.h"

#include "driver.h"
#include "patch.h"

/*#include "patch_data.h"
#include "patch_defs.h"
*/

const float PATCH_MIN_RELEASE = 0.005;
const float PATCH_LEGATO_LAG = 0.005;
const float ALMOST_ZERO = 1e-6;


int         patch_samplerate = -1;
int         patch_buffersize = -1;

int         patch_legato_lag = 20;     /* bogus initial value */


Patch*      patches[PATCH_COUNT];


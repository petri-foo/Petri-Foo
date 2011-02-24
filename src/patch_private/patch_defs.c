#include "adsr.h"
#include "driver.h"
#include "patch.h"
#include "sample.h"

#include <glib.h>

#include "patch_data.h"
#include "patch_defs.h"


const float  PATCH_MIN_RELEASE = 0.05;
const float  PATCH_LEGATO_LAG = 0.05;
const float  ALMOST_ZERO = 1e-6;


float        patch_samplerate = DRIVER_DEFAULT_SAMPLERATE;
int          patch_legato_lag = 20;     /* bogus initial value */

float        patch_declick_dec = 0.1;   /* bogus initial value 
                                         * FIXME: patch_declic_dec
                                         * will become deprecated */

Patch        patches[PATCH_COUNT];


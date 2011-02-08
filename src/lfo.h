#ifndef __LFO_H__
#define __LFO_H__

/*
 * This class implements a LUT-based LFO with various waveforms and
 * linear interpolation.
 *
 * It uses 32-bit fixed-point phase and increment, where the 8 MSB
 * represent the integer part of the number and the 24 LSB the
 * fractionnal part
 *
 * Originally written Remy Muller on 2003-08-22
 *
 * Adapted for C and Specimen by that incorrigible wannabe
 * cyber-ninja, Pete Bessman, on 2004-06-03.
 */

#include <glib.h>
#include "ticks.h"

typedef enum
{
     LFO_SHAPE_SINE,
     LFO_SHAPE_TRIANGLE,
     LFO_SHAPE_SAW,
     LFO_SHAPE_SQUARE
}
LFOShape;

typedef struct _LFOParams
{
    gboolean lfo_on;
     LFOShape shape;
     float freq;        /* frequency in hz */
     float sync_beats;
     gboolean sync;
     gboolean positive; /* oscillate from [0, 1] instead of [-1, 1] */
     float delay;       /* in seconds */
     float attack;      /* in seconds */

    int     mod1_id;    /* ID of modulation source */
    float   mod1_amt;   /* amount of modulation we add [-1.0, 1.0]  */

    int     mod2_id;
    float   mod2_amt;
}
LFOParams;

typedef struct _LFO
{
     gboolean positive; /* whether to constrain values to [0, 1] */
     float val;         /* current value */
     guint32 phase;     /* current phase within shape, 8 MSB
                         * representing integer part and 24
                         * LSB representing fractional part */
     guint32 inc;       /* amount to increase phase by per tick */
     float* tab;        /* points to tabelized waveform */
     Tick delay;
     Tick attack;
     Tick attack_ticks; /* how far along we are in the attack phase */

    float*  freq_mod1;
    float*  freq_mod2;
    float   mod1_amt;
    float   mod2_amt;
}
LFO;

/* initialize LFO subsystem */
void lfo_init ( );

/* set operating samplerate of all LFOs (call at least once and
 * whenever samplerate changes) */
void lfo_set_samplerate (int rate);

/* set operating tempo in beats per minute of all tempo-synced LFOs */
void lfo_set_tempo (float bpm);

/* prepare an LFO for use (must be first function called on an LFO) */
void lfo_prepare (LFO* lfo);

/* activate an LFO using the given params; an LFO must be re-activated
 * after the samplerate/tempo changes in order for those changes to
 * take effect */
void lfo_trigger (LFO* lfo, LFOParams* params);

/* advance an LFO and return its new value */
float lfo_tick (LFO* lfo);

#endif /* __LFO_H__ */

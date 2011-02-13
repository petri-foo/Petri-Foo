#include <math.h>
#include <glib.h>
#include "maths.h"
#include "petri-foo.h"
#include "lfo.h"
#include "driver.h"
#include "sync.h"
#include "ticks.h"

/* sample rate we expect the world to be running at */
static int samplerate = DRIVER_DEFAULT_SAMPLERATE;

/* tempo we expect the world to be running at */
static float tempo = SYNC_DEFAULT_TEMPO;

/* lookup tables for different shapes */
static float sintab[255];		/* sine */
static float sqtab[255];		/* square */
static float tritab[255];		/* triangle */
static float sawtab[255];		/* saw */

/* set the phase increment based on the LFO frequency */
inline static void lfo_set_inc_from_freq (LFO* lfo, float freq)
{
     lfo->inc = (guint32) ((255.0 * freq / samplerate) * (float) (1 << 24));
}

/* set the phase increment based on period length in beats */
inline static void lfo_set_inc_from_beats (LFO* lfo, float beats)
{
     lfo_set_inc_from_freq (lfo, (tempo / 60.0) / beats);
}

void lfo_init ( )
{
     int i;
     float* t;
     
     /* build triangle table */
     t = tritab;
     for (i = 0; i < 64; i++)
     {
	  t[i] = i / 64.0;
	  t[i + 64] = (64 - i) / 64.0;
	  t[i + 128] = -i / 64.0;
	  t[i + 192] = -(64 - i) / 64.0;
     }

     /* build sine table */
     t = sintab;
     for (i = 0; i <= 255; i++)
	  t[i] = sin (2.0 * G_PI * (i / 255.0));

     /* build saw table */
     t = sawtab;
     for (i = 0; i < 255; i++)
	  t[i] = 2.0 * (i / 255.0) - 1.0;

     /* build square table */
     t = sqtab;
     for (i = 0; i < 128; i++)
     {
	  t[i] = 1.0;
	  t[i + 128] = -1.0;
     }
}

void lfo_set_samplerate (int rate)
{
     samplerate = rate;
}

void lfo_set_tempo (float bpm)
{
     tempo = bpm;
}

void lfo_prepare (LFO* lfo)
{
     lfo->positive = FALSE;
     lfo->val = 0.0;
     lfo->phase = 0;
     lfo_set_inc_from_freq (lfo, 1.0);
     lfo->tab = sintab;
     lfo->delay = 0;
     lfo->attack = 0;
     lfo->attack_ticks = 0;
}

void lfo_trigger (LFO* lfo, LFOParams* params)
{
     lfo->positive = params->positive;
     
     switch (params->shape)
     {
     case LFO_SHAPE_TRIANGLE:
	  lfo->tab = tritab;
	  break;
     case LFO_SHAPE_SAW:
	  lfo->tab = sawtab;
	  break;
     case LFO_SHAPE_SQUARE:
	  lfo->tab = sqtab;
	  break;
     case LFO_SHAPE_SINE:		/* fallthrough is intentional */
     default:
	  lfo->tab = sintab;
	  break;
     }

     /* we recalculate our phase increment in case the tempo or
      * samplerate has changed */
     if (params->sync)
     {
	  lfo_set_inc_from_beats (lfo, params->sync_beats);
     }
     else
     {
	  lfo_set_inc_from_freq (lfo, params->freq);
     }

     lfo->delay = ticks_secs_to_ticks (params->delay);
     lfo->attack = ticks_secs_to_ticks (params->attack);
     lfo->attack_ticks = 0;
     lfo->phase = 0;
     lfo->val = 0;
     lfo->mod1_amt = params->mod1_amt;
     lfo->mod2_amt = params->mod2_amt;
}

float lfo_tick (LFO* lfo)
{
     guint8 index, index0, index1, index2;
     guint8 frac;

     if (lfo->delay)
     {
	  lfo->delay--;
	  lfo->val = 0;
	  return lfo->val;
     }

     lfo->phase
      += lfo->inc
        * (lfo->freq_mod1 ? (1 + *lfo->freq_mod1 * lfo->mod1_amt) : 1)
        * (lfo->freq_mod2 ? (1 + *lfo->freq_mod2 * lfo->mod2_amt) : 1);

     /* calculate new value */
     index = lfo->phase >> 24;
     frac = (lfo->phase & 0x00FF0000) >> 16;

     /* ensure the unsigned rollover happens if needed */
     index0 = index - 1;
     index1 = index + 1;
     index2 = index + 2;

     lfo->val = cerp (lfo->tab[index0],
		      lfo->tab[index],
		      lfo->tab[index1],
		      lfo->tab[index2],
		      frac);

     if (lfo->positive)
     {
	  lfo->val = (lfo->val + 1) / 2.0;
     }
     
     if (lfo->attack)
     {
	  lfo->val = lerp (0.0, lfo->val, (lfo->attack_ticks * 1.0) / lfo->attack);
	  lfo->attack_ticks++;
	  if (lfo->attack_ticks == lfo->attack)
	       lfo->attack = 0;
     }
     
     return lfo->val;
}

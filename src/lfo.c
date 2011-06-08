#include "lfo.h"

#include "maths.h"
#include "petri-foo.h"
#include "driver.h"
#include "patch.h"
#include "sync.h"
#include "ticks.h"


#include <math.h>
#include <stdlib.h>

struct _LFO
{
    bool        positive;   /* whether to constrain values to [0, 1] */
    float       val;        /* current value */
    uint32_t    phase;      /* current phase within shape, 8 MSB
                             * representing integer part and 24
                             * LSB representing fractional part */
    uint32_t    inc;        /* amount to increase phase by per tick */
    float*      tab;        /* points to tabelized waveform */
    Tick        delay;
    Tick        attack;
    Tick        attack_ticks; /* how far along we are in the attack phase */

    float const*    freq_mod1;
    float const*    freq_mod2;
    float           mod1_amt;
    float           mod2_amt;
/*
    float const*    am1;
    float const*    am2;
    float           am1_amt;
    float           am2_amt;
*/
};


/* sample rate we expect the world to be running at */
static int samplerate = -1;

static float sync_tempo = SYNC_DEFAULT_TEMPO;


static float sin_tab[255];
static float squ_tab[255];
static float tri_tab[255];
static float saw_tab[255];


inline static void lfo_phase_inc_from_freq (LFO* lfo, float freq)
{
     lfo->inc = (uint32_t)((255.0 * freq / samplerate) * (float)(1 << 24));
}


inline static void lfo_phase_inc_from_beats (LFO* lfo, float beats)
{
     lfo_phase_inc_from_freq(lfo, (sync_tempo / 60.0) / beats);
}


void lfo_params_init(LFOParams* lfopar, float freq, LFOShape shape)
{
    lfopar->lfo_on = true;
    lfopar->shape = shape;
    lfopar->freq = freq;
    lfopar->sync_beats = 0.0;
    lfopar->sync = false;
    lfopar->positive = false;
    lfopar->delay = 0.0;
    lfopar->attack = 0.0;
    lfopar->mod1_id = MOD_SRC_NONE;
    lfopar->mod1_amt = 0.0;
    lfopar->mod2_id = MOD_SRC_NONE;
    lfopar->mod2_amt = 0.0;
/*
    lfopar->am1_id = MOD_SRC_NONE;
    lfopar->am1_amt = 0.0;
    lfopar->am2_id = MOD_SRC_NONE;
    lfopar->am2_amt = 0.0;
*/
}


LFO* lfo_new(void)
{
    LFO* lfo = malloc(sizeof(*lfo));

    if (!lfo)
        return 0;

    lfo_init(lfo);

    return lfo;
}


void lfo_free(LFO* lfo)
{
    free(lfo);
}


void lfo_init(LFO* lfo)
{
    lfo->positive = false;
    lfo->val = 0.0;
    lfo->phase = 0;
    lfo_phase_inc_from_freq(lfo, 1.0);
    lfo->tab = sin_tab;
    lfo->delay = 0;
    lfo->attack = 0;
    lfo->attack_ticks = 0;
    lfo->freq_mod1 = NULL;
    lfo->freq_mod2 = NULL;
    lfo->mod1_amt = 0.0;
    lfo->mod2_amt = 0.0;
/*
    lfo->am1 = NULL;
    lfo->am2 = NULL;
    lfo->am1_amt = 0.0;
    lfo->am2_amt = 0.0;
*/
}


void lfo_tables_init(void)
{
    int i;
    float* t;

    t = tri_tab;

    for (i = 0; i < 64; i++)
    {
        t[i] = i / 64.0;
        t[i + 64] = (64 - i) / 64.0;
        t[i + 128] = -i / 64.0;
        t[i + 192] = -(64 - i) / 64.0;
    }

    t = sin_tab;

    for (i = 0; i <= 255; i++)
        t[i] = sin (2.0 * M_PI * (i / 255.0));

    t = saw_tab;

    for (i = 0; i < 255; i++)
        t[i] = 2.0 * (i / 255.0) - 1.0;

    t = squ_tab;

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
     sync_tempo = bpm;
}


void lfo_rigger (LFO* lfo, LFOParams* params)
{
    lfo->positive = params->positive;

    switch (params->shape)
    {
    case LFO_SHAPE_TRIANGLE:
        lfo->tab = tri_tab;
        break;
    case LFO_SHAPE_SAW:
        lfo->tab = saw_tab;
        break;
    case LFO_SHAPE_SQUARE:
        lfo->tab = squ_tab;
        break;
    case LFO_SHAPE_SINE:
    default:
        lfo->tab = sin_tab;
        break;
    }

    /* we recalculate our phase increment in case the tempo or
     * samplerate has changed */
    if (params->sync)
        lfo_phase_inc_from_beats(lfo, params->sync_beats);
    else
        lfo_phase_inc_from_freq(lfo, params->freq);

    lfo->delay = ticks_secs_to_ticks(params->delay);
    lfo->attack = ticks_secs_to_ticks(params->attack);
    lfo->attack_ticks = 0;

    lfo->mod1_amt = params->mod1_amt;
    lfo->mod2_amt = params->mod2_amt;
/*
    lfo->am1_amt = params->am1_amt;
    lfo->am2_amt = params->am2_amt;
*/
}


void lfo_trigger (LFO* lfo, LFOParams* params)
{
    lfo_rigger(lfo, params);
    lfo->phase = 0;
    lfo->val = 0;
}


float lfo_tick(LFO* lfo)
{
    uint8_t index, index0, index1, index2;
    uint8_t frac;

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

    lfo->val = cerp(lfo->tab[index0],
                    lfo->tab[index],
                    lfo->tab[index1],
                    lfo->tab[index2], frac);

    if (lfo->positive)
        lfo->val = (lfo->val + 1) / 2.0;
     
    if (lfo->attack)
    {
        lfo->val = lerp(0.0,
                        lfo->val,
                        (lfo->attack_ticks * 1.0) / lfo->attack);

        lfo->attack_ticks++;

        if (lfo->attack_ticks == lfo->attack)
            lfo->attack = 0;
    }

    return lfo->val;
}


float const* lfo_output(LFO* lfo)
{
    return &lfo->val;
}


void lfo_set_freq_mod1(LFO* lfo, float const* src)
{
    lfo->freq_mod1 = src;
}


void lfo_set_freq_mod2(LFO* lfo, float const* src)
{
    lfo->freq_mod2 = src;
}


void lfo_set_output(LFO* lfo, float val)
{
    lfo->val = val;
}

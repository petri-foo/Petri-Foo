#include "petri-foo.h"
#include "adsr.h"
#include "maths.h"
#include "ticks.h"


#include <stdlib.h>


typedef struct _ADSR
{
    ADSRState state;    /* what phase of the envelope we are in         */

    Tick    ticks;      /* how far along we are in the current state    */
    float   aval;       /* value we attacked from                       */
    float   val;        /* our current value [0.0, 1.0]                 */
    float   rval;       /* value we released at                         */

    /* pre key tracked/velocity sensed values */
    Tick    _delay;     /* delay length in ticks                        */
    Tick    _attack;    /* attack length in ticks                       */
    Tick    _hold;      /* hold length in ticks                         */
    Tick    _decay;     /* decay length in ticks                        */
    float   _sustain;   /* sustain level [0.0, 1.0]                     */
    Tick    _release;   /* release length in ticks                      */

    /* running values */
    Tick    delay;      /* delay length in ticks                        */
    Tick    attack;     /* attack length in ticks                       */
    Tick    hold;       /* hold length in ticks                         */
    Tick    decay;      /* decay length in ticks                        */
    float   sustain;    /* sustain level [0.0, 1.0]                     */
    Tick    release;    /* release length in ticks                      */

    float key_amt;
/*  float vel_amt; */

} ADSR;


void adsr_params_init(ADSRParams* params, float attack, float release)
{
    params->env_on =    false;
    params->delay =     0.0;
    params->attack =    attack;
    params->decay =     0.0;
    params->hold =      0.0;
    params->sustain =   1.0;
    params->release =   release;

    params->key_amt = 0.0;
/*  params->vel_amt = 0.0; */
}


ADSR* adsr_new(void)
{
    ADSR* env = malloc(sizeof(*env));

    if (!env)
        return 0;

    adsr_init(env);

    return env;
}


void adsr_free(ADSR* env)
{
    free(env);
}


void adsr_init(ADSR* env)
{
    env->state   = ADSR_STATE_IDLE;
    env->ticks   = 0;
    env->val     = 0.0;
    env->rval    = 0.0;

    env->_delay  = 0;
    env->_attack = 0;
    env->_hold   = 0;
    env->_decay  = 0;
    env->_sustain= 0.0;
    env->_release= 0;

    env->delay   = 0;
    env->attack  = 0;
    env->hold    = 0;
    env->decay   = 0;
    env->sustain = 0.0;
    env->release = 0;

    env->key_amt = 0.0;
}


void adsr_trigger(ADSR* env, float key, float vel)
{
    env->state = ADSR_STATE_DELAY;
    env->ticks = 0;
    env->aval = env->val;

    if (env->key_amt < 0)
    {
        env->delay =    lerp(env->_delay,   env->_delay * (1.0 - key),
                                            env->key_amt * -1.0);

        env->attack =   lerp(env->_attack,  env->_attack * (1.0 - key),
                                            env->key_amt * -1.0);

        env->hold =     lerp(env->_hold,    env->_hold * (1.0 - key),
                                            env->key_amt * -1.0);

        env->decay =    lerp(env->_decay,   env->_decay * (1.0 - key),
                                            env->key_amt * -1.0);

        env->release =  lerp(env->_release, env->_release * (1.0 - key),
                                            env->key_amt * -1.0);
    }
    else
    {
        env->delay =    lerp(env->_delay,   env->_delay * key,
                                            env->key_amt);

        env->attack =   lerp(env->_attack,  env->_attack * key,
                                            env->key_amt);

        env->hold =     lerp(env->_hold,    env->_hold * key,
                                            env->key_amt);

        env->decay =    lerp(env->_decay,   env->_decay * key,
                                            env->key_amt);

        env->release =  lerp(env->_release, env->_release * key,
                                            env->key_amt);
    }

/*
    env->sustain = lerp(env->_sustain, env->_sustain * vel, env->vel_amt);
*/

    env->sustain = env->_sustain;
}


void adsr_release (ADSR* env)
{
     env->state = ADSR_STATE_RELEASE;
     env->ticks = 0;
}


/* return current value between 0 and 1 and advance */
float adsr_tick(ADSR* e)
{
    float d;

    switch (e->state)
    {
    case ADSR_STATE_DELAY:
        if (e->delay)
        e->val = 0.0;

        if (++e->ticks > e->delay)
        {
            e->state = ADSR_STATE_ATTACK;
            e->ticks = 0;
        }
        break;

    case ADSR_STATE_ATTACK:
        if (e->attack == 0)
        {
            e->val = 1.0;
        }
        else
        {
            d = (e->ticks * 1.0) / e->attack;
            e->val = lerp (e->aval, 1.0, d);
        }

        if (++e->ticks > e->attack)
        {
            e->state = ADSR_STATE_HOLD;
            e->ticks = 0;
        }
        break;

    case ADSR_STATE_HOLD:
        e->val = 1.0;

        if (++e->ticks > e->hold)
        {
            e->state = ADSR_STATE_DECAY;
            e->ticks = 0;
        }
        break;

    case ADSR_STATE_DECAY:
        if (e->decay == 0)
        {
            e->val = e->sustain;
        }
        else
        {
            d = (e->ticks * 1.0) / e->decay;
            e->val = lerp (1.0, e->sustain, d);
        }

        if (++e->ticks > e->decay)
        {
            e->state = ADSR_STATE_SUSTAIN;
            e->ticks = 0;
            e->val = e->sustain;
        }
        break;

    case ADSR_STATE_SUSTAIN:
        /* e->val just hovers at e->sustain */
        break;

    case ADSR_STATE_RELEASE:
        if (e->release == 0)
        {
            e->val = 0.0;
        }
        else if (e->ticks == 0)
        {
            /* we are just beginning to release, store our current
             * value for later use */
            e->rval = e->val;
        }
        else
        {
            d = (e->ticks * 1.0) / e->release;
            e->val = lerp (e->rval, 0.0, d);
        }

        if (++e->ticks > e->release)
        {
            e->state = ADSR_STATE_IDLE;
            e->ticks = 0;
        }
        break;

    case ADSR_STATE_IDLE:   /* fall through */
    default:
        e->val = 0.0;
        break;
    }

    return e->val;
}


void adsr_set_params (ADSR* env, ADSRParams* params)
{
    env->_delay   = ticks_secs_to_ticks (params->delay);
    env->_attack  = ticks_secs_to_ticks (params->attack);
    env->_hold    = ticks_secs_to_ticks (params->hold);
    env->_decay   = ticks_secs_to_ticks (params->decay);
    env->_sustain = params->sustain;
    env->_release = ticks_secs_to_ticks (params->release);
    env->key_amt  = params->key_amt;
/*  env->vel_amt  = params->vel_amt; */
}


float const* adsr_output(ADSR* env)
{
    return &env->val;
}

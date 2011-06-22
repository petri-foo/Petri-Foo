#ifndef MIDI_CONTROL_H
#define MIDI_CONTROL_H

enum
{
    /* pitch wheel is a special case            */
    CC_PITCH_WHEEL =    -1,

    /* midi control change parameters */

    CC_BANK_SELECT =            0x00,
    CC_MOD_WHEEL,
    CC_BREATH,

    CC_FOOT =                   0x04,
    CC_PORTAMENTO_TIME,
    CC_DATA_ENTRY_MSB,
    CC_CHANNEL_VOLUME,
    CC_BALANCE,

    CC_PAN =                    0x0a,
    CC_EXPRESSION,
    CC_EFFECT_CONTROL1,
    CC_EFFECT_CONTROL2,

    CC_GEN_PURPOSE1 =           0x10,
    CC_GEN_PURPOSE2,
    CC_GEN_PURPOSE3,
    CC_GEN_PURPOSE4,

    CC_SUSTAIN =                0x40,
    CC_PORTAMENTO,
    CC_SOSTENUTO,
    CC_SOFT,
    CC_LEGATO,
    CC_HOLD2,

    CC_SNDCTRL1_VARIATION =     0x46,
    CC_SNDCTRL2_TIMBRE,
    CC_SNDCTRL3_RELEASE,
    CC_SNDCTRL4_ATTACK,
    CC_SNDCTRL5_BRIGHTNESS,
    CC_SNDCTRL6_DECAY_TIME,
    CC_SNDCTRL7_VIBRATO_RATE,
    CC_SNDCTRL8_VIBRATO_DEPTH,
    CC_SNDCTRL9_VIBRATO_DELAY,
    CC_SNDCTRL10_UNDEFINED,

    CC_GEN_PURPOSE5 =           0x50,
    CC_GEN_PURPOSE6,
    CC_GEN_PURPOSE7,
    CC_GEN_PURPOSE8,

    CC_PORTAMENTO_CONTROL =     0x54,

    CC_HIRES_VELO_PREFIX =      0x58,

    CC_FX1_DEPTH =              0x5b,
    CC_FX2_DEPTH,
    CC_FX3_DEPTH,
    CC_FX4_DEPTH,
    CC_FX5_DEPTH,

    CC_DATA_INC =               0x60,
    CC_DATA_DEC,
    CC_NRPN_LSB,
    CC_NRPN_MSB,
    CC_RPN_LSB,
    CC_RPN_MSB,

    /* ---- */
    /* used to gain count of controllers with regard to mod sources: */
    CC___MSG___LAST =             0x78,
    /* ---- */

    CC_CHMODE_ALL_SOUND_OFF =   0x78,
    CC_CHMODE_RESET_ALL_CTRL,
    CC_CHMODE_LOCAL_CTRL_OFF,
    CC_CHMODE_ALL_NOTES_OFF,
    CC_CHMODE_OMNI_OFF,
    CC_CHMODE_OMNI_ON,
    CC_CHMODE_MONO_ON,
    CC_CHMODE_POLY_ON

};


enum { CC_ARR_SIZE = CC___MSG___LAST + 2 };


#define MIDI_CONTROL_H__CC_MAP_DEF                              \
static inline float cc_map(int param, int value)                \
{                                                               \
    switch(param) {                                             \
    case CC_PAN: case CC_BALANCE: return (value - 64.0) / 64.0; \
    default: return value / 127.0;                              \
    }                                                           \
}



#endif

#include "AD9833.h"

/* ==== Bits del registro de control ==== */
#define AD_B28       13
#define AD_HLB       12
#define AD_FSELECT   11
#define AD_PSELECT   10
#define AD_RESET      8
#define AD_SLEEP1     7
#define AD_SLEEP12    6
#define AD_OPBITEN    5
#define AD_DIV2       3
#define AD_MODE       1

#define AD9833_FREQ0   (1 << 14)
#define AD9833_FREQ1   (1 << 15)

#define AD9833_PHASE0  ((1 << 15) | (1 << 14))
#define AD9833_PHASE1  ((1 << 15) | (1 << 14) | (1 << 13))

#define AD_2POW28 ((uint64_t)1 << 28)

/* ============================================================
   CS
============================================================ */
static inline void _cs_select(AD9833_Handle_t *h)
{
    h->io.cs_low(h->io.ctx);
}

static inline void _cs_unselect(AD9833_Handle_t *h)
{
    h->io.cs_high(h->io.ctx);
}

/* ============================================================
   SPI write 16-bit
============================================================ */
static void AD9833_WriteReg(AD9833_Handle_t *h, uint16_t reg)
{
    uint8_t buf[2] = {
        (uint8_t)(reg >> 8),
        (uint8_t)(reg & 0xFF)
    };

    _cs_select(h);
    h->io.spi_tx(h->io.ctx, buf, 2);
    _cs_unselect(h);
}

static void sendCtl(AD9833_Handle_t *h)
{
    AD9833_WriteReg(h, h->regCtl);
}

/* ============================================================
   Helpers
============================================================ */
static uint32_t calcFreqWord(AD9833_Handle_t *h, float f)
{
    double word = ((double)f * (double)AD_2POW28) /
                  (double)h->mclk_hz;
    return (uint32_t)(word + 0.5);
}

static uint16_t calcPhase(uint16_t tenths_deg)
{
    double val = (512.0 * (tenths_deg / 10.0) / 45.0) + 0.5;
    return (uint16_t)val;
}

/* ============================================================
   INIT
============================================================ */
void AD9833_Init(AD9833_Handle_t *h)
{
    h->regCtl = (1 << AD_B28);
    sendCtl(h);

    AD9833_Reset(h, true);

    AD9833_SetFrequency(h, AD_CHAN_0, 1000.0f);
    AD9833_SetPhase(h, AD_CHAN_0, 0);

    AD9833_Reset(h, false);

    AD9833_SetMode(h, AD_MODE_SINE);
    AD9833_SetActiveFrequency(h, AD_CHAN_0);
    AD9833_SetActivePhase(h, AD_CHAN_0);
}

/* ============================================================
   RESET
============================================================ */
void AD9833_Reset(AD9833_Handle_t *h, bool hold)
{
    h->regCtl |= (1 << AD_RESET);
    sendCtl(h);

    if (!hold) {
        h->regCtl &= ~(1 << AD_RESET);
        sendCtl(h);
    }
}

/* ============================================================
   MODE
============================================================ */
void AD9833_SetMode(AD9833_Handle_t *h, AD9833_Mode_t mode)
{
    h->mode = mode;

    switch (mode) {
    case AD_MODE_OFF:
        h->regCtl &= ~(1 << AD_OPBITEN);
        h->regCtl &= ~(1 << AD_MODE);
        h->regCtl |=  (1 << AD_SLEEP1);
        h->regCtl |=  (1 << AD_SLEEP12);
        break;

    case AD_MODE_SINE:
        h->regCtl &= ~(1 << AD_OPBITEN);
        h->regCtl &= ~(1 << AD_MODE);
        h->regCtl &= ~(1 << AD_SLEEP1);
        h->regCtl &= ~(1 << AD_SLEEP12);
        break;

    case AD_MODE_SQUARE1:
        h->regCtl |=  (1 << AD_OPBITEN);
        h->regCtl &= ~(1 << AD_MODE);
        h->regCtl |=  (1 << AD_DIV2);
        h->regCtl &= ~(1 << AD_SLEEP1);
        h->regCtl &= ~(1 << AD_SLEEP12);
        break;

    case AD_MODE_SQUARE2:
        h->regCtl |=  (1 << AD_OPBITEN);
        h->regCtl &= ~(1 << AD_MODE);
        h->regCtl &= ~(1 << AD_SLEEP1);
        h->regCtl &= ~(1 << AD_SLEEP12);
        break;

    case AD_MODE_TRIANGLE:
        h->regCtl &= ~(1 << AD_OPBITEN);
        h->regCtl |=  (1 << AD_MODE);
        h->regCtl &= ~(1 << AD_SLEEP1);
        h->regCtl &= ~(1 << AD_SLEEP12);
        break;
    }

    sendCtl(h);
}

/* ============================================================
   Selección de canal
============================================================ */
void AD9833_SetActiveFrequency(AD9833_Handle_t *h, AD9833_Channel_t chan)
{
    if (chan == AD_CHAN_0)
        h->regCtl &= ~(1 << AD_FSELECT);
    else
        h->regCtl |=  (1 << AD_FSELECT);

    sendCtl(h);
}

void AD9833_SetActivePhase(AD9833_Handle_t *h, AD9833_Channel_t chan)
{
    if (chan == AD_CHAN_0)
        h->regCtl &= ~(1 << AD_PSELECT);
    else
        h->regCtl |=  (1 << AD_PSELECT);

    sendCtl(h);
}

/* ============================================================
   FRECUENCIA
============================================================ */
void AD9833_SetFrequency(AD9833_Handle_t *h,
                         AD9833_Channel_t chan,
                         float freq_hz)
{
    h->freq[chan] = freq_hz;
    h->regFreq[chan] = calcFreqWord(h, freq_hz);

    h->regCtl |= (1 << AD_B28);
    sendCtl(h);

    uint16_t lsb = h->regFreq[chan] & 0x3FFF;
    uint16_t msb = (h->regFreq[chan] >> 14) & 0x3FFF;

    uint16_t reg_lsb = (chan == AD_CHAN_0 ? AD9833_FREQ0 : AD9833_FREQ1) | lsb;
    uint16_t reg_msb = (chan == AD_CHAN_0 ? AD9833_FREQ0 : AD9833_FREQ1) | msb;

    AD9833_WriteReg(h, reg_lsb);
    AD9833_WriteReg(h, reg_msb);
}

/* ============================================================
   FASE
============================================================ */
void AD9833_SetPhase(AD9833_Handle_t *h,
                     AD9833_Channel_t chan,
                     uint16_t tenths_deg)
{
    h->phase[chan] = tenths_deg;

    uint16_t p = calcPhase(tenths_deg);
    uint16_t reg =
        (chan == AD_CHAN_0 ? AD9833_PHASE0 : AD9833_PHASE1) |
        (p & 0x0FFF);

    AD9833_WriteReg(h, reg);
}

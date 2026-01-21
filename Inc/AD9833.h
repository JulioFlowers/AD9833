#ifndef AD9833_H
#define AD9833_H


#include <stdint.h>
#include <stdbool.h>

#ifndef AD9833_SPI_HANDLE_TYPE
#define AD9833_SPI_HANDLE_TYPE SPI_HandleTypeDef
#endif

typedef enum {
AD_MODE_OFF = 0,
AD_MODE_SINE,
AD_MODE_TRIANGLE,
AD_MODE_SQUARE1,
AD_MODE_SQUARE2
} AD9833_Mode_t;

typedef enum {
AD_CHAN_0 = 0,
AD_CHAN_1 = 1
} AD9833_Channel_t;

typedef struct {
AD9833_SPI_HANDLE_TYPE *hspi;
GPIO_TypeDef *cs_port;
uint16_t cs_pin;
uint32_t mclk_hz;


uint16_t regCtl;
uint32_t regFreq[2];
uint16_t phase[2];
float freq[2];
AD9833_Mode_t mode;


} AD9833_Handle_t;

void AD9833_Init(AD9833_Handle_t *h,
AD9833_SPI_HANDLE_TYPE *hspi,
GPIO_TypeDef *cs_port,
uint16_t cs_pin,
uint32_t mclk_hz);

void AD9833_Reset(AD9833_Handle_t *h, bool hold);

void AD9833_SetFrequency(AD9833_Handle_t *h, AD9833_Channel_t chan, float freq_hz);
void AD9833_SetPhase(AD9833_Handle_t *h, AD9833_Channel_t chan, uint16_t tenths_deg);

void AD9833_SetMode(AD9833_Handle_t *h, AD9833_Mode_t mode);
void AD9833_SetActiveFrequency(AD9833_Handle_t *h, AD9833_Channel_t chan);
void AD9833_SetActivePhase(AD9833_Handle_t *h, AD9833_Channel_t chan);

#endif

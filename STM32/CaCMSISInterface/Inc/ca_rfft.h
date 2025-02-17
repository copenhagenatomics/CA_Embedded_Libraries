#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "arm_math.h"

typedef struct  CA_rfft_ctx_ CA_rfft_ctx;

#ifdef __cplusplus
extern "C" {
#endif

CA_rfft_ctx* ca_rfft_init(uint16_t fftLen);
q15_t* ca_rfft(CA_rfft_ctx* ctx, int16_t *pData, int noOfChannels, int noOfSamples, int channel);
int ca_rfft_absmax(q15_t *table, size_t size, q15_t* x, q15_t* y);

#ifdef __cplusplus
}
#endif
/*!
 * @file    ca_hanning.h
 * @brief   Header file of ca_hanning.c
 * @date    11/03/2025
 * @author  Timothé D
 */

#ifndef INC_CA_HANNING_H_
#define INC_CA_HANNING_H_

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************************************
** PUBLIC FUNCTION DECLARATIONS
***************************************************************************************************/

void hanningInit(float *pDst, uint32_t length);
void hanning(float *hanningCoef, int16_t *pData, uint32_t noOfChannels, uint32_t noOfSamples,
             uint16_t channel);

#ifdef __cplusplus
}
#endif

#endif /* INC_ADS7953_H_ */

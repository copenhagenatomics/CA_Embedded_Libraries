/*!
 * @file    iir_filters.h
 * @brief   Header file of iir_filters.c
 * @date    03/12/2025
 * @author  Timothé Dodin
 */

#ifndef INC_IIR_FILTERS_H_
#define INC_IIR_FILTERS_H_

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

// Second order IIR filter handler
typedef struct _iir2 {
    // Output coefficients
    float a1;
    float a2;
    // Input coefficients
    float b0;
    float b1;
    float b2;
    float x[3];  // Input
    float y[3];  // Output
} iir2_t;

/***************************************************************************************************
** PUBLIC FUNCTION DECLARATIONS
***************************************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

void iir2_band_pass_init(iir2_t *filt, float ts, float fc, float bw);
void iir2_band_stop_init(iir2_t *filt, float ts, float fc, float bw);
void iir2_low_pass_init(iir2_t *filt, float ts, float fc, float bw);
float iir2_update(iir2_t *filt, float newValue);

#ifdef __cplusplus
}
#endif

#endif /* INC_IIR_FILTERS_H_ */

/*!
** @brief Driver file for the SM4291 Pressure/Temperature sensor from TE connectivity
**
** https://www.te.com/en/product-4291-HGE-S-500-000.html
**
** @author Luke W
** @date   12/12/2024
*/

#ifndef SM4291_H_
#define SM4291_H_

#include <stdbool.h>
#include <stdint.h>

#include "stm32f4xx_hal.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

/* STATUS register bits */
#define STATUS_IDLE             0x0001U
#define STATUS_DSP_S_UP         0x0008U
#define STATUS_DSP_T_UP         0x0010U
#define STATUS_BS_FAIL          0x0080U
#define STATUS_BC_FAIL          0x0100U
#define STATUS_DSP_SAT          0x0400U
#define STATUS_COM_CRC_ERROR    0x0800U
#define STATUS_DSP_S_MISSED     0x4000U
#define STATUS_DSP_T_MISSED     0x8000U

/* Note: STATUS_SYNC register bits have same names and positions, but meaning of DSP_x_UP is 
** subtly different. See datasheet */
#define STATUS_SYNC_DSP_S_UP    0x0008U
#define STATUS_SYNC_DSP_T_UP    0x0010U


typedef struct {
    I2C_HandleTypeDef*  i2c;

    /* Whether to use CRC protection on the communication */
    bool                crc;

    /* Different devices have different pressure ranges */
    double              press_scaler;
    double              press_offset;

    uint32_t            serial;
} sm4291_i2c_handle_t;

/***************************************************************************************************
** PUBLIC FUNCTIONS
***************************************************************************************************/

sm4291_i2c_handle_t* sm4291Init(I2C_HandleTypeDef* hi2c, bool crc, double press_min, double press_max);
void sm4291Close(sm4291_i2c_handle_t* i2c);
int  sm4291GetTemp(sm4291_i2c_handle_t* i2c, double* result);
int  sm4291GetPressure(sm4291_i2c_handle_t* i2c, double* result);
int  sm4291GetSerial(sm4291_i2c_handle_t* i2c, uint32_t* result);
int  sm4291GetStatus(sm4291_i2c_handle_t* i2c, uint16_t* status);
int  sm4291GetStatusSync(sm4291_i2c_handle_t* i2c, uint16_t* status);
int  sm4291Reset(sm4291_i2c_handle_t* i2c);
int  sm4291Sleep(sm4291_i2c_handle_t* i2c);

#endif /* SM4291_H_ */
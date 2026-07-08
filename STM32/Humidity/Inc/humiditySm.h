/*!
 *  @file humiditySm.h
 *  @author Luke W
 *  @date   02/07/2026
 * 
 *  @brief C-style class for SHT45 humidity sensor state machine.
 *
 *  Handles measurement, I2C bus recovery, IIR filtering, periodic heater
 *  activation at high humidity, and an optional burn-in / decontamination mode.
 *
 *  Allocate one ::humidity_sm_t per sensor and pass its address to every function.
 *  This allows multiple independent sensor instances to coexist.
 *
 *  Typical usage:
 *  @code
 *  static humidity_sm_t sensor = {0};
 *
 *  void init(void) {
 *      humidity_sm_init(&sensor, &hi2c1,
 *                       GPIOB, GPIO_PIN_6,   // SCL
 *                       GPIOB, GPIO_PIN_7,   // SDA
 *                       MY_ERROR_MSK, MY_NO_ERROR_MSK);
 *  }
 *
 *  void loop(void) {
 *      humidity_sm_run(&sensor);
 *  }
 *  @endcode
 */

#ifndef INC_HUMIDITY_SM_H_
#define INC_HUMIDITY_SM_H_

#if defined(STM32F401xC) || defined(STM32F401xx)
#include "stm32f4xx_hal.h"
#elif defined(STM32H753xx)
#include "stm32h7xx_hal.h"
#endif

#include <stdbool.h>
#include <stdint.h>

#include "sht45.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

#define HUMIDITY_SM_HEATING_INTERVAL        60000U /* 1 minute between heating cycles */
#define HUMIDITY_SM_HEATING_SETTLING_TIME   55000U /* Hold humidity output for 55 s after heating */
#define HUMIDITY_SM_RH_THRESHOLD_HIGH       85     /* Trigger heating above this % RH */
#define HUMIDITY_SM_MAX_TEMP_BEFORE_HEATING 80     /* Pause burn-in heating above this °C */
#define HUMIDITY_SM_BURN_IN_TIME            4800000U /* Auto-stop burn-in after 80 minutes */

/***************************************************************************************************
** TYPES – exposed so callers can declare instances statically
***************************************************************************************************/

typedef enum {
    HUMIDITY_SM_MEASURE,
    HUMIDITY_SM_START_HEATING,
    HUMIDITY_SM_WAIT_FOR_CONVERSION,
    HUMIDITY_SM_UPDATE,
} humidity_sm_state_t;

typedef enum {
    HUMIDITY_LEVEL_NORMAL,
    HUMIDITY_LEVEL_HIGH,
} humidity_level_t;

typedef struct {
    humidity_level_t hum_state;
    float tempBeforeHeating;
    uint32_t lastHeating;
    int isFirstHeatingCycle;
} humidity_heating_state_t;

typedef struct {
    /* SHT45 handle */
    sht4x_handle_t sensor;

    /* IIR-filtered measurement outputs */
    float mvgRH;
    float mvgAH;
    float mvgTemp;

    /* Filter/error bookkeeping */
    bool reset_avg_filter;
    int error_count;

    /* State machine */
    humidity_sm_state_t state;
    humidity_heating_state_t heatingState;

    /* Burn-in mode */
    bool inBurnin;
    uint32_t burninStartTime;

    /* I2C recovery GPIO (SCL to toggle, SDA to read) */
    GPIO_TypeDef* sclPort;
    uint16_t sclPin;
    GPIO_TypeDef* sdaPort;
    uint16_t sdaPin;

    /* Board-status masks (project-specific) */
    uint32_t errorMsk;
} humidity_sm_t;

/***************************************************************************************************
** PUBLIC FUNCTION DECLARATIONS
***************************************************************************************************/

void humidity_sm_init(humidity_sm_t* sm, I2C_HandleTypeDef* hi2c, GPIO_TypeDef* sclPort,
                      uint16_t sclPin, GPIO_TypeDef* sdaPort, uint16_t sdaPin, uint32_t errorMsk);
void humidity_sm_run(humidity_sm_t* sm);
void humidity_sm_start_burnin(humidity_sm_t* sm);
void humidity_sm_stop_burnin(humidity_sm_t* sm);
bool humidity_sm_in_burnin(const humidity_sm_t* sm);
float humidity_sm_get_rh(const humidity_sm_t* sm);
float humidity_sm_get_ah(const humidity_sm_t* sm);
float humidity_sm_get_temp(const humidity_sm_t* sm);
uint32_t humidity_sm_get_serial(const humidity_sm_t* sm);

#endif /* INC_HUMIDITY_SM_H_ */

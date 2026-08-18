/*!
 *  @file humiditySm.c
 *  @author Luke W
 *  @date   02/07/2026
 * 
 *  @brief C-style class for SHT45 humidity sensor state machine.
 *         See humiditySm.h for usage.
 */

#include "humiditySm.h"

#include "systemInfo.h"

#ifndef __NOP
#define __NOP() ((void)0)
#endif

/***************************************************************************************************
** PRIVATE CONSTANTS
***************************************************************************************************/

static const int MAX_ERROR_COUNT = 10;
static const int M_AVG_COUNT     = 10;


/***************************************************************************************************
** PRIVATE FUNCTION DECLARATIONS
***************************************************************************************************/

static void                  setMeasurementError(humidity_sm_t* sm);
static void                  clearI2CBus(humidity_sm_t* sm);
static void                  resetI2C(humidity_sm_t* sm);
static humidity_sm_state_t   startConversion(humidity_sm_t* sm);
static humidity_sm_state_t   getConversion(humidity_sm_t* sm);
static humidity_sm_state_t   updateHumidity(humidity_sm_t* sm);
static humidity_sm_state_t   monitorTempInBurnin(humidity_sm_t* sm);
static humidity_sm_state_t   startHeating(humidity_sm_t* sm, uint8_t heatingProgram);

/***************************************************************************************************
** PRIVATE FUNCTIONS
***************************************************************************************************/

/*!
 *  @brief Accumulate a measurement error and invalidate outputs after MAX_ERROR_COUNT failures.
 */
static void setMeasurementError(humidity_sm_t* sm) {
    if (++sm->error_count >= MAX_ERROR_COUNT) {
        sm->error_count      = MAX_ERROR_COUNT; /* Clamp to prevent overflow */
        sm->reset_avg_filter = true;
        sm->mvgTemp          = 10000.0f;
        sm->mvgRH            = -1.0f;
        sm->mvgAH            = -1.0f;
    }
}

/*!
 *  @brief Toggle SCL to free a device that is holding SDA low.
 *
 *  Called only after the I2C peripheral has been de-initialised and SDA has
 *  been confirmed stuck low.  SCL must be re-claimed as GPIO before calling.
 */
static void clearI2CBus(humidity_sm_t* sm) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin              = sm->sclPin;
    GPIO_InitStruct.Mode             = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull             = GPIO_PULLUP;
    GPIO_InitStruct.Speed            = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(sm->sclPort, &GPIO_InitStruct);

    for (int i = 0; i < 10; i++) {
        HAL_GPIO_WritePin(sm->sclPort, sm->sclPin, GPIO_PIN_RESET);
        for (int j = 0; j < 40; j++) {
            __NOP();
        }
        HAL_GPIO_WritePin(sm->sclPort, sm->sclPin, GPIO_PIN_SET);
        for (int j = 0; j < 40; j++) {
            __NOP();
        }
    }
    HAL_GPIO_DeInit(sm->sclPort, sm->sclPin);
}

/*!
 *  @brief De-init, optionally clear a stuck bus, then re-init I2C.
 */
static void resetI2C(humidity_sm_t* sm) {
    HAL_I2C_DeInit(sm->sensor.hi2c);

    if (HAL_GPIO_ReadPin(sm->sdaPort, sm->sdaPin) == GPIO_PIN_RESET) {
        clearI2CBus(sm);
    }

    HAL_I2C_Init(sm->sensor.hi2c);
}

/*!
 *  @brief Initiate a new measurement, or try to recover from an error first.
 */
static humidity_sm_state_t startConversion(humidity_sm_t* sm) {
    if (bsGetField(sm->errorMsk)) {
        resetI2C(sm);

        if (sht4x_get_serial(&sm->sensor) == HAL_OK) {
            bsClearField(sm->errorMsk);
        }
        else {
            bsSetError(sm->errorMsk);
            setMeasurementError(sm);
            return HUMIDITY_SM_MEASURE;
        }
    }

    if (sht4x_initiate_measurement(&sm->sensor, SHT4X_MEASURE_HIGHREP) != HAL_OK) {
        bsSetError(sm->errorMsk);
        setMeasurementError(sm);
        return HUMIDITY_SM_MEASURE;
    }
    return HUMIDITY_SM_WAIT_FOR_CONVERSION;
}

/*!
 *  @brief Read back a conversion result.
 *
 *  A NACK (HAL_I2C_ERROR_AF) means the sensor has not finished converting yet;
 *  any other error is treated as fatal for this cycle.
 */
static humidity_sm_state_t getConversion(humidity_sm_t* sm) {
    sht4x_get_measurement(&sm->sensor);
    if (sm->sensor.hi2c->ErrorCode) {
        /* The datasheet specifies that the maximum time for a high repetition conversion is max 
        ** 8.3 ms. If the sensor has not yet fully converted the sample it will respond with a NACK. 
        ** As soon as a new measurement is ready the NACK flag will go low. 
        ** On any other errors reset the I2C peripheral and soft reset SHT45. */
        if (sm->sensor.hi2c->ErrorCode == HAL_I2C_ERROR_AF) {
            return HUMIDITY_SM_WAIT_FOR_CONVERSION;
        }

        bsSetError(sm->errorMsk);
        return HUMIDITY_SM_MEASURE;
    }

    return HUMIDITY_SM_UPDATE;
}

/*!
 *  @brief Apply IIR filter in normal operation.
 *
 *  Humidity outputs are frozen while the sensor temperature is still elevated
 *  after a heating cycle to prevent reporting spuriously low humidity.
 *  Triggers a new heating cycle when RH exceeds HUMIDITY_SM_RH_THRESHOLD_HIGH.
 */
static humidity_sm_state_t updateHumidity(humidity_sm_t* sm) {
    sm->error_count = 0;

    if ((sm->sensor.data.temperature - sm->heatingState.tempBeforeHeating >= 0.1f) &&
        ((HAL_GetTick() - sm->heatingState.lastHeating) <= HUMIDITY_SM_HEATING_SETTLING_TIME) &&
        sm->heatingState.hum_state == HUMIDITY_LEVEL_HIGH) {
        /* Temperature has not yet settled after heating – update temp only */
        sm->mvgTemp += (sm->sensor.data.temperature - sm->mvgTemp) / M_AVG_COUNT;
    }
    else if (sm->reset_avg_filter) {
        /* After re-establishing connection set the outputs directly and use moving averaging
        ** filter from next iteration. */
        sm->reset_avg_filter = false;
        sm->mvgTemp          = sm->sensor.data.temperature;
        sm->mvgRH            = sm->sensor.data.relative_humidity;
        sm->mvgAH            = sm->sensor.data.absolute_humidity;
    }
    else {
        sm->mvgTemp += (sm->sensor.data.temperature - sm->mvgTemp) / M_AVG_COUNT;
        sm->mvgRH += (sm->sensor.data.relative_humidity - sm->mvgRH) / M_AVG_COUNT;
        sm->mvgAH += (sm->sensor.data.absolute_humidity - sm->mvgAH) / M_AVG_COUNT;
    }

    sm->heatingState.hum_state =
        (sm->mvgRH >= HUMIDITY_SM_RH_THRESHOLD_HIGH) ? HUMIDITY_LEVEL_HIGH : HUMIDITY_LEVEL_NORMAL;

    if (sm->heatingState.hum_state == HUMIDITY_LEVEL_HIGH &&
        ((HAL_GetTick() - sm->heatingState.lastHeating >= HUMIDITY_SM_HEATING_INTERVAL) ||
         sm->heatingState.isFirstHeatingCycle)) {
        sm->heatingState.isFirstHeatingCycle = 0;
        return HUMIDITY_SM_START_HEATING;
    }
    return HUMIDITY_SM_MEASURE;
}

/*!
 *  @brief Monitor temperature during burn-in and keep heating as long as the sensor
 *         is below HUMIDITY_SM_MAX_TEMP_BEFORE_HEATING.
 *
 *  Humidity outputs are still updated but are not considered valid in this mode.
 *  Automatically exits burn-in after HUMIDITY_SM_BURN_IN_TIME.
 */
static humidity_sm_state_t monitorTempInBurnin(humidity_sm_t* sm) {

    if (sm->reset_avg_filter) {
        /* After re-establishing connection set the outputs directly and use moving averaging
        ** filter from next iteration. */
        sm->reset_avg_filter = false;
        sm->mvgTemp          = sm->sensor.data.temperature;
        sm->mvgRH            = sm->sensor.data.relative_humidity;
        sm->mvgAH            = sm->sensor.data.absolute_humidity;
    }
    else {
        /* IIR low pass filtering. Normal operation. */
        sm->mvgTemp += (sm->sensor.data.temperature - sm->mvgTemp) / M_AVG_COUNT;
        sm->mvgRH += (sm->sensor.data.relative_humidity - sm->mvgRH) / M_AVG_COUNT;
        sm->mvgAH += (sm->sensor.data.absolute_humidity - sm->mvgAH) / M_AVG_COUNT;
    }

    if ((HAL_GetTick() - sm->burninStartTime) >= HUMIDITY_SM_BURN_IN_TIME) {
        sm->inBurnin = false;
    }

    if (sm->mvgTemp >= HUMIDITY_SM_MAX_TEMP_BEFORE_HEATING) {
        return HUMIDITY_SM_MEASURE; /* Too hot – skip heating this cycle */
    }
    return HUMIDITY_SM_START_HEATING;
}

/*!
 *  @brief Activate the heater and record the pre-heating temperature.
 */
static humidity_sm_state_t startHeating(humidity_sm_t* sm, uint8_t heatingProgram) {
    /* Record temperature of sensor prior to heating. The humidity should not be output until after
    ** the temperature has settled to previous level */
    sm->heatingState.tempBeforeHeating = sm->mvgTemp;

    if (sht4x_turn_on_heater(&sm->sensor, heatingProgram) != HAL_OK) {
        bsSetError(sm->errorMsk);
        return HUMIDITY_SM_MEASURE;
    }
    sm->heatingState.lastHeating = HAL_GetTick();

    return HUMIDITY_SM_WAIT_FOR_CONVERSION;
}

/***************************************************************************************************
** PUBLIC FUNCTIONS
***************************************************************************************************/

/*!
 *  @brief Initialise a humidity state-machine instance.
 *  @param sm          Instance to initialise.
 *  @param hi2c        I2C peripheral connected to the SHT45.
 *  @param sclPort     GPIO port of the SCL pin (used for bus recovery).
 *  @param sclPin      GPIO pin mask of the SCL pin.
 *  @param sdaPort     GPIO port of the SDA pin (read to detect stuck-low bus).
 *  @param sdaPin      GPIO pin mask of the SDA pin.
 *  @param errorMsk    Board-status bit(s) to set/clear for this sensor's error.
 */
void humidity_sm_init(humidity_sm_t* sm, I2C_HandleTypeDef* hi2c, GPIO_TypeDef* sclPort,
                      uint16_t sclPin, GPIO_TypeDef* sdaPort, uint16_t sdaPin, uint32_t errorMsk) {
    sm->sensor.hi2c           = hi2c;
    sm->sensor.device_address = SHT45_I2C_ADDR;
    sm->sensor.serial_number  = 0x00;

    sm->sclPort    = sclPort;
    sm->sclPin     = sclPin;
    sm->sdaPort    = sdaPort;
    sm->sdaPin     = sdaPin;
    sm->errorMsk   = errorMsk;

    sm->state            = HUMIDITY_SM_MEASURE;
    sm->mvgRH            = 0.0f;
    sm->mvgAH            = 0.0f;
    sm->mvgTemp          = 0.0f;
    sm->reset_avg_filter = true; /* Seed filter directly on first reading */
    sm->error_count      = 0;
    sm->inBurnin         = false;
    sm->burninStartTime  = 0;

    sm->heatingState.hum_state           = HUMIDITY_LEVEL_NORMAL;
    sm->heatingState.isFirstHeatingCycle = 1;
    sm->heatingState.tempBeforeHeating   = 0.0f;
    sm->heatingState.lastHeating         = 0;

    if (sht4x_get_serial(&sm->sensor) != HAL_OK) {
        bsSetError(errorMsk);
    }
}

/*!
 *  @brief Advance the state machine by one step.
 *  @param sm  Instance to update.
 *  @note  Call every main-loop iteration.
 */
void humidity_sm_run(humidity_sm_t* sm) {
    typedef humidity_sm_state_t (*updateFn_t)(humidity_sm_t*);
    updateFn_t updateFn = sm->inBurnin ? monitorTempInBurnin : updateHumidity;
    uint8_t heaterProg  = sm->inBurnin ? SHT4X_HEATER_110mW_1s : SHT4X_HEATER_200mW_100ms;

    switch (sm->state) {
        case HUMIDITY_SM_MEASURE: {
            sm->state = startConversion(sm);
            break;
        }
        case HUMIDITY_SM_START_HEATING: {
            sm->state = startHeating(sm, heaterProg);
            break;
        }
        case HUMIDITY_SM_WAIT_FOR_CONVERSION: {
            sm->state = getConversion(sm);
            break;
        }
        case HUMIDITY_SM_UPDATE: {
            sm->state = updateFn(sm);
            break;
        }
        default: {
            sm->state = HUMIDITY_SM_MEASURE;
            break;
        }
    }
}

/*!
 *  @brief Enter burn-in / decontamination mode.
 *  @param sm  Instance to update.
 *  @note  Heats the sensor continuously until the temperature exceeds
 *         HUMIDITY_SM_MAX_TEMP_BEFORE_HEATING, or until HUMIDITY_SM_BURN_IN_TIME elapses.
 */
void humidity_sm_start_burnin(humidity_sm_t* sm) {
    sm->inBurnin        = true;
    sm->burninStartTime = HAL_GetTick();
}

/*!
 *  @brief Exit burn-in mode immediately.
 *  @param sm  Instance to update.
 */
void humidity_sm_stop_burnin(humidity_sm_t* sm) {
    sm->inBurnin = false;
}

/*!
 *  @brief Query whether the sensor is currently in burn-in mode.
 *  @param sm  Instance to query.
 *  @return    true while burn-in mode is active.
 */
bool humidity_sm_in_burnin(const humidity_sm_t* sm) {
    return sm->inBurnin != false;
}

/*!
 *  @brief Return the IIR-filtered relative humidity (%).
 *  @param sm  Instance to query.
 */
float humidity_sm_get_rh(const humidity_sm_t* sm) {
    return sm->mvgRH;
}

/*!
 *  @brief Return the IIR-filtered absolute humidity (g/m³).
 *  @param sm  Instance to query.
 */
float humidity_sm_get_ah(const humidity_sm_t* sm) {
    return sm->mvgAH;
}

/*!
 *  @brief Return the IIR-filtered temperature (°C).
 *  @param sm  Instance to query.
 */
float humidity_sm_get_temp(const humidity_sm_t* sm) {
    return sm->mvgTemp;
}

/*!
 *  @brief Return the sensor serial number read during initialisation.
 *  @param sm  Instance to query.
 */
uint32_t humidity_sm_get_serial(const humidity_sm_t* sm) {
    return sm->sensor.serial_number;
}

/*!
 *  @file   humiditySm_tests.cpp
 *  @brief  Unit tests for the SHT45 humidity sensor state machine
 *  @date   08/07/2026
 *  @author Luke W
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

/* Fakes */
#include "fake_stm32xxxx_hal.h"

/* Real supporting units */
#include "crc.c"
#include "sht45.c"
#include "systeminfo.c"

/* UUT */
#include "humiditySm.c"

using namespace std;

/***************************************************************************************************
** TEST HELPERS
***************************************************************************************************/

static const uint32_t TEST_ERR_MSK = 0x00000001U;

/* Inverse of the SHT45 raw -> physical conversions performed by sht45.c, used to build mock
** sensor payloads from a desired temperature / relative humidity. */
static uint16_t tempToAdc(float tempC) {
    return (uint16_t)((tempC + 45.0f) / 175.0f * 65535.0f);
}

static uint16_t rhToAdc(float rh) {
    return (uint16_t)((rh + 6.0f) / 125.0f * 65535.0f);
}

/* Mirrors sht45.c's decode, so expectations are not skewed by uint16_t quantisation. */
static float adcToTemp(uint16_t adc) {
    return -45.0f + 175.0f * adc / 65535.0f;
}

static float adcToRh(uint16_t adc) {
    float rh = -6.0f + 125.0f * adc / 65535.0f;
    if (rh > 100) {
        rh = 100;
    }
    else if (rh < 0) {
        rh = 0;
    }
    return rh;
}

static void encodeSht45Word(uint8_t* buf, uint16_t value) {
    buf[0] = (uint8_t)(value >> 8);
    buf[1] = (uint8_t)(value & 0xFF);
    initCrc8(0xFF, 0x31);
    buf[2] = crc8Calculate(buf, 2);
}

/***************************************************************************************************
** TEST FIXTURES
***************************************************************************************************/

class mockSHT45 : public stm32I2cTestDevice {
   public:
    mockSHT45(I2C_TypeDef* instance, uint16_t address) {
        bus  = instance;
        addr = address << 1;
    }

    HAL_StatusTypeDef transmit(uint8_t* buf, uint8_t size) {
        lastCmd = buf[0];
        return txRet;
    }

    HAL_StatusTypeDef recv(uint8_t* buf, uint8_t size) {
        if (rxRet != HAL_OK) {
            return rxRet;
        }

        if (lastCmd == SHT4X_READ_SERIAL) {
            encodeSht45Word(buf, (uint16_t)(serial >> 16));
            encodeSht45Word(buf + 3, (uint16_t)(serial & 0xFFFFU));
        }
        else {
            encodeSht45Word(buf, tempAdc);
            encodeSht45Word(buf + 3, humAdc);
        }
        return HAL_OK;
    }

    uint8_t lastCmd          = 0;
    HAL_StatusTypeDef txRet  = HAL_OK;
    HAL_StatusTypeDef rxRet  = HAL_OK;
    uint32_t serial          = 0xCAFEF00DU;
    uint16_t tempAdc         = 0;
    uint16_t humAdc          = 0;
};

class HumiditySmTests : public ::testing::Test {
   protected:
    HumiditySmTests() {
        /* Install a mock humidity sensor */
        humiditySensor = new mockSHT45(hi2c.Instance, SHT45_I2C_ADDR);
        fakeHAL_I2C_addDevice(humiditySensor);

        /* Initialise the device */
        humidity_sm_init(&sm, &hi2c, GPIOA, GPIO_PIN_0, GPIOB, GPIO_PIN_1, TEST_ERR_MSK);
    }

    void runOneMeasurementCycle() {
        for(int i = 0; i < 3; i++) {
            humidity_sm_run(&sm);
        }
    }

    I2C_HandleTypeDef hi2c = {.Instance = I2C1};
    humidity_sm_t sm{};
    mockSHT45* humiditySensor;
};

/***************************************************************************************************
** TESTS
***************************************************************************************************/

TEST_F(HumiditySmTests, testInitSuccess) {
    humiditySensor->serial = 0x11223344U;
    /* Re-initialise to pickup the new serial number */
    humidity_sm_init(&sm, &hi2c, GPIOA, GPIO_PIN_0, GPIOB, GPIO_PIN_1, TEST_ERR_MSK);

    EXPECT_EQ(bsGetField(TEST_ERR_MSK), 0u);
    EXPECT_EQ(humidity_sm_get_serial(&sm), 0x11223344U);
    EXPECT_FALSE(humidity_sm_in_burnin(&sm));
}

TEST_F(HumiditySmTests, testInitCommunicationError) {
    humiditySensor->txRet = HAL_ERROR;
    humidity_sm_init(&sm, &hi2c, GPIOA, GPIO_PIN_0, GPIOB, GPIO_PIN_1, TEST_ERR_MSK);

    EXPECT_NE(bsGetField(TEST_ERR_MSK), 0u);
}

TEST_F(HumiditySmTests, testFirstMeasurementSetsOutputsDirectly) {
    humiditySensor->tempAdc = tempToAdc(22.5f);
    humiditySensor->humAdc  = rhToAdc(40.0f);

    humidity_sm_run(&sm);
    EXPECT_EQ(sm.state, HUMIDITY_SM_WAIT_FOR_CONVERSION);
    EXPECT_EQ(humiditySensor->lastCmd, SHT4X_MEASURE_HIGHREP);

    humidity_sm_run(&sm); /* Return instantly as I2C error is not set */
    EXPECT_EQ(sm.state, HUMIDITY_SM_UPDATE);

    humidity_sm_run(&sm);
    EXPECT_EQ(sm.state, HUMIDITY_SM_MEASURE);

    EXPECT_NEAR(humidity_sm_get_temp(&sm), adcToTemp(humiditySensor->tempAdc), 0.01f);
    EXPECT_NEAR(humidity_sm_get_rh(&sm), adcToRh(humiditySensor->humAdc), 0.01f);

    /* Absolute humidity calculator: https://www.omnicalculator.com/physics/absolute-humidity */
    EXPECT_NEAR(humidity_sm_get_ah(&sm), 7.994, 0.01f); 
}

TEST_F(HumiditySmTests, testFilteringAppliesIirAverage) {
    humiditySensor->tempAdc = tempToAdc(20.0f);
    humiditySensor->humAdc  = rhToAdc(40.0f);

    runOneMeasurementCycle();
    float rhBefore = humidity_sm_get_rh(&sm);

    /* Step change in humidity - filtered output should move only part-way there. */
    humiditySensor->humAdc = rhToAdc(60.0f);
    runOneMeasurementCycle();

    float rhAfter  = humidity_sm_get_rh(&sm);
    float rawStep  = adcToRh(humiditySensor->humAdc);
    EXPECT_LT(rhAfter, rawStep);
    EXPECT_GT(rhAfter, rhBefore);

    /* SHT45 Datasheet says high rep measurement takes ~8.3 ms. Filter should have settled close 
    ** after ~ 1 s (100 cycles) */
    for (int i = 0; i < 100; i++) {
        runOneMeasurementCycle();
    }
    EXPECT_NEAR(humidity_sm_get_rh(&sm), rawStep, 0.01f);
}

TEST_F(HumiditySmTests, testWaitForConversionOnNack) {
    humidity_sm_run(&sm);  /* MEASURE -> WAIT_FOR_CONVERSION */
    ASSERT_EQ(sm.state, HUMIDITY_SM_WAIT_FOR_CONVERSION);

    /* Sensor still converting - datasheet says this shows up as a NACK/AF. */
    hi2c.ErrorCode = HAL_I2C_ERROR_AF;
    humidity_sm_run(&sm);
    EXPECT_EQ(sm.state, HUMIDITY_SM_WAIT_FOR_CONVERSION);

    hi2c.ErrorCode = HAL_I2C_ERROR_NONE;
    humidity_sm_run(&sm);
    EXPECT_EQ(sm.state, HUMIDITY_SM_UPDATE);
}

TEST_F(HumiditySmTests, testFatalConversionErrorReturnsToMeasureAndFlagsError) {
    humidity_sm_run(&sm);  /* MEASURE -> WAIT_FOR_CONVERSION */
    ASSERT_EQ(sm.state, HUMIDITY_SM_WAIT_FOR_CONVERSION);

    hi2c.ErrorCode = HAL_I2C_ERROR_BERR;
    humidity_sm_run(&sm);

    EXPECT_EQ(sm.state, HUMIDITY_SM_MEASURE);
    EXPECT_NE(bsGetField(TEST_ERR_MSK), 0u);
}

TEST_F(HumiditySmTests, testBusRecoveryClearsErrorOnceSensorResponds) {
    /* Bus goes bad - initiating a measurement fails and flags the error. */
    humiditySensor->txRet = HAL_ERROR;
    humidity_sm_run(&sm);
    ASSERT_EQ(sm.state, HUMIDITY_SM_MEASURE);
    ASSERT_NE(bsGetField(TEST_ERR_MSK), 0u);

    /* While still broken, every attempt re-runs bus recovery and re-tries comms. */
    humidity_sm_run(&sm);
    EXPECT_EQ(sm.state, HUMIDITY_SM_MEASURE);
    EXPECT_NE(bsGetField(TEST_ERR_MSK), 0u);

    /* Bus recovers - next attempt should clear the error and resume measuring. */
    humiditySensor->txRet = HAL_OK;
    humidity_sm_run(&sm);

    EXPECT_EQ(bsGetField(TEST_ERR_MSK), 0u);
    EXPECT_EQ(sm.state, HUMIDITY_SM_WAIT_FOR_CONVERSION);
}

TEST_F(HumiditySmTests, testErrorCountInvalidatesOutputsAfterMaxConsecutiveErrors) {
    humiditySensor->txRet = HAL_ERROR;

    for (int i = 0; i < 9; i++) {
        humidity_sm_run(&sm);
    }
    EXPECT_NE(humidity_sm_get_temp(&sm), 10000.0f);
    EXPECT_NE(humidity_sm_get_rh(&sm), -1.0f);
    EXPECT_NE(humidity_sm_get_ah(&sm), -1.0f);

    /* 10 consecutive failures (spec: "after 10 consecutive measurement errors"). */
    humidity_sm_run(&sm);

    EXPECT_FLOAT_EQ(humidity_sm_get_temp(&sm), 10000.0f);
    EXPECT_FLOAT_EQ(humidity_sm_get_rh(&sm), -1.0f);
    EXPECT_FLOAT_EQ(humidity_sm_get_ah(&sm), -1.0f);
}

TEST_F(HumiditySmTests, testCondensationHeatingTriggersImmediatelyOnHighHumidity) {
    humiditySensor->tempAdc = tempToAdc(25.0f);
    humiditySensor->humAdc  = rhToAdc(90.0f);

    /* MEASURE -> START_HEATING (first heating cycle is not interval-gated) */
    runOneMeasurementCycle();

    EXPECT_EQ(sm.state, HUMIDITY_SM_START_HEATING);
    humidity_sm_run(&sm);  /* START_HEATING -> WAIT_FOR_CONVERSION */

    EXPECT_EQ(humiditySensor->lastCmd, SHT4X_HEATER_200mW_100ms);
    EXPECT_EQ(sm.heatingState.lastHeating, HAL_GetTick());
}

TEST_F(HumiditySmTests, testCondensationHeatingRespectsIntervalBetweenCycles) {
    humiditySensor->tempAdc = tempToAdc(25.0f);
    humiditySensor->humAdc  = rhToAdc(90.0f);

    /* Full first heating cycle: MEASURE -> WAIT -> UPDATE -> START_HEATING -> WAIT -> UPDATE. */
    for (int i = 0; i < 2; i++) {
        runOneMeasurementCycle();
    }
    ASSERT_EQ(sm.state, HUMIDITY_SM_MEASURE);
    uint32_t firstHeatTick = sm.heatingState.lastHeating;

    /* Only 30s later - still high humidity, but too soon to re-heat. */
    forceTick(firstHeatTick + 59000);
    runOneMeasurementCycle();
    EXPECT_EQ(sm.state, HUMIDITY_SM_MEASURE);
    EXPECT_EQ(sm.heatingState.lastHeating, firstHeatTick);

    /* 60s+ later - condensation protection re-triggers the heater. */
    forceTick(firstHeatTick + 60001);
    runOneMeasurementCycle();
    EXPECT_EQ(sm.state, HUMIDITY_SM_START_HEATING);
}

TEST_F(HumiditySmTests, testHeatingSettlingHoldsHumidityOutput) {
    humiditySensor->tempAdc = tempToAdc(25.0f);
    humiditySensor->humAdc  = rhToAdc(90.0f);

    /* Full first heating cycle: MEASURE -> WAIT -> UPDATE -> START_HEATING -> WAIT -> UPDATE. */
    for (int i = 0; i < 2; i++) {
        runOneMeasurementCycle();
    }
    ASSERT_EQ(sm.state, HUMIDITY_SM_MEASURE);
    float tempBefore = humidity_sm_get_temp(&sm);
    float rhBefore   = humidity_sm_get_rh(&sm);

    /* Sensor has warmed up from the heater, well inside the settling window. Humidity has
    ** genuinely dropped too, but the reading must not be trusted until temp settles. */
    humiditySensor->tempAdc = tempToAdc(30.0f);
    humiditySensor->humAdc  = rhToAdc(50.0f);
    forceTick(sm.heatingState.lastHeating + 1000);

    runOneMeasurementCycle();

    EXPECT_GT(humidity_sm_get_temp(&sm), tempBefore);
    EXPECT_FLOAT_EQ(humidity_sm_get_rh(&sm), rhBefore);

    /* Verify the RH value becomes live again once temperature returns to normal */
    humiditySensor->tempAdc = tempToAdc(25.0f);
    for (int i = 0; i < 100; i++) { /* "1 second" to clear IIR filter */
        runOneMeasurementCycle();
    }
    EXPECT_NEAR(humidity_sm_get_rh(&sm), 50.0f, 0.01);
}

TEST_F(HumiditySmTests, testBurninUsesDifferentHeaterProgramEveryCycle) {
    humiditySensor->tempAdc = tempToAdc(25.0f);
    humiditySensor->humAdc  = rhToAdc(20.0f);  /* Low humidity - would not heat outside burn-in. */

    humidity_sm_start_burnin(&sm);
    EXPECT_TRUE(humidity_sm_in_burnin(&sm));

    runOneMeasurementCycle(); 
    EXPECT_EQ(sm.state, HUMIDITY_SM_START_HEATING);

    humidity_sm_run(&sm);  /* START_HEATING -> WAIT */
    EXPECT_EQ(humiditySensor->lastCmd, SHT4X_HEATER_110mW_1s);
}

TEST_F(HumiditySmTests, testBurninTemperatureProtectionSkipsHeating) {
    humiditySensor->tempAdc = tempToAdc(85.0f);  /* Above HUMIDITY_SM_MAX_TEMP_BEFORE_HEATING (80C). */
    humiditySensor->humAdc  = rhToAdc(20.0f);

    humidity_sm_start_burnin(&sm);
    runOneMeasurementCycle(); 

    EXPECT_EQ(sm.state, HUMIDITY_SM_MEASURE);
}

TEST_F(HumiditySmTests, testBurninAutoStopsAfterBurnInTime) {
    humiditySensor->tempAdc = tempToAdc(25.0f);
    humiditySensor->humAdc  = rhToAdc(20.0f);

    forceTick(0);
    humidity_sm_start_burnin(&sm);

    forceTick(HUMIDITY_SM_BURN_IN_TIME + 1);
    runOneMeasurementCycle();

    EXPECT_FALSE(humidity_sm_in_burnin(&sm));
}

TEST_F(HumiditySmTests, testStopBurninExitsImmediately) {
    humidity_sm_start_burnin(&sm);
    ASSERT_TRUE(humidity_sm_in_burnin(&sm));

    humidity_sm_stop_burnin(&sm);
    EXPECT_FALSE(humidity_sm_in_burnin(&sm));
}

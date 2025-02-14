/*!
 * @file    ADS7953_tests.cpp
 * @brief   This file contains the unit test functions for the ADS7953
 * @date    12/02/2025
 * @author  Timothé D
*/

#include <gtest/gtest.h>
#include <gmock/gmock.h>

/* Fakes */
#include "fake_stm32xxxx_hal.h"

/* Real supporting units */

/* UUT */
#include "ADS7953.c"

using namespace std;

/***************************************************************************************************
** TEST FIXTURES
***************************************************************************************************/

class ADS7953Test: public ::testing::Test 
{
    protected:
        /*******************************************************************************************
        ** METHODS
        *******************************************************************************************/
        ADS7953Test() {}

        static void generateSine(int16_t* pData, int noOfChannels, int noOfSamples, int channel,  int amplitudeOffset, int amplitude, int freq)
        {
            const float Ts = 1.0/10000.0;
            for (int i = 0; i < noOfSamples; i++)
                pData[noOfChannels*i + channel] = amplitudeOffset + (amplitude * sin(2*M_PI*(i*Ts)*freq));
        }

    ADS7953Device_t dev;
    DMA_HandleTypeDef hdma1 = { .Instance = DMA1_Stream1 };
    DMA_HandleTypeDef hdma2 = { .Instance = DMA1_Stream2 };
    DMA_HandleTypeDef hdma3 = { .Instance = DMA1_Stream3 };
    DMA_HandleTypeDef hdma4 = { .Instance = DMA1_Stream4 };
    TIM_HandleTypeDef htim  = { .Instance = TIM3 };
    SPI_HandleTypeDef hspi  = { .Instance = SPI2, .hdmarx = &hdma1 };

    ADS7953DMAs_t DMAs = {
        .hdma_spi_rx        = &hdma1,
        .hdma_tim_receiving = &hdma2,
        .hdma_tim_disabling = &hdma3,
        .hdma_tim_enabling  = &hdma4
    };
};

/***************************************************************************************************
** TESTS
***************************************************************************************************/

TEST_F(ADS7953Test, testIsBufferHealthy)
{
    const float tol = 0.0001;
    const int noOfSamples = 1000;
    const int noOfChannels = 2;
    int16_t pData[noOfSamples*noOfChannels*2] = {0};

    generateSine(pData, noOfChannels, noOfSamples, 0, 2047, 2047, 1000);
    generateSine(pData, noOfChannels, noOfSamples, 1, 2047, 1023, 1000);

    for (int i = 0; i < noOfSamples; i++)
    {
        pData[noOfChannels*i + 1] |= 0x1000;
    }

    ADS7953Init(&dev, &hspi, &htim, DMAs, pData, sizeof(pData)/sizeof(int16_t), noOfChannels);

    EXPECT_EQ(isBufferHealthy(&dev, pData), true);
    EXPECT_NEAR(extADCRms(&dev, pData, 0, noOfSamples), 2506.606445, tol);
    EXPECT_NEAR(extADCRms(&dev, pData, 1, noOfSamples), 2170.527588, tol);

    for (int i = 0; i < noOfSamples; i++)
    {
        pData[noOfChannels*i + 1] |= 0x1000;
    }
    pData[noOfChannels*10] |= 0x1000;

    EXPECT_EQ(isBufferHealthy(&dev, pData), false);
}

TEST_F(ADS7953Test, testExtADCMax)
{
    const int noOfSamples = 1000;
    const int noOfChannels = 5;
    int16_t pData[noOfSamples*noOfChannels*2] = {0};

    for (int i = 0; i < noOfSamples; i++)
    {
        pData[noOfChannels*i] = i*1;
        pData[noOfChannels*i + 1] = i*2;
    }

    const int AMPLITUDE = 2047; 
    const int OFFSET_1 = 2047;
    const int OFFSET_2 = 0;
    const int OFFSET_3 = -2047;

    generateSine(pData, noOfChannels, noOfSamples, 2, OFFSET_1, AMPLITUDE, 1000);
    generateSine(pData, noOfChannels, noOfSamples, 3, OFFSET_2, AMPLITUDE, 1000);
    generateSine(pData, noOfChannels, noOfSamples, 4, OFFSET_3, AMPLITUDE, 1000);

    ADS7953Init(&dev, &hspi, &htim, DMAs, pData, sizeof(pData)/sizeof(int16_t), noOfChannels);

    EXPECT_EQ(extADCMax(&dev, pData, 0), noOfSamples - 1);
    EXPECT_EQ(extADCMax(&dev, pData, 1), (noOfSamples - 1)*2);
    EXPECT_EQ(extADCMax(&dev, pData, 2), 3993);
    EXPECT_EQ(extADCMax(&dev, pData, 3), 1946);
    EXPECT_EQ(extADCMax(&dev, pData, 4), -100);
}

TEST_F(ADS7953Test, testExtADCMin)
{
    const int noOfSamples = 1000;
    const int noOfChannels = 5;
    int16_t pData[noOfSamples*noOfChannels*2] = {0};

    for (int i = 0; i<noOfSamples; i++)
    {
        pData[noOfChannels*i] = i*1 + 12;
        pData[noOfChannels*i+1] = -i*2;
    }

    const int AMPLITUDE = 2047;
    const int OFFSET_1 = 2047;
    const int OFFSET_2 = 0;
    const int OFFSET_3 = -2047;
    generateSine(pData, noOfChannels, noOfSamples, 2, OFFSET_1, AMPLITUDE, 1000);
    generateSine(pData, noOfChannels, noOfSamples, 3, OFFSET_2, AMPLITUDE, 1000);
    generateSine(pData, noOfChannels, noOfSamples, 4, OFFSET_3, AMPLITUDE, 1000);

    ADS7953Init(&dev, &hspi, &htim, DMAs, pData, sizeof(pData)/sizeof(int16_t), noOfChannels);

    EXPECT_EQ(extADCMin(&dev, pData, 0), 12);
    EXPECT_EQ(extADCMin(&dev, pData, 1), (1 - noOfSamples)*2);
    EXPECT_EQ(extADCMin(&dev, pData, 2), 100);
    EXPECT_EQ(extADCMin(&dev, pData, 3), -1946);
    EXPECT_EQ(extADCMin(&dev, pData, 4), -3993);
}

TEST_F(ADS7953Test, testExtADCMean)
{
    const int noOfSamples = 100;
    const int noOfChannels = 2;
    int16_t pData[noOfSamples*noOfChannels*2] = {0};

    for (int i = 0; i < noOfSamples; i++)
    {
        pData[noOfChannels*i] = i*1 + 1;
        pData[noOfChannels*i + 1] = i*2 + 1;
    }

    ADS7953Init(&dev, &hspi, &htim, DMAs, pData, sizeof(pData)/sizeof(int16_t), noOfChannels);

    EXPECT_EQ(extADCMean(&dev, pData, 0), 50.5);
    EXPECT_EQ(extADCMean(&dev, pData, 1), 100);
}

TEST_F(ADS7953Test, testExtADCRms)
{
    const float tol = 0.0001;
    const int noOfSamples = 1000;
    const int noOfChannels = 2;
    int16_t pData[noOfSamples*noOfChannels*2] = {0};

    generateSine(pData, noOfChannels, noOfSamples, 0, 2047, 2047, 1000);
    generateSine(pData, noOfChannels, noOfSamples, 1, 2047, 1023, 1000);

    ADS7953Init(&dev, &hspi, &htim, DMAs, pData, sizeof(pData)/sizeof(int16_t), noOfChannels);

    EXPECT_NEAR(extADCRms(&dev, pData, 0, noOfSamples), 2506.606445, tol);
    EXPECT_NEAR(extADCRms(&dev, pData, 1, noOfSamples), 2170.527588, tol);
}

TEST_F(ADS7953Test, testExtADCSetOffset)
{
    const int noOfSamples = 1000;
    const int noOfChannels = 3;
    int16_t pData[noOfSamples*noOfChannels*2] = {0};

    int16_t dcValues[3] = {2055, 4085, 16};
    for (int i = 0; i < noOfSamples; i++)
    {
        pData[noOfChannels*i] = dcValues[0];
        pData[noOfChannels*i + 1] = dcValues[1];
        pData[noOfChannels*i + 2] = dcValues[2];
    }

    ADS7953Init(&dev, &hspi, &htim, DMAs, pData, sizeof(pData)/sizeof(int16_t), noOfChannels);

    extADCSetOffset(&dev, pData, 0, -dcValues[0]);
    extADCSetOffset(&dev, pData, 1, -dcValues[1]);
    extADCSetOffset(&dev, pData, 2, -dcValues[2]);

    for (int i = 0; i < noOfChannels*noOfSamples; i++)
    {
        EXPECT_EQ(pData[i], 0);
    }
}

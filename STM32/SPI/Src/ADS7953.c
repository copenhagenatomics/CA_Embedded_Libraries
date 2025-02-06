/*!
 * @file    ADS7953.c
 * @brief   This file contains the functions implementing the ADS7953
 * @date    28/10/2024
 * @author  Timothé D
*/

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include "stm32f4xx_hal.h"
#include "StmGpio.h"
#include "ADS7953.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

#define MAX_DEVICES_NO      5   // 5 SPIs on the STM32F4
#define MAX_CHANNELS_NO     16  // 16 channels on the ADC
#define SPI_TIMEOUT         10  // Timeout used for SPI transmit

typedef union
{
    uint8_t data[2];
    uint16_t fullMessage;
} ADS7953Message_t;

/***************************************************************************************************
** PRIVATE FUNCTION DECLARATIONS
***************************************************************************************************/

static uint16_t lastInputProgramming(uint16_t noOfChannels);
static uint16_t getChannelAddress(uint16_t message);
static uint16_t getConversionResult(uint16_t message);
static uint16_t createModeCtrllRegMessage();
static uint16_t createProgramRegMessage(uint16_t noOfChannels);
static int setRegisters(ADS7953Device_t *dev);
static void bufferHalfFullCallback(DMA_HandleTypeDef *hdma);
static void bufferFullCallback(DMA_HandleTypeDef *hdma);
static void initDMA(ADS7953Device_t *dev);
static void initTimer(ADS7953Device_t *dev);

/***************************************************************************************************
** PRIVATE OBJECTS
***************************************************************************************************/

/* 
This array stores the adresses of all the ADS7953 initialized
It is needed in bufferHalfFullCallback() and bufferFullCallback()
as the device pointer cannot be passed directly
*/
static ADS7953Device_t *listOfDevices[MAX_DEVICES_NO] = {NULL};

/***************************************************************************************************
** PRIVATE FUNCTION DEFINITIONS
***************************************************************************************************/

/*!
 * @brief   Create part of message to setup the last ADC channel
 * @param   noOfInputs Number of ADC channels used
 * @return  Part of message
*/
static uint16_t lastInputProgramming(uint16_t noOfInputs)
{
    return (((noOfInputs - 1) & 0x0FU) << 6);
}

/*!
 * @brief   Extraction of channel address from SPI message
 * @param   message SPI message received
 * @return  ADC channel address
*/
static uint16_t getChannelAddress(uint16_t message)
{
    return ((message & 0xF000U) >> 12);
}

/*!
 * @brief   Extraction of ADC value from SPI message
 * @param   message SPI message received
 * @return  12-bits ADC value
*/
static uint16_t getConversionResult(uint16_t message)
{
    return (message & 0x0FFFU);
}

/*!
 * @brief   Creation of Mode Control Register Message
 * @return  SPI message
*/
static uint16_t createModeCtrllRegMessage()
{
    static const uint16_t AUTO_2_MODE_SELECT = 0x3000;
    static const uint16_t ENABLE_PROGRAMMING = 0x0800;
    static const uint16_t CHANNEL_COUNTER_RESET = 0x0400;
    static const uint16_t DOUBLE_V_REF_SELECT = 0x0040;

    return (AUTO_2_MODE_SELECT | ENABLE_PROGRAMMING | CHANNEL_COUNTER_RESET | DOUBLE_V_REF_SELECT);
}

/*!
 * @brief   Creation of Program Register Message
 * @param   noOfChannels Number of ADC channels used
 * @return  SPI message
*/
static uint16_t createProgramRegMessage(uint16_t noOfChannels)
{
    uint16_t AUTO_2_PROGRAM_REGISTER = 0x9000U;

    return (AUTO_2_PROGRAM_REGISTER | lastInputProgramming(noOfChannels));
}

/*!
 * @brief   Sends 2 configuration messages to the ADS7953 selected
 * @param   dev Pointer to the ADC structure
*/
static int setRegisters(ADS7953Device_t *dev)
{
    // Messages
    ADS7953Message_t modeControlRegisterMessage = { .fullMessage = createModeCtrllRegMessage() };
    ADS7953Message_t programRegisterMessage = { .fullMessage = createProgramRegMessage(dev->noOfChannels) };

    HAL_Delay(1);                   // Necessary to wait until the NSS pin is high (pull-up)
    __HAL_SPI_ENABLE(dev->hspi);    // The NSS pin can be forced low at for the next SPI message

    if (HAL_SPI_Transmit(dev->hspi, modeControlRegisterMessage.data, sizeof(ADS7953Message_t)/sizeof(uint16_t), SPI_TIMEOUT) != HAL_OK)
    {
        return -1;
    }

    __HAL_SPI_DISABLE(dev->hspi);   // The NSS pin isn't forced low anymore
    HAL_Delay(1);                   // Necessary to wait until the NSS pin is high (pull-up)
    __HAL_SPI_ENABLE(dev->hspi);    // The NSS pin can be forced low at for the next SPI message

    if (HAL_SPI_Transmit(dev->hspi, programRegisterMessage.data, sizeof(ADS7953Message_t)/sizeof(uint16_t), SPI_TIMEOUT) != HAL_OK)
    {
        return -2;
    }

    __HAL_SPI_DISABLE(dev->hspi);   // The NSS pin isn't forced low anymore
    HAL_Delay(1);                   // Necessary to wait until the NSS pin is high (pull-up)

    return 0;
}

/*!
 * @brief   Interrupt function that is called when the first half of the buffer is filled
 * @param   hdma Pointer to DMA handler
*/
static void bufferHalfFullCallback(DMA_HandleTypeDef *hdma)
{
    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef*) hdma->Parent;
    int deviceIndex = 0;
    while (hspi != listOfDevices[deviceIndex]->hspi)
    {
        deviceIndex++;
    }
    listOfDevices[deviceIndex]->activeBuffer = FirstPart;
}

/*!
 * @brief   Interrupt function that is called when the second half of the buffer is filled
 * @param   hdma Pointer to DMA handler
*/
static void bufferFullCallback(DMA_HandleTypeDef *hdma)
{
    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef*) hdma->Parent;
    int deviceIndex = 0;
    while (hspi != listOfDevices[deviceIndex]->hspi)
    {
        deviceIndex++;
    }
    listOfDevices[deviceIndex]->activeBuffer = SecondPart;
}

/*!
 * @brief   Interrupt function that is called when the second half of the buffer is filled
 * @param   hdma Pointer to DMA handler
*/
static void initDMA(ADS7953Device_t *dev)
{
    // Dummy byte to fill the data register of the SPI
    static const uint16_t INITIATE_TRANSFER = 0;

    /*
    CR1 register when the SPI is disabled
    NSS pin goes high because of pull-up
    - MSTR = 1 --> Master mode
    - BR_0 = 1 --> 10 kbps baudrate
    - SSI  = 1 --> Hardware NSS pin
    - DFF  = 1 --> 16-bits messages
    - SPE  = 0 --> SPI disabled
    */
    static const uint16_t CR1_OFF = SPI_CR1_MSTR | SPI_CR1_BR_0 | SPI_CR1_SSI | SPI_CR1_DFF;

    /*
    CR1 register when the SPI is enabled
    NSS pin goes low
    - SPE  = 1 --> SPI enabled
    */
    static const uint16_t CR1_ON = CR1_OFF | SPI_CR1_SPE;

    // Half-complete and complete interrupt functions
    dev->hspi->hdmarx->XferHalfCpltCallback = bufferHalfFullCallback;
    dev->hspi->hdmarx->XferCpltCallback = bufferFullCallback;

    // Start DMA handle from RX register to data buffer
    HAL_DMA_Start_IT(dev->hdma_spi_rx, (uint32_t) &dev->hspi->Instance->DR, (uint32_t) dev->buffer, dev->bufLength);

    // Enable RX DMA functionality
    SET_BIT(dev->hspi->Instance->CR2, SPI_CR2_RXDMAEN);

    // Enable SPI handle
    __HAL_SPI_ENABLE(dev->hspi);
    
    // Timer DMA that transfers a dummy byte to SPI TX register to initiate master receive
    HAL_DMA_Start(dev->hdma_tim_receiving, (uint32_t) &INITIATE_TRANSFER, (uint32_t) &dev->hspi->Instance->DR, sizeof(INITIATE_TRANSFER)/sizeof(uint16_t));

    // Timer DMAs that sets/resets the SPE bit of CR1, resulting in an NSS pin alternating between high and low (due to pull-up setup)
    HAL_DMA_Start(dev->hdma_tim_disabling, (uint32_t) &CR1_OFF, (uint32_t) &dev->hspi->Instance->CR1, sizeof(CR1_OFF)/sizeof(uint16_t));
    HAL_DMA_Start(dev->hdma_tim_enabling, (uint32_t) &CR1_ON, (uint32_t) &dev->hspi->Instance->CR1, sizeof(CR1_ON)/sizeof(uint16_t));
}

/*!
 * @brief   Initialization of the timer used for DMA
 * @note    The timer 
 * @param   dev Pointer to the ADC structure
*/
static void initTimer(ADS7953Device_t *dev)
{
    // Activation of DMA interrupts
    SET_BIT(dev->htim->Instance->DIER, TIM_DIER_CC1DE);
    SET_BIT(dev->htim->Instance->DIER, TIM_DIER_CC2DE);
    SET_BIT(dev->htim->Instance->DIER, TIM_DIER_UDE);

    // Timer start
    HAL_TIM_OC_Start(dev->htim, TIM_CHANNEL_1);     // Triggers hdma_tim_disabling (stops the SPI, NSS goes high)
    HAL_TIM_OC_Start(dev->htim, TIM_CHANNEL_2);     // Triggers hdma_tim_enabling (starts the SPI, NSS goes low)
    HAL_TIM_Base_Start(dev->htim);                  // Triggers hdma_tim_receiving (starts receiving SPI message)
}

/***************************************************************************************************
** PUBLIC FUNCTION DEFINITIONS
***************************************************************************************************/

/*!
 * @brief   Verification of ADC buffer health
 * @note    Checks that no shift in measurement happened and removes the channel info (last 4 bits)
 * @param   dev Pointer to the ADC structure
 * @param   pData Pointer to the ADC buffer
 * @return  1 if the buffer is OK
*/
bool isBufferHealthy(ADS7953Device_t *dev, int16_t *pData)
{
    if (pData == NULL)
    {
        return false;
    }

    for (uint16_t channel = 0; channel < dev->noOfChannels; channel++)
    {
        for (uint16_t sampleId = 0; sampleId < dev->noOfSamples; sampleId++)
        {
            if (getChannelAddress(pData[sampleId * dev->noOfChannels + channel]) != channel)
            {
                return false;
            }
            pData[sampleId * dev->noOfChannels + channel] = getConversionResult(pData[sampleId * dev->noOfChannels + channel]);
        }
    }

    return true;
}

/*!
 * @brief   Calculation of maximum
 * @param   dev Pointer to the ADC structure
 * @param   pData Pointer to the ADC buffer
 * @param   channel ADC channel
 * @return  Maximum of buffer for given channel
*/
int16_t extADCMax(ADS7953Device_t *dev, int16_t *pData, uint16_t channel)
{
    if (pData == NULL || channel >= dev->noOfChannels)
    {
        return 0;
    }

    int16_t max = pData[0];
    for (uint16_t sampleId = 1; sampleId < dev->noOfSamples; sampleId++)
    {
        int16_t sample = pData[sampleId * dev->noOfChannels + channel];
        if (sample > max)
        {
            max = sample;
        }
    }
    return max;
}

/*!
 * @brief   Calculation of minimum
 * @param   dev Pointer to the ADC structure
 * @param   pData Pointer to the ADC buffer
 * @param   channel ADC channel
 * @return  Minimum of buffer for given channel
*/
int16_t extADCMin(ADS7953Device_t *dev, int16_t *pData, uint16_t channel)
{
    if (pData == NULL || channel >= dev->noOfChannels)
    {
        return 0;
    }

    int16_t min = pData[0];
    for (uint16_t sampleId = 1; sampleId < dev->noOfSamples; sampleId++)
    {
        int16_t sample = pData[sampleId * dev->noOfChannels + channel];
        if (sample < min)
        {
            min = sample;
        }
    }
    return min;
}

/*!
 * @brief   Calculation of average
 * @param   dev Pointer to the ADC structure
 * @param   pData Pointer to the ADC buffer
 * @param   channel ADC channel
 * @return  Mean of buffer for given channel
*/
double extADCMean(ADS7953Device_t *dev, int16_t *pData, uint16_t channel)
{
    if (pData == NULL || channel >= dev->noOfChannels)
    {
        return 0;
    }

    int32_t sum = 0;
    for (uint32_t sampleId = 0; sampleId < dev->noOfSamples; sampleId++)
    {
        sum += pData[sampleId * dev->noOfChannels + channel];
    }
    return (((double) sum) / ((double) dev->noOfSamples));
}

/*!
 * @brief   Calculation of RMS
 * @param   dev Pointer to the ADC structure
 * @param   pData Pointer to the ADC buffer
 * @param   channel ADC channel
 * @param   noOfPoints Number of points to consider
 * @return  RMS of buffer for given channel
*/
double extADCRms(ADS7953Device_t *dev, int16_t *pData, uint16_t channel, uint32_t noOfPoints)
{
    if (pData == NULL || channel >= dev->noOfChannels || noOfPoints > dev->noOfSamples)
    {
        return 0;
    }

    double sum = 0;
    for (uint32_t sampleId = 0; sampleId < dev->noOfSamples; sampleId++)
    {
        double mult = pData[sampleId * dev->noOfChannels + channel];
        sum += (mult * mult);
    }
    return sqrt(sum / ((double) dev->noOfSamples));
}

/*!
 * @brief   Application of offset
 * @param   dev Pointer to the ADC structure
 * @param   pData Pointer to the ADC buffer
 * @param   channel ADC channel
 * @param   offset Offset
*/
void extADCSetOffset(ADS7953Device_t *dev, int16_t *pData, uint16_t channel, int16_t offset)
{
    if (pData == NULL || channel >= dev->noOfChannels)
    {
        return;
    }

    for (uint32_t sampleId = 0; sampleId < dev->noOfSamples; sampleId++)
    {
        pData[sampleId * dev->noOfChannels + channel] += offset;
    }
}

/*!
 * @brief   Configuration of an ADS7953 device
 * @note    Requires that CHANNEL_2 and CHANNEL_3 of the timer are used as output compare
 * @param   dev Pointer to the ADC structure
 * @param   htim Pointer to the timer used for DMA
 * @param   hspi Pointer to the SPI handler
 * @param   buff Pointer to the ADC buffer
 * @param   length Buffer length
 * @param   noOfChannels Number of inputs used
 * @param   hdma_spi_rx Pointer to the SPI RX DMA handler
 * @param   hdma_tim_receiving Pointer to the timer DMA handler triggering the SPI receive
 * @param   hdma_tim_disabling Pointer to the timer DMA handler stopping the SPI to set the NSS pin high
 * @param   hdma_tim_enabling Pointer to the timer DMA handler starting the SPI to set the NSS pin low
 * @return  0 on success, else negative value
*/
int ADS7953Init(ADS7953Device_t *dev, TIM_HandleTypeDef *htim, SPI_HandleTypeDef *hspi, int16_t *buff, uint32_t length, uint8_t noOfChannels, DMA_HandleTypeDef *hdma_spi_rx, DMA_HandleTypeDef *hdma_tim_receiving, DMA_HandleTypeDef *hdma_tim_disabling, DMA_HandleTypeDef *hdma_tim_enabling)
{
    if ((noOfChannels > MAX_CHANNELS_NO) || (noOfChannels < 1))
    {
        return -1;
    }

    // SPI
    dev->hspi = hspi;
    dev->hdma_spi_rx = hdma_spi_rx;

    // Timer
    dev->htim = htim;
    dev->hdma_tim_receiving = hdma_tim_receiving;
    dev->hdma_tim_disabling = hdma_tim_disabling;
    dev->hdma_tim_enabling = hdma_tim_enabling;
    
    // Buffer
    dev->buffer = buff;
    dev->bufLength = length;
    dev->noOfChannels = noOfChannels;
    dev->noOfSamples = length / (2 * noOfChannels);
    dev->lastBuffer = SecondPart;                       // Second part as the DMA will start by filling the first part
    dev->activeBuffer = SecondPart;                     // Same as last buffer so that the callback function isn't called now

    // Registration of device
    int deviceIndex = 0;
    while (listOfDevices[deviceIndex] != NULL)          // To find the next empty spot in the listOfDevices array
    {
        deviceIndex++;
        if (deviceIndex == MAX_DEVICES_NO)
        {
            return -2;
        }
    }
    listOfDevices[deviceIndex] = dev;
    
    // ADC registers configuration
    if (setRegisters(dev) != 0) 
    {
        return -3;
    }

    // DMA initialization
    initDMA(dev);

    // Timer initialization
    initTimer(dev);     

    return 0;
}

/*!
 * @brief   Resets the ADC
 * @note    ADS7953Init must be called before this function
 * @param   dev Pointer to the ADC structure
*/
int ADS7953Reset(ADS7953Device_t *dev)
{
    // Stopping the timer triggering the DMAs
    HAL_TIM_Base_Stop(dev->htim);
    HAL_TIM_OC_Stop(dev->htim, TIM_CHANNEL_1);
    HAL_TIM_OC_Stop(dev->htim, TIM_CHANNEL_2);

    // Stopping the RX DMA Stream
    __HAL_DMA_DISABLE(dev->hdma_spi_rx);

    // ADC registers configuration
    if (setRegisters(dev) != 0) 
    {
        return -1;
    }

    // Resets initial buffer state
    dev->lastBuffer = SecondPart;
    dev->activeBuffer = SecondPart;

    // Restarting the RX DMA Stream
    __HAL_DMA_ENABLE(dev->hdma_spi_rx);

    // Restarting the timer triggering the DMAs
    HAL_TIM_OC_Start(dev->htim, TIM_CHANNEL_1);
    HAL_TIM_OC_Start(dev->htim, TIM_CHANNEL_2);
    HAL_TIM_Base_Start(dev->htim);

    return 0;
}

/*!
 * @brief   Loop to update the ADC buffer
 * @note    Called in Loop function
 * @note    ADS7953Init must be called before this function
 * @param   dev Pointer to the ADC structure
 * @param   callback Callback function to use the ADC values
*/
void ADS7953Loop(ADS7953Device_t *dev, extADCCallBack callback)
{
    // The the buffer is half-full or full
    if (dev->activeBuffer != dev->lastBuffer)
    {
        dev->lastBuffer = dev->activeBuffer;

        int16_t *pData = NULL;
        if (dev->activeBuffer == FirstPart)
        {
            pData = dev->buffer;
        }
        else
        {
            pData = &dev->buffer[dev->bufLength / 2];
        }

        callback(pData);
    }
}

/*!
 * @file    AD5683.c
 * @brief   This file contains the functions implementing the AD5683
 * @ref     https://www.analog.com/media/en/technical-documentation/data-sheets/AD5683R_5682R_5681R_5683.pdf
 * @date    04/03/2026
 * @author  Valdemar H. Ramussen
 */

#include "AD5683.h"
#include "USBprint.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include "StmGpio.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

/* Command definitions from Table 10 of AD5683R datasheet */
#define AD5683R_CMD_NOP                     0x0
#define AD5683R_CMD_WRITE_INPUT             0x1
#define AD5683R_CMD_SOFTWARE_LDAC           0x2
#define AD5683R_CMD_WRITE_UPDATE_DAC        0x3
#define AD5683R_CMD_WRITE_DAC_ONLY          0x4
#define AD5683R_CMD_POWER_DOWN_UP           0x5
#define AD5683R_CMD_LOAD_INT_REF            0x7
#define AD5683R_CMD_SET_GAIN                0x9
#define AD5683R_CMD_SET_REF_SEL             0xA
#define AD5683R_CMD_WRITE_CONFIG            0xB

/* Timeout for SPI transmit operations */
#define AD5683R_TIMEOUT    10

/***************************************************************************************************
** PRIVATE FUNCTIONS
***************************************************************************************************/

/**
 * @brief  Helper function to format a 24-bit SPI packet and transmit.
 * @note   This formats standard commands that use a 16-bit data value 
 * placed in the DB19 to DB4 bits. The final 4 bits (DB3-DB0) 
 * are sent as zeros.
 * @param  dev Pointer to the AD5683R device structure
 * @param  command 4-bit command code (0-15) to be placed in DB19-DB16
 * @param  data 16-bit data value to be placed in DB19-DB4
 */
static void AD5683R_Send_16bit_Data(ad5683r_t *dev, uint8_t command, uint16_t data)
{
    uint8_t tx_buffer[3];
    
    /* * Format the 24-bit packet (MSB first):
     * tx_buffer[0] = [Command(3:0)] [Data(15:12)]
     * tx_buffer[1] = [Data(11:4)]
     * tx_buffer[2] = [Data(3:0)] [0 0 0 0]  (DB3-DB0 are don't care, sent as zero)
     */
    tx_buffer[0] = (command << 4) | ((data >> 12) & 0x0F);
    tx_buffer[1] = (data >> 4) & 0xFF;
    tx_buffer[2] = (data << 4) & 0xF0;

    /* Manually pull CS low to begin transmission */
    stmSetGpio(*dev->csPin, 0);
    
    /* Transmit the 3-byte data. The received value is ignored. */
    if (HAL_SPI_Transmit(dev->hspi, tx_buffer, 3, AD5683R_TIMEOUT) != HAL_OK)
    {
        /* An error callback or flag can be added here for robust error handling */
    }

    /* Manually pull CS high to finalize the transmission and update the DAC */
    stmSetGpio(*dev->csPin, 1);
}


/***************************************************************************************************
** PUBLIC FUNCTIONS
***************************************************************************************************/

/**
 * @brief  Initialize the AD5683R device.
 * @param  dev Pointer to the AD5683R device structure
 * @param  hspi Pointer to the SPI handle
 * @param  csPin Pointer to the chip select GPIO pin
 * @param  resetPin Pointer to the reset GPIO pin
 */
void AD5683R_Init(ad5683r_t *dev, SPI_HandleTypeDef *hspi, StmGpio *csPin, StmGpio *resetPin)
{
    dev->hspi = hspi;
    dev->csPin = csPin;
    dev->resetPin = resetPin;

    /* Ensure GPIO pins are in their inactive (high) state initially */
    stmSetGpio(*dev->csPin, 1);
    stmSetGpio(*dev->resetPin, 1);
    //stmSetGpio(*dev->ldacPin, 1);

    /* Perform a physical hardware reset pulse */
    AD5683R_Reset(dev);
    
    /* Perform a first NOP transmission to ensure a clean state */
    AD5683R_Send_16bit_Data(dev, AD5683R_CMD_NOP, 0x0000);
}

/**
 * @brief  Reset the AD5683R device.
 * @param  dev Pointer to the AD5683R device structure
 */
void AD5683R_Reset(ad5683r_t *dev)
{
    /* Pulse the RESET pin low
     */
    stmSetGpio(*dev->resetPin, 0);
    HAL_Delay(1);
    stmSetGpio(*dev->resetPin, 1);
    
    /* Small delay after reset pulse (reset recovery) before any new commands.
     * Table 10 doesn't specify an explicit reset recovery, but good practice.
     */
    HAL_Delay(1);
}

/**
 * @brief  Set the value of the AD5683R device.
 * @param  dev Pointer to the AD5683R device structure
 * @param  data The 16-bit data value to set
 */
void AD5683R_SetValue(ad5683r_t *dev, uint16_t data)
{
    /* Use command 0x3 (Write to input register, update DAC register) for single-command update. */
    AD5683R_Send_16bit_Data(dev, AD5683R_CMD_WRITE_UPDATE_DAC, data);
}

/**
 * @brief  Set the power mode of the AD5683R device.
 * @param  dev Pointer to the AD5683R device structure
 * @param  mode The power mode to set
 */
void AD5683R_PowerMode(ad5683r_t *dev, AD5683R_PowerMode_t mode)
{
    uint8_t tx_buffer[3];
    uint8_t command = AD5683R_CMD_POWER_DOWN_UP;
    
    /* * Command 0x5 formatting requires specific bits in the 24-bit stream.
     * Table 10 and 11: Power mode PD1, PD0 are in DB1, DB0 of the stream.
     * All DB19-DB2 don't care (sent as zero).
     * * Byte 0: [0 1 0 1] [0 0 0 0]
     * Byte 1: [0 0 0 0] [0 0 0 0]
     * Byte 2: [0 0 0 0] [0 0 PD1 PD0]
     */
    tx_buffer[0] = (command << 4);
    tx_buffer[1] = 0x00;
    tx_buffer[2] = (mode & 0x3); /* DB1, DB0 of the 24-bit word */
    
    /* Pulse CS low and transmit the 3 bytes */
    stmSetGpio(*dev->csPin, 0);
    HAL_SPI_Transmit(dev->hspi, tx_buffer, 3, AD5683R_TIMEOUT);
    stmSetGpio(*dev->csPin, 1);
}

/**
 * @brief  Set the internal reference of the AD5683R device.
 * @param  dev Pointer to the AD5683R device structure
 * @param  enable The enable flag for the internal reference
 */
void AD5683R_InternalRef(ad5683r_t *dev, uint8_t enable)
{
    uint16_t data = 0x0000;
    if (enable) { data = 1; }
    
    AD5683R_Send_16bit_Data(dev, AD5683R_CMD_LOAD_INT_REF, data);
}
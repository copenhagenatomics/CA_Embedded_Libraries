/*!
 * @file    ADS114S08.c
 * @brief   This file contains the functions implementing the ADS114S08
 * @ref     https://www.ti.com/lit/ds/symlink/ads114s08b.pdf
 * @date    18/03/2026
 * @author  Valdemar H. Ramussen
 * @note    This implementation is currently tailored to the specific use case of reading 6 channels in single-shot mode at 400 SPS for the Analog Input Calibration. For a more general driver, additional functions and configurability would be needed.
 */

#include "ADS114S08.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>
#include "StmGpio.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

/* --- Commands --- */
#define ADS_CMD_NOP       0x00
#define ADS_CMD_WAKEUP    0x02
#define ADS_CMD_POWERDOWN 0x04
#define ADS_CMD_RESET     0x06
#define ADS_CMD_START     0x08
#define ADS_CMD_STOP      0x0A
#define ADS_CMD_RDATA     0x12
#define ADS_CMD_RREG      0x20
#define ADS_CMD_WREG      0x40

/* --- Registers --- */
#define ADS_REG_STATUS    0x01
#define ADS_REG_INPMUX    0x02
#define ADS_REG_PGA       0x03
#define ADS_REG_DATARATE  0x04
#define ADS_REG_REF       0x05

/* --- Multiplexer Definitions --- */
#define ADS_MUX_AINCOM    0x0C

#define SPI_TIMEOUT 10

/***************************************************************************************************
** PRIVATE FUNCTIONS
***************************************************************************************************/

// Prototypes for private functions
static void ADS114_WriteReg(ads114s08_t *dev, uint8_t reg, uint8_t data);
static void ADS114_SendCommand(ads114s08_t *dev, uint8_t cmd);

/**
 * @brief Write a single byte to a register
 * @param dev Pointer to the ADS114S08 device structure    
 * @param reg Register address (0-31)
 * @param data Data byte to write
 */
static void ADS114_WriteReg(ads114s08_t *dev, uint8_t reg, uint8_t data) {
    SPI_HandleTypeDef *hspi = dev->hspi;
    uint8_t tx[3];
    
    tx[0] = ADS_CMD_WREG | (reg & 0x1F); // Command byte 1: 010r rrrr
    tx[1] = 0x00;                        // Command byte 2: Number of bytes to write minus 1
    tx[2] = data;                        // Data byte
    
    stmSetGpio(*dev->csPin, 0);   // CS must stay low for entire sequence
    HAL_SPI_Transmit(hspi, tx, 3, SPI_TIMEOUT);
    stmSetGpio(*dev->csPin, 1);
}

static void ADS114_SendCommand(ads114s08_t *dev, uint8_t cmd) {
    stmSetGpio(*dev->csPin, 0);   // CS must stay low for entire sequence
    HAL_SPI_Transmit(dev->hspi, &cmd, 1, SPI_TIMEOUT);
    stmSetGpio(*dev->csPin, 1);
}

/***************************************************************************************************
** PUBLIC FUNCTIONS
***************************************************************************************************/

/**
 * @brief Initialize the ADS114S08
 * @note Configures the device for single-shot mode 400 SPS with internal clock, gain of 1, and external reference.
 * @param dev Pointer to the ADS114S08 device structure
 * @param hspi Pointer to the SPI handle
 * @param csPin Pointer to the chip select GPIO pin
 * @param resetPin Pointer to the reset GPIO pin
 * @param drdyPin Pointer to the data ready GPIO pin
 * @param startSyncPin Pointer to the start/sync GPIO pin (not used in current implementation, but defined for completeness)
  */
void ADS114_Init(ads114s08_t *dev, SPI_HandleTypeDef *hspi, StmGpio *csPin, StmGpio *resetPin, StmGpio *drdyPin, StmGpio *startSyncPin) {
    // Store device configuration
    dev->hspi = hspi;
    dev->csPin = csPin;
    dev->resetPin = resetPin;
    dev->drdyPin = drdyPin;
    dev->startSyncPin = startSyncPin;

    // Hardware Reset: Take RESET pin low for min 4 tCLK, then high
    stmSetGpio(*dev->resetPin, 0);
    HAL_Delay(2); 
    stmSetGpio(*dev->resetPin, 1);
    
    // Wait for POR and initialization (min 2.2 ms)
    HAL_Delay(5);
    
    // Configure Reference (05h): 
    // REFSEL = 00 (REFP0/REFN0 default for external REF5025)
    // REFCON = 00 (Internal reference off)
    ADS114_WriteReg(dev, ADS_REG_REF, 0x00);
    
    // Configure PGA (03h):
    // PGA_EN = 01 (PGA enabled for all channels, necesarry for DUT_V_OUT voltage divider)
    // GAIN = 000 (Gain = 1)
    ADS114_WriteReg(dev, ADS_REG_PGA, 0b00001000); 
    
    // Configure Data Rate & Mode (04h):
    // CLK = 0 (Internal clock)
    // MODE = 1 (Single-shot mode)
    // DR = 1001 (400 SPS, 2.7 ms for single-shot conversion)
    // Note: In single-shot mode, the data rate setting determines the conversion time and thus the minimum delay between START command and reading data.
    // (with DR = 0100 we would get 20 SPS, 56.6 ms for single-shot conversion)
    ADS114_WriteReg(dev, ADS_REG_DATARATE, 0b00111001); 
    
    // START/SYNC will be held low, not actually needed
    stmSetGpio(*dev->startSyncPin, 0);
}

/**
 * @brief Read a single channel in single-shot mode. Blocks until conversion is ready.
 * @param dev Pointer to the ADS114S08 device structure
 * @param positive_ain Positive input channel
 * @return 16-bit ADC value
 */
uint16_t ADS114_ReadChannelSingleShot(ads114s08_t *dev, uint8_t positive_ain) {
    SPI_HandleTypeDef *hspi = dev->hspi;
    // Set the Input Multiplexer (02h)
    // MUXP = positive_ain, MUXN = 1100 (AINCOM)
    uint8_t mux_val = (positive_ain << 4) | ADS_MUX_AINCOM;
    ADS114_WriteReg(dev, ADS_REG_INPMUX, mux_val);
    
    // Trigger Conversion
    // We can *either* pulse the START_SYNC pin OR send START command.
    // We'll send the START command. The START_SYNC pin will be held low in initialization and not used.
    ADS114_SendCommand(dev, ADS_CMD_START);
    
    // Wait for conversion to complete (DRDY goes low)
    // 100 ms timeout to prevent infinite loop
    // NOTE: If single-shot conversion time is adjusted up, this timeout may need to be increased
    uint32_t timeout = HAL_GetTick() + 100;
    while (stmGetGpio(*dev->drdyPin) && HAL_GetTick() < timeout) { /* Wait */ }
    
    // Read Data 
    uint8_t tx[3] = {ADS_CMD_RDATA, 0x00, 0x00};
    uint8_t rx[3] = {0};
    
    stmSetGpio(*dev->csPin, 0);   // CS must stay low for entire sequence
    HAL_SPI_TransmitReceive(hspi, tx, rx, 3, SPI_TIMEOUT);
    stmSetGpio(*dev->csPin, 1);
    
    // Combine the 16 bits of data (Data comes MSB first after command)
    uint16_t result = (rx[1] << 8) | rx[2];
    
    return result;
}
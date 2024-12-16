/*!
** @brief Generates various CRCs
**
** @author Luke W
** @date   16/12/2024
*/

#include "crc.h"

/***************************************************************************************************
** PRIVATE OBJECTS
***************************************************************************************************/

uint8_t crc4_init = 0x00U;
uint8_t crc4_poly = 0x00U;
uint8_t crc8_init = 0x00U;
uint8_t crc8_poly = 0x00U;

/***************************************************************************************************
** PUBLIC FUNCTION DEFINITIONS
***************************************************************************************************/

/*!
** @brief Initialises the CRC4 init and polynomial variabls
*/
void initCrc4(uint8_t init, uint8_t poly) {
    crc4_init = init;
    crc4_poly = poly;
}

/*!
** @brief Initialises the CRC8 init and polynomial variabls
*/
void initCrc8(uint8_t init, uint8_t poly) {
    crc8_init = init;
    crc8_poly = poly;
}

/*!
** @brief Calculates CRC8
**
** Taken from: https://stackoverflow.com/questions/51752284/how-to-calculate-crc8-in-c
*/
uint8_t crc8Calculate(uint8_t *data, size_t len)
{
    uint8_t crc = crc8_init;

    for (int i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if ((crc & 0x80) != 0)
                crc = (uint8_t)((crc << 1) ^ crc8_poly);
            else
                crc <<= 1;
        }
    }
    return crc;
}

/*!
** @brief Calculates CRC4
*/
uint8_t crc4Calculate(uint8_t *data, size_t len)
{
    /* CRC polynomial starts in the lowest 4 bits */
    uint8_t crc  = crc4_init & 0xF;
    uint8_t poly = crc4_poly & 0xF;

    for (int i = 0; i < len; i++) {
        for(int j = 1; j >= 0; j--) {
            crc ^= (data[i] >> 4U * j) & 0xFU;

            for (int k = 0; k < 4; k++) {
                if ((crc & 0x8) != 0)
                    crc = (uint8_t)((crc << 1) ^ poly);
                else
                    crc <<= 1;
            }
        }
    }

    return crc & 0xFU;
}

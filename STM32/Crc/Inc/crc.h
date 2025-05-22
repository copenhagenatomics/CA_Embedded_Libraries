/*!
** @brief Generates various CRCs
**
** @author Luke W
** @date   16/12/2024
*/

#include <stdint.h>
#include <stddef.h>

#ifndef CRC_H_
#define CRC_H_

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************************************
** PUBLIC FUNCTION DECLARATIONS
***************************************************************************************************/

void initCrc4(uint8_t init, uint8_t poly);
void initCrc8(uint8_t init, uint8_t poly);
uint8_t crc8Calculate(uint8_t *data, size_t len);
uint8_t crc4Calculate(uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* CRC_H_ */

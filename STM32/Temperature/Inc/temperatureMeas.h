/*!
 * @file   temperatureMeas.h
 * @brief  Header file of temperatureMeas.c
 * @date   17/11/2026
 * @author Timothé Dodin
 */

#ifndef INC_TEMPERATUREMEAS_H_
#define INC_TEMPERATUREMEAS_H_

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

// Value returned when the conversion is not possible
#define TEMPERATURE_ERROR 10000.0f

// Thermocouple types come first, followed by the RTDs
enum sensorType_e {
    TYPE_B = 0,
    TYPE_E,
    TYPE_J,
    TYPE_K,
    TYPE_N,
    TYPE_R,
    TYPE_S,
    TYPE_T,
    PT_100,
    PT_1000,
    NO_OF_TYPES
};

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************************************
** PUBLIC FUNCTION DECLARATIONS
***************************************************************************************************/

float voltToDeg(float millivolt, int sensorType, float coldJunctionTemp);
float degToVolt(float temperature, int sensorType, float coldJunctionTemp);
float resToDeg(float ohm, int sensorType, float leadResistance);
float degToRes(float temperature, int sensorType);

#ifdef __cplusplus
}
#endif

#endif /* INC_TEMPERATUREMEAS_H_ */

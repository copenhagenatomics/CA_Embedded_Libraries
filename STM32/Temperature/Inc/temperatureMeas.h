/*!
 * @file   temperatureMeas.h
 * @brief  Header file of temperatureMeas.c
 * @date   17/11/2026
 * @author Timothé Dodin
 */

#ifndef INC_CALIBRATION_H_
#define INC_CALIBRATION_H_

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

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

/***************************************************************************************************
** PUBLIC FUNCTION DECLARATIONS
***************************************************************************************************/

float voltToDeg(float millivolt, int sensorType, float coldJunctionTemp);

#endif /* INC_CALIBRATION_H_ */

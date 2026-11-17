/*!
 * @file   temperatureMeas.c
 * @brief  Temperature conversion for different sensors
 * @date   17/11/2026
 * @author Timothé Dodin
 */

#include <math.h>
#include <stdint.h>

#include "temperatureMeas.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

#define TEMPERATURE_ERROR 10000.0f

/***************************************************************************************************
** PRIVATE FUNCTION DECLARATIONS
***************************************************************************************************/

/***************************************************************************************************
** PRIVATE OBJECTS
***************************************************************************************************/

/***************************************************************************************************
** PRIVATE FUNCTION DEFINITIONS
***************************************************************************************************/

/*!
 * @brief Evaluates a polynomial efficiently
 * @param c Array of coefficients
 * @param n Number of coefficients
 * @param x Value where the polynomial is evaluated
 */
static float polyEval(const float* c, uint32_t n, float x) {
    float result = c[n - 1];
    for (int32_t i = n - 2; i >= 0; i--) {
        result = result * x + c[i];
    }
    return result;
}

/* NIST ITS-90 Type K forward polynomial: °C -> mV */
static float tempToVolt(float t) {
    static const float C_NEG[11] = {0.0000000000e0f,    3.9450128025e-2f,   2.3622373598e-5f,
                                    -3.2858906784e-7f,  -4.9904828777e-9f,  -6.7509059173e-11f,
                                    -5.7410327428e-13f, -3.1088872894e-15f, -1.0451609365e-17f,
                                    -1.9889266878e-20f, -1.6322697486e-23f};
    static const float C_POS[10] = {-1.7600413686e-2f, 3.8921204975e-2f,   1.8558770032e-5f,
                                    -9.9457592874e-8f, 3.1840945719e-10f,  -5.6072844889e-13f,
                                    5.6075059059e-16f, -3.2020720003e-19f, 9.7151147152e-23f,
                                    -1.2104721275e-26f};
    static const float A[3]      = {1.185976e-1, -1.183432e-4, 1.269686e2};

    float millivolt;
    if (t < 0.0f) {
        millivolt = polyEval(C_NEG, 11, t);
    }
    else {
        float dt  = t - A[2];
        millivolt = polyEval(C_POS, 10, t) + A[0] * expf(A[1] * dt * dt);
    }
    return millivolt;
}

static float voltToTempTypeK(float millivolt, float coldJunctionTemp) {
    static const float D_NEG[10] = {0.0000000e0f,   2.5173462e1f,   -1.1662878e0f,  -1.0833638e0f,
                                    -8.9773540e-1f, -3.7342377e-1f, -8.6632643e-2f, -1.0450598e-2f,
                                    -5.1920577e-4f, 0.0000000e0f};
    static const float D_0_500[10] = {0.0000000e0f,  2.5083550e1f,   7.8601060e-2f, -2.5031310e-1f,
                                      8.3152700e-2f, -1.2280340e-2f, 9.8040360e-4f, -4.4130300e-5f,
                                      1.0577340e-6f, -1.0527550e-8f};
    static const float D_500_1372[10] = {
        -1.3180580e2f, 4.8302220e01f,  -1.6460310e0f, 5.4647310e-2f, -9.6507150e-4f,
        8.8021930e-6f, -3.1108100e-8f, 0.0000000e0f,  0.0000000e0f,  0.0000000e0f};

    static float mVComp = millivolt + tempToVolt(coldJunctionTemp);

    if ((mVComp < -5.891f) || (mVComp > 54.886f)) {
        return TEMPERATURE_ERROR;
    }

    float* array = (mVComp < 0.0f) ? D_NEG : ((mVComp < 20.644f) ? D_0_500 : D_500_1372);

    return polyEval(array, 10, mVComp);
}

static float resToTempPt100(millivolt) {
    static const float R_0 = 100.0f;
    
}

/***************************************************************************************************
** PUBLIC FUNCTION DEFINITIONS
***************************************************************************************************/

float voltToDeg(float millivolt, int sensorType, float coldJunctionTemp) {
    switch (sensorType) {
        case TYPE_K:
            return voltToTempTypeK(millivolt, coldJunctionTemp);
        case PT_100:
            return voltToTempPt100(millivolt);
        default:
            break;
    }
}

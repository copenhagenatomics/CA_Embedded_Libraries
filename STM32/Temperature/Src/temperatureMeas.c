/*!
 * @file   temperatureMeas.c
 * @brief  Temperature conversion for different sensors
 * @date   17/11/2026
 * @author Timothé Dodin
 */

#include <math.h>
#include <stddef.h>
#include <stdint.h>

#include "temperatureMeas.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

// Callendar-Van Dusen coefficients - IEC 60751
#define RTD_A 3.9083e-3f
#define RTD_B -5.775e-7f
#define RTD_C -4.183e-12f

#define RTD_T_MIN -200.0f
#define RTD_T_MAX 850.0f

// Voltage accepted beyond the limits of a sensor, to absorb floating point rounding [mV]
#define TOLERANCE_MV 1e-3f

#define RTD_NEWTON_ROUNDS 5

/***************************************************************************************************
** PRIVATE TYPES
***************************************************************************************************/

// One polynomial piece of a thermocouple reference function, valid up to tMax
typedef struct {
    float tMax;         // [degC] - Upper temperature limit of the piece
    uint8_t n;          // Number of coefficients
    const float* c;     // [mV/degC^i] - Coefficients ,lowest order first
    const float* expo;  // NULL, or {a0, a1, a2} of the term a0 * exp(a1 * (T - a2)^2)
} tcPiece_t;

// One polynomial piece of a thermocouple inverse function, valid up to vMax
typedef struct {
    float vMax;      // [mv] - Upper voltage limit of the piece [mV]
    uint8_t n;       // Number of coefficients
    const float* c;  // [degC/mV^i] - Coefficients, lowest order first
} tcInvPiece_t;

typedef struct {
    const tcPiece_t* pieces;  // Reference function pieces, by increasing temperature
    uint8_t nPieces;          // Number of funstion pieces
    const tcInvPiece_t* inv;  // Inverse function pieces, by increasing voltage
    uint8_t nInv;             // Number of inverse function pieces
    float tMin;               // [degC] - Lowest valid hot junction temperature
    float tMax;               // [degC] - Highest valid hot junction temperature
    float tCjMin;             // [degC] - Lowest valid cold junction temperature
} tcDef_t;

/***************************************************************************************************
** PRIVATE OBJECTS
***************************************************************************************************/

// NIST ITS-90 thermocouple functions
// https://its90.nist.gov/RefFunctions

static const float TC_B_0[7]          = {0.000000000e+00f,  -2.465081925e-04f, 5.904042155e-06f,
                                         -1.325793142e-09f, 1.566829240e-12f,  -1.694452883e-15f,
                                         6.299034648e-19f};
static const float TC_B_1[9]          = {-3.893816948e+00f, 2.857174724e-02f,  -8.488510503e-05f,
                                         1.578527957e-07f,  -1.683534423e-10f, 1.110979391e-13f,
                                         -4.451542942e-17f, 9.897564188e-21f,  -9.379133136e-25f};
static const tcPiece_t TC_B_PIECES[2] = {
    {630.615f, 7, TC_B_0, NULL},
    {1820.0f, 9, TC_B_1, NULL},
};
static const float TC_B_INV_0[9]      = {9.842332458e+01f,  6.997150269e+02f,  -8.476530762e+02f,
                                         1.005264587e+03f,  -8.334595337e+02f, 4.550854187e+02f,
                                         -1.552303467e+02f, 2.988674927e+01f,  -2.474286079e+00f};
static const float TC_B_INV_1[9]      = {2.131507111e+02f,  2.851050415e+02f,  -5.274288177e+01f,
                                         9.916080475e+00f,  -1.296530843e+00f, 1.119587049e-01f,
                                         -6.062519271e-03f, 1.866169914e-04f,  -2.487858183e-06f};
static const tcInvPiece_t TC_B_INV[2] = {
    {2.431f, 9, TC_B_INV_0},
    {13.82f, 9, TC_B_INV_1},
};

static const float TC_E_0[14] = {
    0.000000000e+00f,  5.866551027e-02f,  4.541097587e-05f,  -7.799804962e-07f, -2.580016023e-08f,
    -5.945258175e-10f, -9.321405627e-12f, -1.028760546e-13f, -8.037012298e-16f, -4.397949745e-18f,
    -1.641477564e-20f, -3.967361984e-23f, -5.582732881e-26f, -3.465784060e-29f};
static const float TC_E_1[11]         = {0.000000000e+00f,  5.866551027e-02f,  4.503227683e-05f,
                                         2.890840634e-08f,  -3.305689611e-10f, 6.502440188e-13f,
                                         -1.919749485e-16f, -1.253660047e-18f, 2.148921729e-21f,
                                         -1.438804194e-24f, 3.596089892e-28f};
static const tcPiece_t TC_E_PIECES[2] = {
    {0.0f, 14, TC_E_0, NULL},
    {1000.0f, 11, TC_E_1, NULL},
};
static const float TC_E_INV_0[9]  = {0.000000000e+00f,  1.697728729e+01f,  -4.351496994e-01f,
                                     -1.585969627e-01f, -9.250286967e-02f, -2.608431317e-02f,
                                     -4.136019852e-03f, -3.403403098e-04f, -1.156488997e-05f};
static const float TC_E_INV_1[10] = {
    0.000000000e+00f,  1.705703545e+01f, -2.330175936e-01f, 6.543558557e-03f, -7.356274727e-05f,
    -1.789600105e-06f, 8.403616647e-08f, -1.373587910e-09f, 1.062982341e-11f, -3.244708782e-14f};
static const tcInvPiece_t TC_E_INV[2] = {
    {0.0f, 9, TC_E_INV_0},
    {76.373f, 10, TC_E_INV_1},
};

static const float TC_J_0[9]          = {0.000000000e+00f,  5.038118735e-02f,  3.047583778e-05f,
                                         -8.568106580e-08f, 1.322819493e-10f,  -1.705295790e-13f,
                                         2.094809093e-16f,  -1.253839597e-19f, 1.563172533e-23f};
static const float TC_J_1[6]          = {2.964562683e+02f,  -1.497612834e+00f, 3.178710351e-03f,
                                         -3.184768730e-06f, 1.572081909e-09f,  -3.069137013e-13f};
static const tcPiece_t TC_J_PIECES[2] = {
    {760.0f, 9, TC_J_0, NULL},
    {1200.0f, 6, TC_J_1, NULL},
};
static const float TC_J_INV_0[9]      = {0.000000000e+00f,  1.952675819e+01f,  -1.228618383e+00f,
                                         -1.075217962e+00f, -5.908693075e-01f, -1.725671291e-01f,
                                         -2.813151106e-02f, -2.396336989e-03f, -8.382331725e-05f};
static const float TC_J_INV_1[8]      = {0.000000000e+00f,  1.978425026e+01f,  -2.001204044e-01f,
                                         1.036969014e-02f,  -2.549687051e-04f, 3.585153081e-06f,
                                         -5.344285015e-08f, 5.099889955e-10f};
static const float TC_J_INV_2[6]      = {-3.113581787e+03f, 3.005436707e+02f,  -9.947731972e+00f,
                                         1.702766269e-01f,  -1.430334640e-03f, 4.738860753e-06f};
static const tcInvPiece_t TC_J_INV[3] = {
    {0.0f, 9, TC_J_INV_0},
    {42.919f, 8, TC_J_INV_1},
    {69.553f, 6, TC_J_INV_2},
};

static const float TC_K_0[11] = {0.000000000e+00f,  3.945012763e-02f,  2.362237319e-05f,
                                 -3.285890671e-07f, -4.990482694e-09f, -6.750906084e-11f,
                                 -5.741032649e-13f, -3.108887261e-15f, -1.045160902e-17f,
                                 -1.988926702e-20f, -1.632269807e-23f};
static const float TC_K_1[10] = {
    -1.760041341e-02f, 3.892120346e-02f, 1.855877053e-05f,  -9.945759416e-08f, 3.184094655e-10f,
    -5.607284385e-13f, 5.607505812e-16f, -3.202071986e-19f, 9.715114873e-23f,  -1.210472161e-26f};
static const float TC_K_1_EXPO[3]     = {1.185975969e-01f, -1.183432032e-04f, 1.269685974e+02f};
static const tcPiece_t TC_K_PIECES[2] = {
    {0.0f, 11, TC_K_0, NULL},
    {1372.0f, 10, TC_K_1, TC_K_1_EXPO},
};
static const float TC_K_INV_0[9]  = {0.000000000e+00f,  2.517346191e+01f,  -1.166287780e+00f,
                                     -1.083363771e+00f, -8.977354169e-01f, -3.734237552e-01f,
                                     -8.663264662e-02f, -1.045059785e-02f, -5.192057579e-04f};
static const float TC_K_INV_1[10] = {
    0.000000000e+00f,  2.508354950e+01f, 7.860106230e-02f,  -2.503131032e-01f, 8.315269649e-02f,
    -1.228034031e-02f, 9.804036235e-04f, -4.413029819e-05f, 1.057734039e-06f,  -1.052755039e-08f};
static const float TC_K_INV_2[7]      = {-1.318058014e+02f, 4.830221939e+01f,  -1.646031022e+00f,
                                         5.464731157e-02f,  -9.650714928e-04f, 8.802193406e-06f,
                                         -3.110810098e-08f};
static const tcInvPiece_t TC_K_INV[3] = {
    {0.0f, 9, TC_K_INV_0},
    {20.644f, 10, TC_K_INV_1},
    {54.886f, 7, TC_K_INV_2},
};

static const float TC_N_0[9]          = {0.000000000e+00f,  2.615910582e-02f,  1.095748394e-05f,
                                         -9.384110911e-08f, -4.641204088e-11f, -2.630335836e-12f,
                                         -2.265343878e-14f, -7.608930052e-17f, -9.341966519e-20f};
static const float TC_N_1[11]         = {0.000000000e+00f,  2.592939511e-02f,  1.571014218e-05f,
                                         4.382562580e-08f,  -2.526117093e-10f, 6.431182084e-13f,
                                         -1.006347139e-15f, 9.974533635e-19f,  -6.086324669e-22f,
                                         2.084922888e-25f,  -3.068219758e-29f};
static const tcPiece_t TC_N_PIECES[2] = {
    {0.0f, 9, TC_N_0, NULL},
    {1300.0f, 11, TC_N_1, NULL},
};
static const float TC_N_INV_0[10] = {
    0.000000000e+00f, 3.843684769e+01f, 1.101048470e+00f, 5.222931385e+00f, 7.206052303e+00f,
    5.848858833e+00f, 2.775491714e+00f, 7.707516551e-01f, 1.158266515e-01f, 7.313886657e-03f};
static const float TC_N_INV_1[8]      = {0.000000000e+00f, 3.868960190e+01f,  -1.082669973e+00f,
                                         4.702049866e-02f, -2.121689931e-06f, -1.172720004e-04f,
                                         5.392800176e-06f, -7.981559946e-08f};
static const float TC_N_INV_2[6]      = {1.972484970e+01f, 3.300942993e+01f,  -3.915159106e-01f,
                                         9.855391458e-03f, -1.274371025e-04f, 7.767022225e-07f};
static const tcInvPiece_t TC_N_INV[3] = {
    {0.0f, 10, TC_N_INV_0},
    {20.613f, 8, TC_N_INV_1},
    {47.513f, 6, TC_N_INV_2},
};

static const float TC_R_0[10] = {
    0.000000000e+00f,  5.289617460e-03f, 1.391665865e-05f,  -2.388556908e-08f, 3.569159898e-11f,
    -4.623476827e-14f, 5.007774259e-17f, -3.731058899e-20f, 1.577164875e-23f,  -2.810386308e-27f};
static const float TC_R_1[6]          = {2.951579332e+00f,  -2.520612441e-03f, 1.595644972e-05f,
                                         -7.640859501e-09f, 2.053052823e-12f,  -2.933596704e-16f};
static const float TC_R_2[5]          = {1.522321167e+02f, -2.688198984e-01f, 1.712802768e-04f,
                                         -3.458957210e-08f, -9.346339417e-15f};
static const tcPiece_t TC_R_PIECES[3] = {
    {1064.18f, 10, TC_R_0, NULL},
    {1664.5f, 6, TC_R_1, NULL},
    {1768.1f, 5, TC_R_2, NULL},
};
static const float TC_R_INV_0[11] = {0.000000000e+00f,  1.889138031e+02f,  -9.383528900e+01f,
                                     1.306861877e+02f,  -2.270357971e+02f, 3.514566040e+02f,
                                     -3.895390015e+02f, 2.823947144e+02f,  -1.260728073e+02f,
                                     3.135361099e+01f,  -3.318776846e+00f};
static const float TC_R_INV_1[10] = {
    1.334584522e+01f, 1.472644501e+02f,  -1.844024849e+01f, 4.031129837e+00f,  -6.249428391e-01f,
    6.468412280e-02f, -4.458750598e-03f, 1.994710183e-04f,  -5.313401743e-06f, 6.481976556e-08f};
static const float TC_R_INV_2[6]      = {-8.199599457e+01f, 1.553962097e+02f,  -8.342197418e+00f,
                                         4.279433489e-01f,  -1.191577874e-02f, 1.492290030e-04f};
static const float TC_R_INV_3[5]      = {3.406177734e+04f, -7.023729004e+03f, 5.582904053e+02f,
                                         -1.952394676e+01f, 2.560740113e-01f};
static const tcInvPiece_t TC_R_INV[4] = {
    {1.923f, 11, TC_R_INV_0},
    {11.361f, 10, TC_R_INV_1},
    {19.739f, 6, TC_R_INV_2},
    {21.103f, 5, TC_R_INV_3},
};

static const float TC_S_0[9]          = {0.000000000e+00f,  5.403133109e-03f,  1.259342935e-05f,
                                         -2.324779658e-08f, 3.220288272e-11f,  -3.314652020e-14f,
                                         2.557442442e-17f,  -1.250688685e-20f, 2.714431721e-24f};
static const float TC_S_1[5]          = {1.329004407e+00f, 3.345093224e-03f, 6.548051715e-06f,
                                         -1.648562620e-09f, 1.299896063e-14f};
static const float TC_S_2[5]          = {1.466282349e+02f, -2.584305108e-01f, 1.636935776e-04f,
                                         -3.304390361e-08f, -9.432237028e-15f};
static const tcPiece_t TC_S_PIECES[3] = {
    {1064.18f, 9, TC_S_0, NULL},
    {1664.5f, 5, TC_S_1, NULL},
    {1768.1f, 5, TC_S_2, NULL},
};
static const float TC_S_INV_0[10] = {
    0.000000000e+00f, 1.849494629e+02f,  -8.005040741e+01f, 1.022374268e+02f,  -1.522485962e+02f,
    1.888213501e+02f, -1.590859375e+02f, 8.230278778e+01f,  -2.341819382e+01f, 2.797862530e+00f};
static const float TC_S_INV_1[10] = {
    1.291507149e+01f, 1.466298828e+02f,  -1.534713364e+01f, 3.145946026e+00f,  -4.163257778e-01f,
    3.187963739e-02f, -1.291637542e-03f, 2.183475044e-05f,  -1.447379532e-07f, 8.211272551e-09f};
static const float TC_S_INV_2[6]      = {-8.087801361e+01f, 1.621573029e+02f,  -8.536869049e+00f,
                                         4.719687104e-01f,  -1.441693678e-02f, 2.081618877e-04f};
static const float TC_S_INV_3[5]      = {5.333875000e+04f, -1.235892285e+04f, 1.092657593e+03f,
                                         -4.265693665e+01f, 6.247205138e-01f};
static const tcInvPiece_t TC_S_INV[4] = {
    {1.874f, 10, TC_S_INV_0},
    {10.332f, 10, TC_S_INV_1},
    {17.536f, 6, TC_S_INV_2},
    {18.693f, 5, TC_S_INV_3},
};

static const float TC_T_0[15] = {
    0.000000000e+00f, 3.874810785e-02f, 4.419443576e-05f, 1.184432321e-07f, 2.003297439e-08f,
    9.013801927e-10f, 2.265115699e-11f, 3.607115420e-13f, 3.849394168e-15f, 2.821352266e-17f,
    1.425159429e-19f, 4.876866353e-22f, 1.079553881e-24f, 1.394502702e-27f, 7.979515203e-31f};
static const float TC_T_1[9]          = {0.000000000e+00f,  3.874810785e-02f,  3.329222818e-05f,
                                         2.061824347e-07f,  -2.188225601e-09f, 1.099688136e-11f,
                                         -3.081575996e-14f, 4.547913454e-17f,  -2.751290282e-20f};
static const tcPiece_t TC_T_PIECES[2] = {
    {0.0f, 15, TC_T_0, NULL},
    {400.0f, 9, TC_T_1, NULL},
};
static const float TC_T_INV_0[8]      = {0.000000000e+00f, 2.594919205e+01f, -2.131696641e-01f,
                                         7.901869416e-01f, 4.252777696e-01f, 1.330447346e-01f,
                                         2.024144679e-02f, 1.266817097e-03f};
static const float TC_T_INV_1[7]      = {0.000000000e+00f, 2.592799950e+01f,  -7.602961063e-01f,
                                         4.637790844e-02f, -2.165393904e-03f, 6.048144132e-05f,
                                         -7.293422186e-07f};
static const tcInvPiece_t TC_T_INV[2] = {
    {0.0f, 8, TC_T_INV_0},
    {20.872f, 7, TC_T_INV_1},
};

static const tcDef_t TC_TYPES[TYPE_T + 1] = {
    [TYPE_B] = {TC_B_PIECES, 2, TC_B_INV, 2, 250.0f, 1820.0f, 0.0f},
    [TYPE_E] = {TC_E_PIECES, 2, TC_E_INV, 2, -200.0f, 1000.0f, -200.0f},
    [TYPE_J] = {TC_J_PIECES, 2, TC_J_INV, 3, -210.0f, 1200.0f, -210.0f},
    [TYPE_K] = {TC_K_PIECES, 2, TC_K_INV, 3, -200.0f, 1372.0f, -200.0f},
    [TYPE_N] = {TC_N_PIECES, 2, TC_N_INV, 3, -200.0f, 1300.0f, -200.0f},
    [TYPE_R] = {TC_R_PIECES, 3, TC_R_INV, 4, -50.0f, 1768.1f, -50.0f},
    [TYPE_S] = {TC_S_PIECES, 3, TC_S_INV, 4, -50.0f, 1768.1f, -50.0f},
    [TYPE_T] = {TC_T_PIECES, 2, TC_T_INV, 2, -200.0f, 400.0f, -200.0f},
};

/***************************************************************************************************
** PRIVATE FUNCTION DEFINITIONS
***************************************************************************************************/

/*!
 * @brief Evaluates a polynomial efficiently
 * @param c Array of coefficients, lowest order first
 * @param n Number of coefficients
 * @param x Value where the polynomial is evaluated
 * @return Value of the polynomial at x
 */
static float polyEval(const float* c, uint32_t n, float x) {
    float result = c[n - 1];
    for (int32_t i = (int32_t)n - 2; i >= 0; i--) {
        result = result * x + c[i];
    }
    return result;
}

/*!
 * @brief Thermocouple reference function: voltage of a thermocouple versus a 0 degC junction
 * @param def  Thermocouple definition
 * @param t    Temperature [degC]
 * @return Voltage [mV]
 */
static float tcForward(const tcDef_t* def, float t) {
    const tcPiece_t* piece = &def->pieces[0];
    for (uint8_t i = 0; i < def->nPieces; i++) {
        piece = &def->pieces[i];
        if (t <= piece->tMax) {
            break;
        }
    }

    float millivolt = polyEval(piece->c, piece->n, t);
    if (piece->expo != NULL) {
        float dt = t - piece->expo[2];
        millivolt += piece->expo[0] * expf(piece->expo[1] * dt * dt);
    }
    return millivolt;
}

/*!
 * @brief Thermocouple inverse function: temperature from the voltage versus a 0 degC junction
 * @param def        Thermocouple definition
 * @param millivolt  Voltage [mV]
 * @return Temperature [degC]
 */
static float tcInverse(const tcDef_t* def, float millivolt) {
    const tcInvPiece_t* piece = &def->inv[0];
    for (uint8_t i = 0; i < def->nInv; i++) {
        piece = &def->inv[i];
        if (millivolt <= piece->vMax) {
            break;
        }
    }
    return polyEval(piece->c, piece->n, millivolt);
}

/*!
 * @brief Gets the definition of a thermocouple type
 * @param sensorType  Sensor type, as sensorType_e
 * @return Definition of the thermocouple, or NULL if sensorType is not a thermocouple
 */
static const tcDef_t* getThermocouple(int sensorType) {
    if ((sensorType < TYPE_B) || (sensorType > TYPE_T)) {
        return NULL;
    }
    return &TC_TYPES[sensorType];
}

/*!
 * @brief Gets the resistance at 0 degC of an RTD
 * @param sensorType  Sensor type, as sensorType_e
 * @return Resistance at 0 degC [ohm], or 0 if sensorType is not an RTD
 */
static float getRtdR0(int sensorType) {
    switch (sensorType) {
        case PT_100:
            return 100.0f;
        case PT_1000:
            return 1000.0f;
        default:
            return 0.0f;
    }
}

/*!
 * @brief Callendar-Van Dusen equation, giving the ratio R(t) / R0
 * @details Coefficients A, B and C are those of IEC 60751 (alpha = 0.00385)
 * @see https://en.wikipedia.org/wiki/Callendar%E2%80%93Van_Dusen_equation
 * @param t      Temperature [degC]
 * @param slope  If not NULL, receives the derivative of the ratio [1/degC]
 * @return Ratio of the resistance at t to the resistance at 0 degC
 */
static float rtdRatio(float t, float* slope) {
    float ratio = 1.0f + t * (RTD_A + RTD_B * t);
    float d     = RTD_A + 2.0f * RTD_B * t;
    if (t < 0.0f) {
        ratio += RTD_C * (t - 100.0f) * t * t * t;
        d += RTD_C * (4.0f * t * t * t - 300.0f * t * t);
    }
    if (slope != NULL) {
        *slope = d;
    }
    return ratio;
}

/*!
 * @brief Inverse of the Callendar-Van Dusen equation
 * @details Closed form solution of the quadratic (A and B terms), which is exact above 0 degC.
 * Below 0 degC it is the starting point of Newton iterations on the full equation (see rtdRatio())
 * @see https://en.wikipedia.org/wiki/Callendar%E2%80%93Van_Dusen_equation
 * @param ratio  Ratio R(t) / R0, which must be inside the valid range of the sensor
 * @return Temperature [degC]
 */
static float rtdTemperature(float ratio) {
    /* Closed form of the quadratic, in a form that does not lose precision around 0 degC */
    float x = ratio - 1.0f;
    float t = 2.0f * x / (RTD_A + sqrtf(RTD_A * RTD_A + 4.0f * RTD_B * x));
    if (t < 0.0f) {
        /* The cubic term cannot be neglected below 0 degC */
        for (int32_t i = 0; i < RTD_NEWTON_ROUNDS; i++) {
            float slope;
            float error = rtdRatio(t, &slope) - ratio;
            t -= error / slope;
        }
    }
    return t;
}

/***************************************************************************************************
** PUBLIC FUNCTION DEFINITIONS
***************************************************************************************************/

/*!
 * @brief Converts a thermocouple voltage into a temperature, with cold junction compensation
 * @param millivolt         Voltage measured across the thermocouple [mV]
 * @param sensorType        Thermocouple type (TYPE_B to TYPE_T)
 * @param coldJunctionTemp  Temperature of the cold junction [degC] (0 = no compensation)
 * @return Temperature of the hot junction [degC], or TEMPERATURE_ERROR if the conversion is not
 *         possible (invalid input, sensor type, or temperature outside of the valid range)
 */
float voltToDeg(float millivolt, int sensorType, float coldJunctionTemp) {
    const tcDef_t* def = getThermocouple(sensorType);
    if ((def == NULL) || !isfinite(millivolt) || !isfinite(coldJunctionTemp) ||
        (coldJunctionTemp < def->tCjMin) || (coldJunctionTemp > def->tMax)) {
        return TEMPERATURE_ERROR;
    }

    /* Compensate in the voltage domain, as the reference functions are not linear */
    float mVComp = millivolt + tcForward(def, coldJunctionTemp);
    if ((mVComp < tcForward(def, def->tMin) - TOLERANCE_MV) ||
        (mVComp > tcForward(def, def->tMax) + TOLERANCE_MV)) {
        return TEMPERATURE_ERROR;
    }
    return tcInverse(def, mVComp);
}

/*!
 * @brief Converts a temperature into the voltage a thermocouple outputs (inverse of voltToDeg)
 * @param temperature       Temperature of the hot junction [degC]
 * @param sensorType        Thermocouple type (TYPE_B to TYPE_T)
 * @param coldJunctionTemp  Temperature of the cold junction [degC]
 * @return Voltage across the thermocouple [mV], or NaN if the conversion is not possible
 */
float degToVolt(float temperature, int sensorType, float coldJunctionTemp) {
    const tcDef_t* def = getThermocouple(sensorType);
    if ((def == NULL) || !isfinite(temperature) || !isfinite(coldJunctionTemp) ||
        (temperature < def->tMin) || (temperature > def->tMax) ||
        (coldJunctionTemp < def->tCjMin) || (coldJunctionTemp > def->tMax)) {
        return NAN;
    }
    return tcForward(def, temperature) - tcForward(def, coldJunctionTemp);
}

/*!
 * @brief Converts an RTD resistance into a temperature, with wire resistance compensation
 * @param ohm             Resistance measured [ohm]
 * @param sensorType      RTD type (PT_100 or PT_1000)
 * @param leadResistance  Resistance to compensate for [ohm]
 * @return Temperature [degC], or TEMPERATURE_ERROR if the conversion is not possible
 */
float resToDeg(float ohm, int sensorType, float leadResistance) {
    float r0 = getRtdR0(sensorType);
    if ((r0 == 0.0f) || !isfinite(ohm) || leadResistance < 0.0f || !isfinite(leadResistance)) {
        return TEMPERATURE_ERROR;
    }

    float resistance = ohm - leadResistance;
    if (resistance <= 0.0f) {
        return TEMPERATURE_ERROR;
    }

    float ratio = resistance / r0;
    if ((ratio < rtdRatio(RTD_T_MIN, NULL)) || (ratio > rtdRatio(RTD_T_MAX, NULL))) {
        return TEMPERATURE_ERROR;
    }
    return rtdTemperature(ratio);
}

/*!
 * @brief Converts a temperature into the resistance of an RTD (inverse of resToDeg, without wires)
 * @param temperature  Temperature [degC]
 * @param sensorType   RTD type (PT_100 or PT_1000)
 * @return Resistance [ohm], or NaN if the conversion is not possible
 */
float degToRes(float temperature, int sensorType) {
    float r0 = getRtdR0(sensorType);
    if ((r0 == 0.0f) || !isfinite(temperature) || (temperature < RTD_T_MIN) ||
        (temperature > RTD_T_MAX)) {
        return NAN;
    }
    return r0 * rtdRatio(temperature, NULL);
}

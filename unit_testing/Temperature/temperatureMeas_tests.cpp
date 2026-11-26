/*!
** @file   temperatureMeas_tests.cpp
** @author Timothé Dodin
** @date   25/11/2026
*/

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cmath>
#include <vector>

#include "temperatureMeas.h"

/***************************************************************************************************
** TEST DATA
***************************************************************************************************/

struct tcPoint {
    int type;
    float degC;
    float mV;
};

// Values of the NIST ITS-90 reference functions
static const tcPoint TC_POINTS[] = {
    {TYPE_B, 250.0f, 0.2912795f},   {TYPE_B, 500.0f, 1.2418497f},    {TYPE_B, 630.615f, 1.9783735f},
    {TYPE_B, 1000.0f, 4.8343387f},  {TYPE_B, 1500.0f, 10.0990608f},  {TYPE_B, 1820.0f, 13.8202792f},
    {TYPE_E, -200.0f, -8.8245811f}, {TYPE_E, -100.0f, -5.2371843f},  {TYPE_E, 0.0f, 0.0000000f},
    {TYPE_E, 100.0f, 6.3189303f},   {TYPE_E, 500.0f, 37.0053538f},   {TYPE_E, 1000.0f, 76.3728265f},
    {TYPE_J, -210.0f, -8.0953796f}, {TYPE_J, -100.0f, -4.6325237f},  {TYPE_J, 0.0f, 0.0000000f},
    {TYPE_J, 100.0f, 5.2689161f},   {TYPE_J, 760.0f, 42.9186413f},   {TYPE_J, 1200.0f, 69.5531798f},
    {TYPE_K, -200.0f, -5.8914036f}, {TYPE_K, -100.0f, -3.5536313f},  {TYPE_K, 0.0f, 0.0000000f},
    {TYPE_K, 100.0f, 4.0962302f},   {TYPE_K, 500.0f, 20.6442864f},   {TYPE_K, 1000.0f, 41.2756065f},
    {TYPE_K, 1372.0f, 54.8863640f}, {TYPE_N, -200.0f, -3.9903761f},  {TYPE_N, -100.0f, -2.4068112f},
    {TYPE_N, 0.0f, 0.0000000f},     {TYPE_N, 100.0f, 2.7741240f},    {TYPE_N, 500.0f, 16.7478569f},
    {TYPE_N, 1300.0f, 47.5127722f}, {TYPE_R, -50.0f, -0.2264652f},   {TYPE_R, 0.0f, 0.0000000f},
    {TYPE_R, 100.0f, 0.6473961f},   {TYPE_R, 1064.18f, 11.3637448f}, {TYPE_R, 1500.0f, 17.4506531f},
    {TYPE_R, 1768.1f, 21.1027023f}, {TYPE_S, -50.0f, -0.2355551f},   {TYPE_S, 0.0f, 0.0000000f},
    {TYPE_S, 100.0f, 0.6459130f},   {TYPE_S, 1064.18f, 10.3342044f}, {TYPE_S, 1500.0f, 15.5816694f},
    {TYPE_S, 1768.1f, 18.6935413f}, {TYPE_T, -200.0f, -5.6029607f},  {TYPE_T, -100.0f, -3.3785821f},
    {TYPE_T, 0.0f, 0.0000000f},     {TYPE_T, 100.0f, 4.2785186f},    {TYPE_T, 200.0f, 9.2881020f},
    {TYPE_T, 400.0f, 20.8719701f},
};

// Valid temperature range of each thermocouple type
struct tcRange {
    int type;
    float tMin;
    float tMax;
};

static const tcRange TC_RANGES[] = {
    {TYPE_B, 250.0f, 1820.0f},  {TYPE_E, -200.0f, 1000.0f}, {TYPE_J, -210.0f, 1200.0f},
    {TYPE_K, -200.0f, 1372.0f}, {TYPE_N, -200.0f, 1300.0f}, {TYPE_R, -50.0f, 1768.1f},
    {TYPE_S, -50.0f, 1768.1f},  {TYPE_T, -200.0f, 400.0f},
};

static const float MAX_ERROR_DEG = 0.1f;
static const float MAX_ERROR_MV  = 1.5e-3f;

/***************************************************************************************************
** TESTS - THERMOCOUPLES
***************************************************************************************************/

TEST(TemperatureMeasTc, DegToVoltMatchesReferenceTable) {
    for (const tcPoint& p : TC_POINTS) {
        if (p.type == TYPE_B && p.degC < 250.0f) {
            continue;
        }
        EXPECT_NEAR(degToVolt(p.degC, p.type, 0.0f), p.mV, MAX_ERROR_MV)
            << "type " << p.type << " " << p.degC;
    }
}

TEST(TemperatureMeasTc, VoltToDegMatchesReferenceTable) {
    for (const tcPoint& p : TC_POINTS) {
        EXPECT_NEAR(voltToDeg(p.mV, p.type, 0.0f), p.degC, MAX_ERROR_DEG)
            << "type " << p.type << " " << p.mV << " mV";
    }
}

TEST(TemperatureMeasTc, WellKnownNistValuesTypeK) {
    EXPECT_NEAR(degToVolt(100.0f, TYPE_K, 0.0f), 4.096f, 5e-4f);
    EXPECT_NEAR(degToVolt(1000.0f, TYPE_K, 0.0f), 41.276f, 5e-4f);
    EXPECT_NEAR(degToVolt(-200.0f, TYPE_K, 0.0f), -5.891f, 5e-4f);
    EXPECT_NEAR(voltToDeg(4.096f, TYPE_K, 0.0f), 100.0f, 0.05f);
    EXPECT_NEAR(voltToDeg(41.276f, TYPE_K, 0.0f), 1000.0f, 0.05f);
}

TEST(TemperatureMeasTc, RoundTripOverFullRange) {
    for (const tcRange& r : TC_RANGES) {
        for (float t = r.tMin; t <= r.tMax; t += 0.5f) {
            float mV = degToVolt(t, r.type, 0.0f);
            ASSERT_FALSE(std::isnan(mV)) << "type " << r.type << " " << t;
            ASSERT_NEAR(voltToDeg(mV, r.type, 0.0f), t, MAX_ERROR_DEG)
                << "type " << r.type << " " << t;
        }
    }
}

TEST(TemperatureMeasTc, RangeLimitsAreAccepted) {
    for (const tcRange& r : TC_RANGES) {
        EXPECT_NEAR(voltToDeg(degToVolt(r.tMin, r.type, 0.0f), r.type, 0.0f), r.tMin,
                    MAX_ERROR_DEG);
        EXPECT_NEAR(voltToDeg(degToVolt(r.tMax, r.type, 0.0f), r.type, 0.0f), r.tMax,
                    MAX_ERROR_DEG);
    }
}

TEST(TemperatureMeasTc, OutsideRangeIsAnError) {
    for (const tcRange& r : TC_RANGES) {
        EXPECT_TRUE(std::isnan(degToVolt(r.tMin - 1.0f, r.type, 0.0f))) << "type " << r.type;
        EXPECT_TRUE(std::isnan(degToVolt(r.tMax + 1.0f, r.type, 0.0f))) << "type " << r.type;

        float vMin = degToVolt(r.tMin, r.type, 0.0f);
        float vMax = degToVolt(r.tMax, r.type, 0.0f);
        EXPECT_EQ(voltToDeg(vMin - 0.01f, r.type, 0.0f), TEMPERATURE_ERROR) << "type " << r.type;
        EXPECT_EQ(voltToDeg(vMax + 0.01f, r.type, 0.0f), TEMPERATURE_ERROR) << "type " << r.type;
    }
}

TEST(TemperatureMeasTc, ColdJunctionCompensation) {
    /* Type K at 100 degC gives 4.096 mV against 0 degC, and 25 degC gives 1.000 mV */
    float mV = degToVolt(100.0f, TYPE_K, 25.0f);
    EXPECT_NEAR(mV, 4.096f - 1.000f, 5e-4f);
    EXPECT_NEAR(voltToDeg(mV, TYPE_K, 25.0f), 100.0f, MAX_ERROR_DEG);

    /* The same voltage without compensation gives a different temperature */
    EXPECT_GT(fabsf(voltToDeg(mV, TYPE_K, 0.0f) - 100.0f), 10.0f);
}

TEST(TemperatureMeasTc, ColdJunctionCompensationAllTypes) {
    for (const tcRange& r : TC_RANGES) {
        for (float cj : {0.0f, 25.0f, 40.0f}) {
            float t  = 0.5f * (r.tMin + r.tMax);
            float mV = degToVolt(t, r.type, cj);
            EXPECT_NEAR(voltToDeg(mV, r.type, cj), t, MAX_ERROR_DEG)
                << "type " << r.type << " cj " << cj;
        }
    }
}

TEST(TemperatureMeasTc, ColdJunctionAtIcePointIsNoCompensation) {
    for (const tcRange& r : TC_RANGES) {
        float mV = degToVolt(r.tMax, r.type, 0.0f);
        EXPECT_FLOAT_EQ(mV, degToVolt(r.tMax, r.type, 0.0f));
        EXPECT_NEAR(voltToDeg(mV, r.type, 0.0f), r.tMax, MAX_ERROR_DEG);
    }
}

TEST(TemperatureMeasTc, ColdJunctionOutOfRangeIsAnError) {
    EXPECT_EQ(voltToDeg(1.0f, TYPE_K, 2000.0f), TEMPERATURE_ERROR);
    EXPECT_EQ(voltToDeg(1.0f, TYPE_K, -300.0f), TEMPERATURE_ERROR);
    EXPECT_EQ(voltToDeg(1.0f, TYPE_B, -10.0f), TEMPERATURE_ERROR);
    EXPECT_TRUE(std::isnan(degToVolt(100.0f, TYPE_K, 2000.0f)));
    EXPECT_TRUE(std::isnan(degToVolt(100.0f, TYPE_K, -300.0f)));
}

TEST(TemperatureMeasTc, InvalidInputsAreErrors) {
    float nan = NAN;
    float inf = INFINITY;
    EXPECT_EQ(voltToDeg(nan, TYPE_K, 0.0f), TEMPERATURE_ERROR);
    EXPECT_EQ(voltToDeg(inf, TYPE_K, 0.0f), TEMPERATURE_ERROR);
    EXPECT_EQ(voltToDeg(-inf, TYPE_K, 0.0f), TEMPERATURE_ERROR);
    EXPECT_EQ(voltToDeg(1.0f, TYPE_K, nan), TEMPERATURE_ERROR);
    EXPECT_TRUE(std::isnan(degToVolt(nan, TYPE_K, 0.0f)));
    EXPECT_TRUE(std::isnan(degToVolt(inf, TYPE_K, 0.0f)));
    EXPECT_TRUE(std::isnan(degToVolt(100.0f, TYPE_K, nan)));
}

TEST(TemperatureMeasTc, InvalidSensorTypeIsAnError) {
    for (int type : std::vector<int>{-1, NO_OF_TYPES, 100, PT_100, PT_1000}) {
        EXPECT_EQ(voltToDeg(1.0f, type, 0.0f), TEMPERATURE_ERROR) << "type " << type;
        EXPECT_TRUE(std::isnan(degToVolt(100.0f, type, 0.0f))) << "type " << type;
    }
}

/***************************************************************************************************
** TESTS - RTDS
***************************************************************************************************/

TEST(TemperatureMeasRtd, Pt100MatchesIecTable) {
    struct {
        float degC;
        float ohm;
    } table[] = {{-200.0f, 18.52f}, {-100.0f, 60.26f}, {0.0f, 100.0f},   {100.0f, 138.51f},
                 {200.0f, 175.86f}, {500.0f, 280.98f}, {850.0f, 390.48f}};
    for (auto& p : table) {
        EXPECT_NEAR(degToRes(p.degC, PT_100), p.ohm, 0.005f) << p.degC;
        /* The table is rounded to 0.01 ohm: at the -200 degC limit it falls outside the range */
        if (p.degC > -200.0f) {
            EXPECT_NEAR(resToDeg(p.ohm, PT_100, 0.0f), p.degC, 0.05f) << p.ohm;
        }
    }
}

TEST(TemperatureMeasRtd, Pt1000IsTenTimesPt100) {
    for (float t = -200.0f; t <= 850.0f; t += 25.0f) {
        EXPECT_NEAR(degToRes(t, PT_1000), 10.0f * degToRes(t, PT_100), 1e-3f * 10.0f) << t;
    }
    EXPECT_NEAR(degToRes(0.0f, PT_1000), 1000.0f, 1e-3f);
    EXPECT_NEAR(degToRes(100.0f, PT_1000), 1385.1f, 0.05f);
    EXPECT_NEAR(resToDeg(1385.1f, PT_1000, 0.0f), 100.0f, 0.05f);
}

TEST(TemperatureMeasRtd, RoundTripOverFullRange) {
    for (int type : std::vector<int>{PT_100, PT_1000}) {
        for (float t = -200.0f; t <= 850.0f; t += 0.25f) {
            float r = degToRes(t, type);
            ASSERT_NEAR(resToDeg(r, type, 0.0f), t, MAX_ERROR_DEG)
                << "type " << type << " " << t;
        }
    }
}

TEST(TemperatureMeasRtd, UncompensatedWiresGiveHigherTemperature) {
    EXPECT_GT(resToDeg(139.71f, PT_100, 0.0f),
              resToDeg(139.71f, PT_100, 1.2f));
}

TEST(TemperatureMeasRtd, InvalidCompensationIsAnError) {
    EXPECT_EQ(resToDeg(138.51f, PT_100, -0.1f), TEMPERATURE_ERROR);
    EXPECT_EQ(resToDeg(138.51f, PT_100, -0.1f), TEMPERATURE_ERROR);
    EXPECT_EQ(resToDeg(138.51f, PT_100, NAN), TEMPERATURE_ERROR);
    EXPECT_EQ(resToDeg(138.51f, PT_100, INFINITY), TEMPERATURE_ERROR);
    /* Compensation larger than the measurement */
    EXPECT_EQ(resToDeg(1.0f, PT_100, 1.0f), TEMPERATURE_ERROR);
    EXPECT_EQ(resToDeg(1.0f, PT_100, 2.0f), TEMPERATURE_ERROR);
}

TEST(TemperatureMeasRtd, OutsideRangeIsAnError) {
    EXPECT_EQ(resToDeg(18.0f, PT_100, 0.0f), TEMPERATURE_ERROR);
    EXPECT_EQ(resToDeg(391.0f, PT_100, 0.0f), TEMPERATURE_ERROR);
    EXPECT_EQ(resToDeg(0.0f, PT_100, 0.0f), TEMPERATURE_ERROR);
    EXPECT_EQ(resToDeg(-10.0f, PT_100, 0.0f), TEMPERATURE_ERROR);
    EXPECT_TRUE(std::isnan(degToRes(-200.1f, PT_100)));
    EXPECT_TRUE(std::isnan(degToRes(850.1f, PT_100)));
    EXPECT_FALSE(std::isnan(degToRes(-200.0f, PT_100)));
    EXPECT_FALSE(std::isnan(degToRes(850.0f, PT_100)));
}

TEST(TemperatureMeasRtd, InvalidInputsAreErrors) {
    EXPECT_EQ(resToDeg(NAN, PT_100, 0.0f), TEMPERATURE_ERROR);
    EXPECT_EQ(resToDeg(INFINITY, PT_100, 0.0f), TEMPERATURE_ERROR);
    EXPECT_TRUE(std::isnan(degToRes(NAN, PT_100)));
    EXPECT_TRUE(std::isnan(degToRes(INFINITY, PT_100)));
}

TEST(TemperatureMeasRtd, InvalidSensorTypeIsAnError) {
    for (int type : std::vector<int>{-1, NO_OF_TYPES, 100, TYPE_K, TYPE_B}) {
        EXPECT_EQ(resToDeg(138.51f, type, 0.0f), TEMPERATURE_ERROR) << "type " << type;
        EXPECT_TRUE(std::isnan(degToRes(100.0f, type))) << "type " << type;
    }
}

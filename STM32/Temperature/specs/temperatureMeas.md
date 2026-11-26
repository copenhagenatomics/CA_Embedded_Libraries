# Specification Document - Temperature library

Author: Timothé Dodin
Date: 25/11/2026

# Introduction

This document contains the specification for the temperature measurement library (`temperatureMeas`). The library converts raw sensor readings (thermocouple voltage, RTD resistance) into temperature, and the reverse. It is hardware independent: it performs no I/O and holds no state, so that it can be used with any ADC front-end.

# Specification

* Supported sensors:
  * Thermocouples of type B, E, J, K, N, R, S and T
    * Characterised according to NIST ITS-90 thermocouple reference functions
  * Platinum RTDs: PT100 (R0 = 100 ohm) and PT1000 (R0 = 1000 ohm)
    * Characterised according to IEC 60751 (Callendar-Van Dusen)
* Units:
  * Temperature: degrees Celsius
  * Thermocouple voltage: millivolts
  * RTD resistance: ohms
* Sensor selection:
  * The sensor is selected with a `sensorType_e` value (B, E, J, K, N, R, S, T, PT_100, PT_1000)
  * An invalid sensor type is handled as an error (see Error handling)
* Conversions (all sensors):
  * Measurement to temperature: thermocouple voltage (mV) or RTD resistance (ohm) to degC
  * Temperature to measurement: degC to thermocouple voltage (mV) or RTD resistance (ohm)
  * Both directions are available for every supported sensor, and each is the inverse of the other within the accuracy given below
  * Thermocouples:
    * Forward (degC to mV): NIST reference polynomials (ITS-90), using the exponential correction term for type K above 0 degC
    * Inverse (mV to degC): NIST approximate inverse polynomials (NIST Monograph 175, appendix A), with the coefficient set selected by voltage range. Their error versus the reference function is below 0.06 degC
  * RTDs:
    * Forward (degC to ohm): Callendar-Van Dusen equation, with the below-0 degC term (C) included
    * Inverse (ohm to degC): closed-form solution of the quadratic for T >= 0 degC; for T < 0 degC a polynomial approximation or a fixed number of Newton iterations (no unbounded loops)
    * PT1000 uses the same equation as PT100, with R0 = 1000 ohm
* Cold junction compensation (thermocouples only)
* Wire resistance compensation (RTDs only)
* Error handling:
  * An error is signalled by returning the value 10000 (`TEMPERATURE_ERROR`)
  * Errors are returned for: input outside the valid range of the sensor, NaN or infinite input, invalid sensor type, invalid cold junction temperature, invalid wire compensation
  * For conversions to a measurement (degC to mV / ohm), the error is NaN, as 10000 is a valid number in these units
  * The library never reads or writes outside of its tables and never loops indefinitely, whatever the input

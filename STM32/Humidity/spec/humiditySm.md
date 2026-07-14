# Specification Document - AC

Author: Luke Walker
Date: 07/07/2026

# Introduction

This document contains the specification for the humidity state machine.

# Specification

* Operate a SHT45 humidity sensor with the following key features:
  * Readout humidity as frequently as possible from the I2C device
  * Readout temperature as frequently as possible from the I2C device
  * Have a selectable burn-in mode that heats the sensor on every measurement cycle for 80 minutes
  * Calculate Absolute humidity from temperature / RH readings
  * Burnin Temperature protection - If the temperature of the sensor is >80 degC before starting the heater, do not run a heating cycle
  * Bus recovery - if an I2C error is detected, attempt to reset the bus by clearing the line
  * Condensation protection - if high humidity is detected, run a heating cycle every 60 seconds to remove condensation
  * Filtering - filter RH, AH and temperature with a short digital filter
  * Error handling - after 10 consecutive measurement errors, give outputs invalid values (temp=10000, humidity=-1)
* API
  * setup + loop type functions API
  * Use sht45 driver from I2C library
  * Functions to get Relative + Absolute humidity, temperature, and device serial
  * Set board status bits in case of humidity error (supplied during setup)


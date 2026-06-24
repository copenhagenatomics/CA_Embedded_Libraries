# Specification Document - CAProtocol

Author: Luke Walker
Date: 09/06/2026

# Introduction

This document contains the specification for the CAProtocol. Not all parts of this specification are a requirement for all boards. CA firmware specifications should specify which parts of this spec to implement.

# Specification

* Provide hooks for the following common commands (further detail in subsequent sections)
  * Serial
  * Status
  * StatusDef
  * DFU
  * CAL
  * LOG
  * OTP
  * uptime
* uptime
  * Command format:
    * uptime [s|l] [r <channel>]
    * The three options (s, l, r) are mutually exclusive
      * s = store uptime records to flash
      * l = load uptime records from flash
      * r = reset a channel count to 0
    * when called without arguments, prints the currently loaded uptime information
  * Data storage
    * Each uptime record must consist of:
      * Title (Free text), Channel index (unique per uptime record), reset count (integer describing how many times the channel has been reset to 0), count (integer with actual count)
    * Must store the following information:
      * Total board operating minutes
      * Minutes since rework
      * Minutes since software update
      * Software failures
    * May store additional information
    * Data must be stored to flash once per day
  * Must automatically detect:
    * Change of firmware and reset software count channel
    * Software failures (e.g. indicated by watchdog reset)
    * Data corruption
    

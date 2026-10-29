# Specification Document - CAProtocol

Author: Luke Walker
Date: 20/10/2026

# Introduction

This document contains the specification for the Copenhagen Atomics (CA) general purpose USB protocol.

# Specification

* The interface is a serial port:
  * Nominal communication parameters are 115200 8N1:
    * Baud rate: 115200
    * Data bits: 8
    * Parity: None
    * Stop bit: 1
  * In most cases, the port will be USB Communications Device Class (CDC) - Virtual serial port
    * Since this port is virtual, communication parameters are for completeness only. 
* Output:
  * Upon opening the serial port, the device will print one line with the reason for the last boot
    * Format: `reconnected Reset Reason: XYZ`
  * The device will automatically printout a comma separated list of relevant variables at a rate of 10 Hz, followed by a CRLF (\r\n)
    * It will be possible to request an output rate of 100 Hz, implemented on a per-device basis
    * Each comma separated variable is referred to as an "output channel". 
      * e.g. the first variable printed (before the first comma) is channel 0.
  * The final value of the output string will always be a status code. 
    * Format: 32-bit 0-padded hex value with the format `0x0123abcd`.
    * The highest bit (e.g. bit 31) is a general error indicator and will be set when any other error bit is set
    * See systemInfo for general status bit definitions
  * Multi-line status/error messages must always start and end with the following messages:
    * Start: `Start of <message name>:`
    * End: `End of <message name>.`
    * The response to the `Serial` command is excepted from this requirement for historic reasons
* Input:
  * Boards will accept ASCII text input. Commands will be delineated by CR, LF, or both.
  * Required commands are:
    * DFU: Will reset the device into DFU mode
    * Serial: will print general board parameters, followed by the CAL string for the device, if applicable. The CAL string will follow the same format as the response to `CAL r`.
      * Format:
      ```
      Serial Number: 2A00393235511039393434
      Product Type: GasSampler
      Sub Product Type: 0
      MCU Family: STM32F401xB/C Rev 1
      Software Version: 0.4.0
      Compile Date: 2026-05-28
      Git SHA: d4a9120cc3949c51a307d2eaacff6ceba54c59a4
      PCB Version: 1.3
      Calibration: CAL 1,3.250,3.250,9100 2,2800000,2.800,2.800 3,500000,10000,500000 4,-1.20,0.98,-1.82 5,0.00,0.00,0.00
      ```
    * OutputDef: Will print a text definition of each output channel, followed by a comma, followed by a unit:
      * Example format:
      ```
      Start of board output definition:
      Temperature Channel 1,degC
      Temperature Channel 2,degC
      ...etc...
      Temperature Channel 10,degC
      Status,hex
      End of board output definition.
      ```
    * StatusDef: Will print a text definition of each status bit, along with a hex value demonstrating the range of that particular value. The first line will define which bits are error bits.
      * Format will be a hex value with the status bit(s) set, in the same format as the status code (See Output section), followed by a comma, followed by the text definition:
      ```
      Start of board status definition:
      0x7e000300,System errors
      0x80000000,Error
      0x40000000,Over temperature
      ...etc...
      0x0000000f,Sequence Step
      0x00000010,Pump running
      End of board status definition.
      ``` 
      * Note: the example above shows both single bit (Pump running) and multi-bit (Sequence step) status codes.
    * Status: Will print a verbose summary of the current board status. If there are no errors, the first line will always be `The board is operating normally.`.
      * Example format (error):
      ```
      Start of board status:
      Under voltage. The board operates at too low voltage of 0.00V. Check power supply.
      Sequence step is: 0
      Pump is: Off
      ...etc...
      SM4291 Pressure Sensor serial is: 0x012024e6
      End of board status.
      ```
      * Example format (No error):
      ```
      Start of board status:
      The board is operating normally.
      Port 1 measures voltage [0-5V]
      ...etc...
      Port 6 measures voltage [0-5V]
      End of board status.
      ```
  * Optional commands are:
    * CAL: Will send (a) calibration string(s) to the device. 
      * Calibration strings have the format: "p, a, b, g", where:
        * p is the calibration number (sometimes referred to as the port number)
        * a is alpha, a dimensionless calibration value, floating point
        * b is beta, an optional dimensionless calibration value, floating point
        * g is gamma, an optional dimensionless calibration value, floating point
      * At least one calibration string is mandatory. Multiple may be sent in the same line. Acceptable examples:
        * `CAL 1,2`
        * `CAL 1,2,3`
        * `CAL 1,2,3,4`
        * `CAL 1,2,3,4 2,5,6,7`
      * A device may reject multiple calibrations on the same line if multiple calibration steps are required for that device. 
      * Calibration values adjusted with CAL should be applied immediately, but held in volatile memory only
        * `CAL w` will be used to write the calibration values to non-volatile memory. The calibration values should be stored along with a CRC for data verification.
        * `CAL r` will be used to readout the currently applied calibration values (e.g. the ones held in volatile memory)
          * Response format: `Calibration: CAL 1,a,b,g 2,a,b,g ...etc... 9,a,b,g 10,a,b,g`
          * The readout of action flags (see below) should be 0.
        * `CAL m` will be used to readout the calibration values stored in non-volatile memory, with the same format as CAL r
      * The device must respond to a CAL message with a response indicating success or failure:
        * `CAL x success` (where x could be a p number or w)
        * `CAL x failed` (where x could be a p number or w or m)
          * Note: readout of the CAL values implicitly passes `CAL r` or `CAL m`
        * In the case of multiple calibrations sent in a single line, a response must be printed for each CAL:
          * Example response when sending CAL 1,0.0 2,NAN 3,0.0:
          ```
          CAL 1 success
          CAL 2 failed
          CAL 3 success
          ```
      * In some CAL cases, a parameter may be used as an "action flag". For example, alpha = an offset, and beta is an "action flag" specifying how to calibrate alpha. If beta = 0, then the offset should be set based on the `CAL 0.1,0` command (e.g. 0.1). If beta = 1, the offset should be measured from sensor data.
        * action flag parameters should be read out as 0 when using `CAL r` or similar.
    * LOG: Will change the output string channels between different configurations. Used to implement debugging output channels which are not desired in general purpose usage
      * Format: `LOG pX` where X is a different output configuration. 0 is always the default general purpose configuration.
      * For differing LOG configurations, the OutputDef printout must match the currently active configuration
    * uptime: Interact with the uptime subsystem (see Uptime subsystem):
      * Format: `uptime [s|l|r ch]` where:
        * No arguments will result in the uptime table being printed out. The printout always starts with the `Serial` header. Example format:
        ```
        Start of uptime
        Serial Number: 3100024650501520313232
        Product Type: AnalogInput
        ...etc...
        PCB Version: 1.0
        Calibration: CAL 1,1.0000000000,0.0000000000,0 2,1.0000000000,0.0000000000,0 3,1.0000000000,0.0000000000,0 4,1.0000000
        Name, channel, reset, count
        Total board uptime minutes, 0, 0, 0
        Minutes since rework, 1, 0, 0
        Minutes since last software update, 2, 1, 0
        Software failures, 3, 0, 0
        ...etc...
        Optional per board datapoint N, 12, 0, 0
        End of uptime
        ```
        * `s` as an argument will store the existing uptime table
        * `l` as an argument will load the uptime table from flash, overwriting the volatile copy
        * `r ch` as an argument will reset the count for channel ch
          * As an exception, "Total board operating minutes" - channel 0 - cannot be reset.
* Uptime subsystem:
  * CA boards will implement an uptime subsystem that will record the running time both for the board and optionally for attached sensors and actuators.
  * The uptime subsystem will maintain a table where each row (also called "channel") records the running time or count of a particular datapoint.
    * Every board will maintain at least the following datapoints:
      * Total board operating minutes
      * Minutes since last rework
      * Minutes since last software update
      * Number of software failures
    * For each datapoint, the following information is saved in the table row:
      * Datapoint id - an integer
      * Datapoint description - a text string with a readable description of the datapoint
      * Reset count - an integer, the number of times a particular count has been reset
      * Count - an integer, the actual count per datapoint.
    * Every minute, the uptime subsystem will automatically update minute counters held in volatile memory.
    * Every 24 hours (measured since last boot), the uptime subsystem will save the entire table into non-volatile memory
      * The table should be stored along with a CRC for data verification.
    * The uptime subsystem will automatically detect changes in SW version, and reset channel 2 ("Minutes since last software update") automatically after boot when a change is detected


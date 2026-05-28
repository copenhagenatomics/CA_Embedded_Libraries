# Specification Document - CAProtocolACDC

Author: Luke Walker
Date: 28/05/2026

# Introduction

This document contains the specification for the CAProtocolACDC.

# Specification

* Defines the following commands:
  * all on - turn all ports on indefinitely
  * all off - turn all ports off
  * pX off - turn off port number X
  * pX on - turn on port number X indefinitely 'always on'
  * pX on YY - turn on port number X for YY seconds
  * pX on ZZZ% - turn on port number X on ZZ percent of the time using PWM 'always on'
  * pX on YY ZZZ% - turn on port number X for YY seconds ZZ percent of the time using PWM
* Implementation of the commands can be accomplished using the following callbacks:
  * allOn
  * allOff
  * portState

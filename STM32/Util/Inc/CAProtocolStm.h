#pragma once
#include "CAProtocol.h"
#include <stdbool.h>

void HALundefined(const char *input);
void HALJumpToBootloader();
void CAPrintHeader();
void CAPrintStatus(bool printStart);
void CAotpRead();

// analyse reason for boot and in case of SW reset jump to DFU SW update.
const char* CAonBoot();

// Generic handler for a CAProtocolCtx handler.
bool CAhandleUserInputs(CAProtocolCtx* ctx, const char* startMsg);

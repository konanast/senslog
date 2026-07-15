#ifndef MODULAR_LOGGER_HEALTH_H
#define MODULAR_LOGGER_HEALTH_H
#include <Arduino.h>
#include "Config.h"

void healthBegin();
uint16_t readVccMillivolts();
uint16_t readBatteryMillivolts();
uint8_t readBatteryPercent();
int freeRamBytes();
uint8_t resetCause();

#endif

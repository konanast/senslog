#ifndef MODULAR_LOGGER_TIMESTAMP_H
#define MODULAR_LOGGER_TIMESTAMP_H
#include <Arduino.h>
#include "Status.h"

void timestampBegin();
void printDate(Print &out);
void printTime(Print &out);
void printTimestampSource(Print &out);
bool rtcAvailable();
void printRtcStatus(Print &out);
void printStartTime(Print &out);

#endif

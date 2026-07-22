#ifndef MODULAR_LOGGER_LOGGER_H
#define MODULAR_LOGGER_LOGGER_H
#include <Arduino.h>
#include <SD.h>

struct MeasurementDescriptor {
  const char *header;
  void (*printValue)(Print &out);
};

void loggerBegin(File &file);
void loggerWriteHeader(Print &out);
bool loggerWriteRow(File &file);
void printMetadata(Print &out);

#endif

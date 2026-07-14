#ifndef MODULAR_LOGGER_SD_FILE_MANAGER_H
#define MODULAR_LOGGER_SD_FILE_MANAGER_H
#include <Arduino.h>
#include <SD.h>

bool storageBegin();
bool openNextLogFile(File &file, char *name, size_t nameLen);
bool writeInfoFile();
void safeFlush(File &file);
void safeClose(File &file);

#endif

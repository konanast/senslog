#include "SDFileManager.h"
#include "Config.h"
#include "Status.h"
#include "Logger.h"
#include <SPI.h>

bool storageBegin() {
  if (!SD.begin(SD_CS_PIN)) {
    g_sdStatus |= STATUS_SD_INIT_FAIL;
    g_sdErrorCount++;
    return false;
  }
  return true;
}

bool openNextLogFile(File &file, char *name, size_t nameLen) {
  for (uint16_t i = 0; i <= LOG_FILE_MAX_INDEX; i++) {
    snprintf(name, nameLen, "%s%04u%s", LOG_PREFIX, i, LOG_EXTENSION);
    if (!SD.exists(name)) {
      file = SD.open(name, FILE_WRITE);
      if (file) return true;
      g_sdStatus |= STATUS_SD_WRITE_FAIL;
      g_sdErrorCount++;
      return false;
    }
  }
  g_sdStatus |= STATUS_SD_WRITE_FAIL;
  g_sdErrorCount++;
  return false;
}

bool writeInfoFile() {
  File info = SD.open(INFO_FILE_NAME, FILE_WRITE);
  if (!info) { g_sdStatus |= STATUS_SD_WRITE_FAIL; g_sdErrorCount++; return false; }
  info.println(F("--- boot ---"));
  printMetadata(info);
  info.flush();
  info.close();
  return true;
}

void safeFlush(File &file) {
  if (file) file.flush();
}

void safeClose(File &file) {
  if (file) { file.flush(); file.close(); }
}

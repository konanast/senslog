/*
  ModularLogger - production-oriented Arduino Pro Mini data logger.

  Wiring summary:
  - microSD module: 3V3 -> 3.3 V supply, CS -> configurable SD_CS_PIN,
    MOSI -> D11, CLK/SCK -> D13, MISO -> D12, GND -> GND.
    Low-cost SD modules may not tolerate 5 V logic. Use a 3.3 V Pro Mini,
    or a module with level shifting when using a 5 V / 16 MHz Pro Mini.
  - RTC module: GND -> GND, VCC -> rated supply, SCL -> A5, SDA -> A4.
  - Battery monitor: battery positive -> R1 -> ADC pin -> R2 -> GND.
    Battery_mV = ADC_mV * (R1 + R2) / R2. Keep ADC pin <= Vcc.

  Select a build in Config.h with LOGGER_PRESET, or pass -DLOGGER_PRESET=n
  from your build system. Unused sensor libraries are excluded by #if flags.
*/

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include "Config.h"
#include "Status.h"
#include "Health.h"
#include "Timestamp.h"
#include "Sensors.h"
#include "SDFileManager.h"
#include "Logger.h"

static File logFile;
static char logFileName[13];
static unsigned long nextLogMs = 0;
static uint16_t rowsSinceFlush = 0;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  healthBegin();
  g_bootId = millis();

  timestampBegin();
  sensorsBegin();

  if (!storageBegin()) {
    Serial.println(F("ERROR: SD init failed"));
    return;
  }

  writeInfoFile();
  if (!openNextLogFile(logFile, logFileName, sizeof(logFileName))) {
    Serial.println(F("ERROR: log open failed"));
    return;
  }

  loggerBegin(logFile);
  logFile.flush();
  Serial.print(F("Logging to "));
  Serial.println(logFileName);
  nextLogMs = millis();
}

void loop() {
  unsigned long loopStart = millis();
  sensorsPollFast();
  sensorsUpdate();

  unsigned long now = millis();
#if LOG_WAIT_FOR_INITIAL_SENSOR_READS
  // Keep nextLogMs unchanged while waiting, so the first completed sensor
  // sample is written immediately rather than after another log interval.
  if (!sensorsInitialReadingsReady()) {
    g_lastLoopTimeMs = (uint16_t)(millis() - loopStart);
    return;
  }
#endif
  if (logFile && (long)(now - nextLogMs) >= 0) {
    g_recordId++;
    bool ok = loggerWriteRow(logFile);
    if (!ok) { g_sdStatus |= STATUS_SD_WRITE_FAIL; g_sdErrorCount++; }
    rowsSinceFlush++;
    if (rowsSinceFlush >= FLUSH_EVERY_N_ROWS) { safeFlush(logFile); rowsSinceFlush = 0; }
    nextLogMs += LOG_INTERVAL_MS;
    if ((long)(millis() - nextLogMs) >= 0) nextLogMs = millis() + LOG_INTERVAL_MS;
  }

  g_lastLoopTimeMs = (uint16_t)(millis() - loopStart);
}

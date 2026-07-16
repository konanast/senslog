#ifndef MODULAR_LOGGER_STATUS_H
#define MODULAR_LOGGER_STATUS_H
#include <Arduino.h>

enum TimestampSource : uint8_t { TS_UPTIME = 0, TS_RTC = 1, TS_GPS = 2 };

enum StatusBits : uint16_t {
  STATUS_OK = 0,
  STATUS_SD_INIT_FAIL = 1 << 0,
  STATUS_SD_WRITE_FAIL = 1 << 1,
  STATUS_RTC_MISSING = 1 << 2,
  STATUS_RTC_INVALID = 1 << 3,
  STATUS_SENSOR_ENV = 1 << 4,
  STATUS_SENSOR_MOTION = 1 << 5,
  STATUS_SENSOR_GPS = 1 << 6,
  STATUS_SENSOR_WIND = 1 << 7,
  STATUS_SENSOR_RAIN = 1 << 8,
  STATUS_SENSOR_BATTERY = 1 << 9,
  STATUS_RTC_ADJUSTED = 1 << 10
};

extern uint16_t g_sensorStatus;
extern uint16_t g_rtcStatus;
extern uint16_t g_sdStatus;
extern uint16_t g_sdErrorCount;
extern uint16_t g_invalidReadingCount;
extern uint32_t g_recordId;
extern uint32_t g_bootId;
extern uint16_t g_lastLoopTimeMs;
extern uint16_t g_lastSdWriteTimeMs;
extern TimestampSource g_timestampSource;

#endif

#ifndef MODULAR_LOGGER_CONFIG_H
#define MODULAR_LOGGER_CONFIG_H

#include <Arduino.h>

#define FW_VERSION "2.0.0"
#define DEVICE_ID "SENSLOG-001"

// Select exactly one preset before including this file, or keep the default.
#ifndef LOGGER_PRESET
#define LOGGER_PRESET 2
#endif

#define PRESET_BMP280 1
#define PRESET_BME280 2
#define PRESET_BME680 3
#define PRESET_GY21 4
#define PRESET_MOBILE_BME280 5
#define PRESET_MOBILE_BME680 6
#define PRESET_WEATHER_STATION 7

// Shared defaults. Presets may override these after they undef the value.
#define SD_CS_PIN 10
#define LOG_INTERVAL_MS 10000UL
#define FLUSH_EVERY_N_ROWS 6
#define SENSOR_FAST_INTERVAL_MS 200UL
#define SENSOR_ENV_INTERVAL_MS 10000UL
#define SENSOR_GPS_INTERVAL_MS 200UL
#define SENSOR_MOTION_INTERVAL_MS 1000UL
// 1: do not write CSV rows until every enabled periodic sensor has made its
// first read attempt.  This prevents startup rows containing only empty
// sensor fields when a sensor interval is longer than LOG_INTERVAL_MS.
// 0: begin logging immediately; early rows may contain empty sensor fields.
#define LOG_WAIT_FOR_INITIAL_SENSOR_READS 1
#define INFO_FILE_NAME "INFO.TXT"
#define LOG_PREFIX "LOG"
#define LOG_EXTENSION ".CSV"
#define LOG_FILE_DIGITS 4
#define LOG_FILE_MAX_INDEX 9999U
#define SEA_LEVEL_PRESSURE_HPA 1013.25f
#define USE_EMPTY_FOR_INVALID 1

// Board declaration for metadata only.
#define BOARD_VOLTAGE "3.3V-or-5V"
#define BOARD_CLOCK "8MHz-or-16MHz"

// Battery divider defaults: Vbat -> R1 -> ADC -> R2 -> GND.
#define BATTERY_PIN A0
#define BATTERY_R1_OHMS 100000UL
#define BATTERY_R2_OHMS 100000UL
#define BATTERY_ADC_REF_MV 3300UL
#define BATTERY_PCT_ENABLED 0
#define BATTERY_EMPTY_MV 3300UL
#define BATTERY_FULL_MV 4200UL

#define WIND_SPEED_PIN 2
#define WIND_DIR_PIN A1
#define RAIN_PIN 3
#define WIND_MS_PER_PULSE 0.667f
#define RAIN_MM_PER_TIP 0.2794f

// Feature flags start disabled. Presets enable only what they need.
#define USE_RTC 1
#define RTC_TYPE_DS3231 1
#define RTC_TYPE_DS1307 0
#define RTC_SET_ON_INVALID 1
#define RTC_SET_ON_EVERY_BOOT 0
#define USE_BMP280 0
#define USE_BME280 0
#define USE_BME680 0
#define USE_GY21 0
#define USE_MPU6050 0
#define USE_ADXL345 0
#define USE_GPS 0
#define USE_BATTERY 0
#define USE_WIND_SPEED 0
#define USE_WIND_DIR 0
#define USE_RAIN 0
#define USE_ANALOG_MQ 0

#if LOGGER_PRESET == PRESET_BMP280
#include "presets/Preset_BMP280.h"
#elif LOGGER_PRESET == PRESET_BME280
#include "presets/Preset_BME280.h"
#elif LOGGER_PRESET == PRESET_BME680
#include "presets/Preset_BME680.h"
#elif LOGGER_PRESET == PRESET_GY21
#include "presets/Preset_GY21.h"
#elif LOGGER_PRESET == PRESET_MOBILE_BME280
#include "presets/Preset_Mobile_BME280.h"
#elif LOGGER_PRESET == PRESET_MOBILE_BME680
#include "presets/Preset_Mobile_BME680.h"
#elif LOGGER_PRESET == PRESET_WEATHER_STATION
#include "presets/Preset_WeatherStation.h"
#else
#error "Unknown LOGGER_PRESET"
#endif

#if (USE_BMP280 + USE_BME280 + USE_BME680 + USE_GY21) > 1
#error "Enable only one primary environmental sensor adapter per build."
#endif

#if USE_RTC && (RTC_TYPE_DS3231 + RTC_TYPE_DS1307) != 1
#error "Select exactly one RTC type when USE_RTC is enabled."
#endif

#endif

#include "Logger.h"
#include "Config.h"
#include "Timestamp.h"
#include "Sensors.h"
#include "Health.h"
#include "Status.h"

static void printFlashText(Print &out, const char *p) { char c; while ((c = pgm_read_byte(p++)) != 0) out.write(c); }
static void printFloat(Print &out, bool valid, float v, uint8_t d) { if (valid && isfinite(v)) out.print(v, d); else if (!USE_EMPTY_FOR_INVALID) out.print(F("NA")); }
static void printUInt(Print &out, bool valid, unsigned long v) { if (valid) out.print(v); else if (!USE_EMPTY_FOR_INVALID) out.print(F("NA")); }

static void pDate(Print &out) { printDate(out); }
static void pTime(Print &out) { printTime(out); }
static void pTimestampSource(Print &out) { printTimestampSource(out); }
static void pRecordId(Print &out) { out.print(g_recordId); }
static void pBootId(Print &out) { out.print(g_bootId); }
static void pUptime(Print &out) { out.print(millis() / 1000UL); }
static void pInterval(Print &out) { out.print(LOG_INTERVAL_MS); }
static void pTemp(Print &out) { printFloat(out, envHasTemp(), envTempC(), 2); }
static void pRH(Print &out) { printFloat(out, envHasHumidity(), envHumidityPct(), 2); }
static void pPressure(Print &out) { printFloat(out, envHasPressure(), envPressureHpa(), 2); }
static void pAltitude(Print &out) { printFloat(out, envHasAltitude(), envAltitudeM(), 2); }
static void pDew(Print &out) { printFloat(out, envHasDewPoint(), envDewPointC(), 2); }
static void pGasOhm(Print &out) { printFloat(out, envHasGasResistance(), envGasResistanceOhm(), 0); }
static void pGasAdc(Print &out) { printUInt(out, mqValid(), mqAdc()); }
static void pGasMv(Print &out) { printUInt(out, mqValid(), mqVoltageMv()); }
static void pAx(Print &out) { printFloat(out, motionValid(), accelX(), 3); }
static void pAy(Print &out) { printFloat(out, motionValid(), accelY(), 3); }
static void pAz(Print &out) { printFloat(out, motionValid(), accelZ(), 3); }
static void pGx(Print &out) { printFloat(out, motionValid(), gyroX(), 2); }
static void pGy(Print &out) { printFloat(out, motionValid(), gyroY(), 2); }
static void pGz(Print &out) { printFloat(out, motionValid(), gyroZ(), 2); }
static void pLat(Print &out) { printFloat(out, gpsValid(), gpsLat(), 6); }
static void pLon(Print &out) { printFloat(out, gpsValid(), gpsLon(), 6); }
static void pGpsAlt(Print &out) { printFloat(out, gpsValid(), gpsAltM(), 1); }
static void pGpsSpeed(Print &out) { printFloat(out, gpsValid(), gpsSpeedKmh(), 1); }
static void pGpsCourse(Print &out) { printFloat(out, gpsValid(), gpsCourseDeg(), 1); }
static void pGpsSats(Print &out) { printUInt(out, gpsValid(), gpsSatellites()); }
static void pGpsHdop(Print &out) { printFloat(out, gpsValid(), gpsHdop(), 2); }
static void pGpsFixValid(Print &out) { out.print(gpsValid() ? 1 : 0); }
static void pGpsFixAge(Print &out) { printUInt(out, gpsValid(), gpsFixAgeMs()); }
static void pWindSpeed(Print &out) { printFloat(out, windSpeedValid(), windSpeedMs(), 2); }
static void pWindDir(Print &out) { printFloat(out, windDirectionValid(), windDirectionDeg(), 1); }
static void pRain(Print &out) { printFloat(out, rainValid(), rainfallMm(), 2); }
static void pVcc(Print &out) { printUInt(out, true, readVccMillivolts()); }
static void pBat(Print &out) { printUInt(out, USE_BATTERY, readBatteryMillivolts()); }
static void pBatPct(Print &out) { uint8_t pct = readBatteryPercent(); printUInt(out, pct != 255, pct); }
static void pFreeRam(Print &out) { out.print(freeRamBytes()); }
static void pReset(Print &out) { out.print(resetCause()); }
static void pSensorStatus(Print &out) { out.print(g_sensorStatus); }
static void pRtcStatus(Print &out) { out.print(g_rtcStatus); }
static void pSdStatus(Print &out) { out.print(g_sdStatus); }
static void pSdErrors(Print &out) { out.print(g_sdErrorCount); }
static void pInvalid(Print &out) { out.print(g_invalidReadingCount); }
static void pLoopTime(Print &out) { out.print(g_lastLoopTimeMs); }
static void pSdWriteTime(Print &out) { out.print(g_lastSdWriteTimeMs); }

#define H(name) static const char h_##name[] PROGMEM = #name
H(Date); H(Time); H(TimestampSource); H(Record_ID); H(Boot_ID); H(Uptime_s); H(SampleInterval_ms); H(Temp_C); H(RH_pct); H(Pressure_hPa); H(Altitude_m); H(DewPoint_C); H(GasResistance_ohm); H(GasADC); H(GasVoltage_mV); H(AccelX_mps2); H(AccelY_mps2); H(AccelZ_mps2); H(GyroX_dps); H(GyroY_dps); H(GyroZ_dps); H(Latitude); H(Longitude); H(GPSAltitude_m); H(Speed_kmh); H(Course_deg); H(Satellites); H(HDOP); H(GPSFixValid); H(GPSFixAge_ms); H(WindSpeed_ms); H(WindDirection_deg); H(Rainfall_mm); H(Vcc_mV); H(Battery_mV); H(Battery_pct); H(FreeRAM_B); H(ResetCause); H(SensorStatus); H(RTC_Status); H(SD_Status); H(SD_ErrorCount); H(InvalidReadingCount); H(LoopTime_ms); H(SD_WriteTime_ms);

const MeasurementDescriptor measurements[] PROGMEM = {
  { h_Date, pDate }, { h_Time, pTime }, { h_TimestampSource, pTimestampSource }, { h_Record_ID, pRecordId }, { h_Boot_ID, pBootId }, { h_Uptime_s, pUptime }, { h_SampleInterval_ms, pInterval },
#if USE_BMP280 || USE_BME280 || USE_BME680 || USE_GY21
  { h_Temp_C, pTemp },
#endif
#if USE_BME280 || USE_BME680 || USE_GY21
  { h_RH_pct, pRH },
#endif
#if USE_BMP280 || USE_BME280 || USE_BME680
  { h_Pressure_hPa, pPressure }, { h_Altitude_m, pAltitude },
#endif
#if USE_BME280 || USE_BME680 || USE_GY21
  { h_DewPoint_C, pDew },
#endif
#if USE_BME680
  { h_GasResistance_ohm, pGasOhm },
#endif
#if USE_ANALOG_MQ
  { h_GasADC, pGasAdc }, { h_GasVoltage_mV, pGasMv },
#endif
#if USE_MPU6050 || USE_ADXL345
  { h_AccelX_mps2, pAx }, { h_AccelY_mps2, pAy }, { h_AccelZ_mps2, pAz },
#endif
#if USE_MPU6050
  { h_GyroX_dps, pGx }, { h_GyroY_dps, pGy }, { h_GyroZ_dps, pGz },
#endif
#if USE_GPS
  { h_Latitude, pLat }, { h_Longitude, pLon }, { h_GPSAltitude_m, pGpsAlt }, { h_Speed_kmh, pGpsSpeed }, { h_Course_deg, pGpsCourse }, { h_Satellites, pGpsSats }, { h_HDOP, pGpsHdop }, { h_GPSFixValid, pGpsFixValid }, { h_GPSFixAge_ms, pGpsFixAge },
#endif
#if USE_WIND_SPEED
  { h_WindSpeed_ms, pWindSpeed },
#endif
#if USE_WIND_DIR
  { h_WindDirection_deg, pWindDir },
#endif
#if USE_RAIN
  { h_Rainfall_mm, pRain },
#endif
  { h_Vcc_mV, pVcc },
#if USE_BATTERY
  { h_Battery_mV, pBat }, { h_Battery_pct, pBatPct },
#endif
  { h_FreeRAM_B, pFreeRam }, { h_ResetCause, pReset }, { h_SensorStatus, pSensorStatus }, { h_RTC_Status, pRtcStatus }, { h_SD_Status, pSdStatus }, { h_SD_ErrorCount, pSdErrors }, { h_InvalidReadingCount, pInvalid }, { h_LoopTime_ms, pLoopTime }, { h_SD_WriteTime_ms, pSdWriteTime }
};
const uint8_t MEASUREMENT_COUNT = sizeof(measurements) / sizeof(measurements[0]);

static MeasurementDescriptor readDescriptor(uint8_t i) { MeasurementDescriptor d; memcpy_P(&d, &measurements[i], sizeof(d)); return d; }

void loggerBegin(File &file) { loggerWriteHeader(file); }

void loggerWriteHeader(Print &out) {
  for (uint8_t i = 0; i < MEASUREMENT_COUNT; i++) { if (i) out.print(','); MeasurementDescriptor d = readDescriptor(i); printFlashText(out, d.header); }
  out.println();
}

bool loggerWriteRow(File &file) {
  unsigned long start = millis();
  for (uint8_t i = 0; i < MEASUREMENT_COUNT; i++) { if (i) file.print(','); MeasurementDescriptor d = readDescriptor(i); d.printValue(file); }
  file.println();
  g_lastSdWriteTimeMs = (uint16_t)(millis() - start);
  return file.getWriteError() == 0;
}

void printMetadata(Print &out) {
  out.print(F("DeviceID=")); out.println(F(DEVICE_ID));
  out.print(F("FirmwareVersion=")); out.println(F(FW_VERSION));
  out.print(F("BuildDate=")); out.print(F(__DATE__)); out.print(' '); out.println(F(__TIME__));
  out.print(F("BoardVoltage=")); out.println(F(BOARD_VOLTAGE));
  out.print(F("BoardClock=")); out.println(F(BOARD_CLOCK));
  out.print(F("PresetID=")); out.println(F(CONFIG_PRESET_ID));
  out.print(F("LoggingInterval_ms=")); out.println(LOG_INTERVAL_MS);
  out.print(F("EnvInterval_ms=")); out.println(SENSOR_ENV_INTERVAL_MS);
  out.print(F("MotionInterval_ms=")); out.println(SENSOR_MOTION_INTERVAL_MS);
  out.print(F("WaitForInitialSensorReads=")); out.println(LOG_WAIT_FOR_INITIAL_SENSOR_READS);
  out.print(F("RTC_Type="));
#if USE_RTC && RTC_TYPE_DS3231
  out.println(F("DS3231"));
#elif USE_RTC && RTC_TYPE_DS1307
  out.println(F("DS1307"));
#else
  out.println(F("NONE"));
#endif
  out.print(F("RTC_Available=")); out.println(rtcAvailable() ? 1 : 0);
  out.print(F("StartTime=")); printStartTime(out); out.println();
  out.print(F("SD_CS_Pin=")); out.println(SD_CS_PIN);
  out.print(F("Battery_R1_ohms=")); out.println(BATTERY_R1_OHMS);
  out.print(F("Battery_R2_ohms=")); out.println(BATTERY_R2_OHMS);
  out.println(F("EnabledSensors="));
#if USE_BMP280
  out.println(F("BMP280"));
#endif
#if USE_BME280
  out.println(F("BME280"));
#endif
#if USE_BME680
  out.println(F("BME680"));
#endif
#if USE_GY21
  out.println(F("GY21_HTU21D_SI7021"));
#endif
#if USE_MPU6050
  out.println(F("MPU6050"));
#endif
#if USE_GPS
  out.println(F("GPS_NEO"));
#endif
}

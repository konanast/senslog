#include "Sensors.h"
#include "Config.h"
#include "Status.h"
#include "Health.h"
#include <Wire.h>

#if USE_BMP280
#include <Adafruit_BMP280.h>
static Adafruit_BMP280 bmp;
static bool bmpOk = false;
#endif
#if USE_BME280
#include <Adafruit_BME280.h>
static Adafruit_BME280 bme;
static bool bmeOk = false;
#endif
#if USE_BME680
#include <Adafruit_BME680.h>
static Adafruit_BME680 bme680;
static bool bme680Ok = false;
#endif
#if USE_GY21
#include <Adafruit_HTU21DF.h>
static Adafruit_HTU21DF gy21;
static bool gy21Ok = false;
#endif
#if USE_MPU6050
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
static Adafruit_MPU6050 mpu;
static bool mpuOk = false;
#endif
#if USE_ADXL345
#include <Adafruit_ADXL345_U.h>
#include <Adafruit_Sensor.h>
static Adafruit_ADXL345_Unified adxl = Adafruit_ADXL345_Unified(345);
static bool adxlOk = false;
#endif
#if USE_GPS
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>
#ifndef GPS_RX_PIN
#define GPS_RX_PIN 4
#endif
#ifndef GPS_TX_PIN
#define GPS_TX_PIN 5
#endif
static TinyGPSPlus gps;
static SoftwareSerial gpsSerial(GPS_RX_PIN, GPS_TX_PIN);
#endif

static unsigned long s_lastEnvMs = 0;
static unsigned long s_lastMotionMs = 0;
static float s_tempC = NAN, s_rh = NAN, s_pressureHpa = NAN, s_altM = NAN, s_dewC = NAN, s_gasOhm = NAN;
static bool s_tempValid = false, s_rhValid = false, s_pressureValid = false, s_altValid = false, s_dewValid = false, s_gasValid = false;
static float s_ax = NAN, s_ay = NAN, s_az = NAN, s_gx = NAN, s_gy = NAN, s_gz = NAN;
static bool s_motionValid = false;
static uint16_t s_mqAdc = 0, s_mqMv = 0;
static bool s_mqValid = false;

#if USE_WIND_SPEED
static volatile unsigned long s_windPulses = 0;
static unsigned long s_lastWindMs = 0;
static float s_windMs = NAN;
static bool s_windValid = false;
static void windIsr() { s_windPulses++; }
#endif
#if USE_RAIN
static volatile unsigned long s_rainTips = 0;
static float s_rainMm = 0;
static bool s_rainValid = false;
static void rainIsr() { s_rainTips++; }
#endif
#if USE_WIND_DIR
static float s_windDirDeg = NAN;
static bool s_windDirValid = false;
#endif

static float dewPoint(float t, float rh) {
  if (!(rh > 0.0f && rh <= 100.0f) || !isfinite(t)) return NAN;
  const float a = 17.62f, b = 243.12f;
  float gamma = log(rh / 100.0f) + (a * t) / (b + t);
  return (b * gamma) / (a - gamma);
}

static void setEnvInvalid(uint16_t bit) {
  g_sensorStatus |= bit;
  g_invalidReadingCount++;
}

void sensorsBegin() {
#if USE_BMP280
  bmpOk = bmp.begin(0x76) || bmp.begin(0x77);
  if (!bmpOk) setEnvInvalid(STATUS_SENSOR_ENV);
#endif
#if USE_BME280
  bmeOk = bme.begin(0x76) || bme.begin(0x77);
  if (!bmeOk) setEnvInvalid(STATUS_SENSOR_ENV);
#endif
#if USE_BME680
  bme680Ok = bme680.begin(0x76) || bme680.begin(0x77);
  if (bme680Ok) {
    bme680.setTemperatureOversampling(BME680_OS_8X);
    bme680.setHumidityOversampling(BME680_OS_2X);
    bme680.setPressureOversampling(BME680_OS_4X);
    bme680.setGasHeater(320, 150);
  } else setEnvInvalid(STATUS_SENSOR_ENV);
#endif
#if USE_GY21
  gy21Ok = gy21.begin();
  if (!gy21Ok) setEnvInvalid(STATUS_SENSOR_ENV);
#endif
#if USE_MPU6050
  mpuOk = mpu.begin();
  if (!mpuOk) setEnvInvalid(STATUS_SENSOR_MOTION);
#endif
#if USE_ADXL345
  adxlOk = adxl.begin();
  if (!adxlOk) setEnvInvalid(STATUS_SENSOR_MOTION);
#endif
#if USE_GPS
  gpsSerial.begin(9600);
#endif
#if USE_WIND_SPEED
  pinMode(WIND_SPEED_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(WIND_SPEED_PIN), windIsr, FALLING);
  s_lastWindMs = millis();
#endif
#if USE_RAIN
  pinMode(RAIN_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RAIN_PIN), rainIsr, FALLING);
#endif
}

void sensorsPollFast() {
#if USE_GPS
  while (gpsSerial.available()) gps.encode(gpsSerial.read());
#endif
}

static void updateEnvironment() {
#if USE_BMP280
  if (bmpOk) {
    s_tempC = bmp.readTemperature();
    s_pressureHpa = bmp.readPressure() / 100.0f;
    s_altM = bmp.readAltitude(SEA_LEVEL_PRESSURE_HPA);
    s_tempValid = isfinite(s_tempC); s_pressureValid = isfinite(s_pressureHpa); s_altValid = isfinite(s_altM);
  }
#elif USE_BME280
  if (bmeOk) {
    s_tempC = bme.readTemperature(); s_rh = bme.readHumidity(); s_pressureHpa = bme.readPressure() / 100.0f; s_altM = bme.readAltitude(SEA_LEVEL_PRESSURE_HPA); s_dewC = dewPoint(s_tempC, s_rh);
    s_tempValid = isfinite(s_tempC); s_rhValid = isfinite(s_rh); s_pressureValid = isfinite(s_pressureHpa); s_altValid = isfinite(s_altM); s_dewValid = isfinite(s_dewC);
  }
#elif USE_BME680
  if (bme680Ok && bme680.performReading()) {
    s_tempC = bme680.temperature; s_rh = bme680.humidity; s_pressureHpa = bme680.pressure / 100.0f; s_altM = bme680.readAltitude(SEA_LEVEL_PRESSURE_HPA); s_dewC = dewPoint(s_tempC, s_rh); s_gasOhm = bme680.gas_resistance;
    s_tempValid = isfinite(s_tempC); s_rhValid = isfinite(s_rh); s_pressureValid = isfinite(s_pressureHpa); s_altValid = isfinite(s_altM); s_dewValid = isfinite(s_dewC); s_gasValid = isfinite(s_gasOhm);
  }
#elif USE_GY21
  if (gy21Ok) {
    s_tempC = gy21.readTemperature(); s_rh = gy21.readHumidity(); s_dewC = dewPoint(s_tempC, s_rh);
    s_tempValid = isfinite(s_tempC); s_rhValid = isfinite(s_rh); s_dewValid = isfinite(s_dewC);
  }
#endif
#if USE_ANALOG_MQ
  s_mqAdc = analogRead(A2);
  s_mqMv = (uint16_t)(((unsigned long)s_mqAdc * readVccMillivolts()) / 1023UL);
  s_mqValid = true;
#endif
}

static void updateMotion() {
#if USE_MPU6050
  if (mpuOk) { sensors_event_t a, g, temp; mpu.getEvent(&a, &g, &temp); s_ax = a.acceleration.x; s_ay = a.acceleration.y; s_az = a.acceleration.z; s_gx = g.gyro.x * 57.2958f; s_gy = g.gyro.y * 57.2958f; s_gz = g.gyro.z * 57.2958f; s_motionValid = true; }
#elif USE_ADXL345
  if (adxlOk) { sensors_event_t e; adxl.getEvent(&e); s_ax = e.acceleration.x; s_ay = e.acceleration.y; s_az = e.acceleration.z; s_motionValid = true; }
#endif
}

void sensorsUpdate() {
  unsigned long now = millis();
  if (now - s_lastEnvMs >= SENSOR_ENV_INTERVAL_MS) { s_lastEnvMs = now; updateEnvironment(); }
  if (now - s_lastMotionMs >= SENSOR_MOTION_INTERVAL_MS) { s_lastMotionMs = now; updateMotion(); }
#if USE_WIND_SPEED
  if (now - s_lastWindMs >= LOG_INTERVAL_MS) { noInterrupts(); unsigned long p = s_windPulses; s_windPulses = 0; interrupts(); s_windMs = (float)p * WIND_MS_PER_PULSE * 1000.0f / (float)(now - s_lastWindMs); s_lastWindMs = now; s_windValid = true; }
#endif
#if USE_WIND_DIR
  s_windDirDeg = ((float)analogRead(WIND_DIR_PIN) * 360.0f) / 1023.0f; s_windDirValid = true;
#endif
#if USE_RAIN
  noInterrupts(); unsigned long tips = s_rainTips; s_rainTips = 0; interrupts(); if (tips) { s_rainMm += tips * RAIN_MM_PER_TIP; s_rainValid = true; }
#endif
}

bool envHasTemp() { return s_tempValid; } float envTempC() { return s_tempC; }
bool envHasHumidity() { return s_rhValid; } float envHumidityPct() { return s_rh; }
bool envHasPressure() { return s_pressureValid; } float envPressureHpa() { return s_pressureHpa; }
bool envHasAltitude() { return s_altValid; } float envAltitudeM() { return s_altM; }
bool envHasDewPoint() { return s_dewValid; } float envDewPointC() { return s_dewC; }
bool envHasGasResistance() { return s_gasValid; } float envGasResistanceOhm() { return s_gasOhm; }
bool motionValid() { return s_motionValid; } float accelX() { return s_ax; } float accelY() { return s_ay; } float accelZ() { return s_az; } float gyroX() { return s_gx; } float gyroY() { return s_gy; } float gyroZ() { return s_gz; }
bool gpsValid() {
#if USE_GPS
  return gps.location.isValid();
#else
  return false;
#endif
}
float gpsLat() {
#if USE_GPS
  return gps.location.lat();
#else
  return NAN;
#endif
}
float gpsLon() {
#if USE_GPS
  return gps.location.lng();
#else
  return NAN;
#endif
}
float gpsAltM() {
#if USE_GPS
  return gps.altitude.meters();
#else
  return NAN;
#endif
}
float gpsSpeedKmh() {
#if USE_GPS
  return gps.speed.kmph();
#else
  return NAN;
#endif
}
float gpsCourseDeg() {
#if USE_GPS
  return gps.course.deg();
#else
  return NAN;
#endif
}
uint8_t gpsSatellites() {
#if USE_GPS
  return gps.satellites.value();
#else
  return 0;
#endif
}
float gpsHdop() {
#if USE_GPS
  return gps.hdop.hdop();
#else
  return NAN;
#endif
}
uint32_t gpsFixAgeMs() {
#if USE_GPS
  return gps.location.age();
#else
  return 0;
#endif
}
bool windSpeedValid() {
#if USE_WIND_SPEED
  return s_windValid;
#else
  return false;
#endif
}
float windSpeedMs() {
#if USE_WIND_SPEED
  return s_windMs;
#else
  return NAN;
#endif
}

bool windDirectionValid() {
#if USE_WIND_DIR
  return s_windDirValid;
#else
  return false;
#endif
}
float windDirectionDeg() {
#if USE_WIND_DIR
  return s_windDirDeg;
#else
  return NAN;
#endif
}

bool rainValid() {
#if USE_RAIN
  return s_rainValid;
#else
  return false;
#endif
}
float rainfallMm() {
#if USE_RAIN
  return s_rainMm;
#else
  return NAN;
#endif
}
bool mqValid() { return s_mqValid; } uint16_t mqAdc() { return s_mqAdc; } uint16_t mqVoltageMv() { return s_mqMv; }

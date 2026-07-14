/*
  ModularLogger.ino - Flexible Arduino Pro Mini data logger

  Target: Arduino Pro Mini (ATmega328P, 5 V or 3.3 V variants)
  Required hardware: microSD module
  Optional hardware: DS3231/DS1307 RTC, environmental sensors, accelerometer,
  GPS, analogue voltage inputs, and other future sensors.

  Design summary:
  - Compile-time feature flags enable only the sensors used by a hardware build.
  - Each output column is represented by a small Measurement descriptor stored in
    flash (PROGMEM). The descriptor contains a CSV header and a callback that
    prints exactly one field.
  - The logger prints the header and each row by walking the same descriptor
    table, so the data column order always matches the header order.
  - No heap-backed Arduino text objects or heap allocation are used.
*/

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

// Uncomment the libraries matching your hardware and set the feature flags below.
// #include <RTClib.h>                 // Adafruit RTClib: DS3231/DS1307
// #include <Adafruit_BME280.h>        // BME280 temperature/RH/pressure
// #include <DHT.h>                    // DHT22/DHT11 fallback example
// #include <Adafruit_ADXL345_U.h>     // ADXL345 accelerometer
// #include <TinyGPSPlus.h>            // GPS parser
// #include <SoftwareSerial.h>         // GPS on Pro Mini without spare UART

// -----------------------------------------------------------------------------
// Hardware configuration
// -----------------------------------------------------------------------------
#define SD_CS_PIN 10
#define LOG_FILE_PREFIX "LOG"
#define LOG_FILE_EXTENSION ".CSV"
#define LOG_INTERVAL_MS 10000UL
#define FLUSH_EVERY_N_ROWS 6

// Set these flags for each hardware variant. See the examples below this file.
#define USE_RTC 1
#define USE_BME280 1
#define USE_DHT 0
#define USE_ANALOG_MV 1
#define USE_ADXL345 0
#define USE_GPS 0
#define USE_WIND 0

// Sensor-specific settings.
#define BME280_ADDRESS 0x76
#define DHT_PIN 2
#define DHT_TYPE DHT22
#define ANALOG_MV_PIN A0
#define ANALOG_REFERENCE_MV 3300UL
#define ANALOG_DIVIDER_NUMERATOR 1UL    // measured voltage = ADC mV * numerator / denominator
#define ANALOG_DIVIDER_DENOMINATOR 1UL
#define GPS_RX_PIN 4                    // Arduino receives from GPS TX
#define GPS_TX_PIN 3                    // Arduino transmits to GPS RX, often unused
#define WIND_PIN 2
#define WIND_MS_PER_PULSE 0.667f        // example calibration placeholder

// -----------------------------------------------------------------------------
// Optional device objects. Keep globals static to avoid heap usage.
// -----------------------------------------------------------------------------
#if USE_RTC
#include <RTClib.h>
RTC_DS3231 rtc;
static bool rtcOk = false;
#endif

#if USE_BME280
#include <Adafruit_BME280.h>
Adafruit_BME280 bme;
static bool bmeOk = false;
#endif

#if USE_DHT
#include <DHT.h>
DHT dht(DHT_PIN, DHT_TYPE);
static bool dhtOk = false;
#endif

#if USE_ADXL345
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
static bool accelOk = false;
#endif

#if USE_GPS
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>
TinyGPSPlus gps;
SoftwareSerial gpsSerial(GPS_RX_PIN, GPS_TX_PIN);
static bool gpsOk = false;
#endif

#if USE_WIND
static volatile unsigned long windPulses = 0;
static bool windOk = false;
void windIsr() { windPulses++; }
#endif

// -----------------------------------------------------------------------------
// Runtime state
// -----------------------------------------------------------------------------
static File logFile;
static char logFileName[13]; // 8.3 filename plus NUL, e.g. LOG000.CSV
static unsigned long nextSampleMs = 0;
static uint16_t rowsSinceFlush = 0;
static bool sdOk = false;

struct SampleCache {
  bool envValid;
  float tempC;
  float rh;
  float pressureKPa;
  float altitudeM;
  float dewPointC;

  bool analogValid;
  uint16_t milliVolts;

  bool accelValid;
  float ax;
  float ay;
  float az;

  bool gpsValid;
  float gpsLat;
  float gpsLon;
  float gpsAltM;
  float gpsSpeedKph;

  bool windValid;
  float windMs;
};

static SampleCache sample;

typedef void (*PrintValueFn)(Print &out);

struct Measurement {
  const char *header;       // points to flash string literal via PSTR/F()
  PrintValueFn printValue;  // prints one CSV field, or an empty field if invalid
};

// -----------------------------------------------------------------------------
// Small printing helpers
// -----------------------------------------------------------------------------
static void printFlashText(Print &out, const char *p) {
  char c;
  while ((c = pgm_read_byte(p++)) != 0) {
    out.write(c);
  }
}

static void printFloatOrBlank(Print &out, bool valid, float value, uint8_t decimals) {
  if (valid && isfinite(value)) {
    out.print(value, decimals);
  }
}

static void printUIntOrBlank(Print &out, bool valid, uint16_t value) {
  if (valid) {
    out.print(value);
  }
}

static float dewPointMagnus(float tempC, float rh) {
  if (!(rh > 0.0f && rh <= 100.0f) || !isfinite(tempC)) {
    return NAN;
  }
  const float a = 17.62f;
  const float b = 243.12f;
  const float gamma = log(rh / 100.0f) + (a * tempC) / (b + tempC);
  return (b * gamma) / (a - gamma);
}

// -----------------------------------------------------------------------------
// Timestamp columns
// -----------------------------------------------------------------------------
static void printTime(Print &out) {
#if USE_RTC
  if (rtcOk) {
    DateTime now = rtc.now();
    uint8_t hour12 = now.hour() % 12;
    if (hour12 == 0) hour12 = 12;
    if (hour12 < 10) out.print('0');
    out.print(hour12);
    out.print(':');
    if (now.minute() < 10) out.print('0');
    out.print(now.minute());
    out.print(':');
    if (now.second() < 10) out.print('0');
    out.print(now.second());
    out.print(now.hour() < 12 ? F(" AM") : F(" PM"));
    return;
  }
#endif
  out.print(millis() / 1000UL);
}

static void printDate(Print &out) {
#if USE_RTC
  if (rtcOk) {
    DateTime now = rtc.now();
    if (now.month() < 10) out.print('0');
    out.print(now.month());
    out.print('/');
    if (now.day() < 10) out.print('0');
    out.print(now.day());
    out.print('/');
    uint8_t yy = now.year() % 100;
    if (yy < 10) out.print('0');
    out.print(yy);
    return;
  }
#endif
  out.print(F("uptime_s"));
}

// -----------------------------------------------------------------------------
// Measurement callback functions
// -----------------------------------------------------------------------------
static void printTempC(Print &out)      { printFloatOrBlank(out, sample.envValid, sample.tempC, 2); }
static void printRH(Print &out)         { printFloatOrBlank(out, sample.envValid, sample.rh, 2); }
static void printPressure(Print &out)   { printFloatOrBlank(out, sample.envValid, sample.pressureKPa, 2); }
static void printAltitude(Print &out)   { printFloatOrBlank(out, sample.envValid, sample.altitudeM, 2); }
static void printDewPoint(Print &out)   { printFloatOrBlank(out, sample.envValid, sample.dewPointC, 2); }
static void printMilliVolts(Print &out) { printUIntOrBlank(out, sample.analogValid, sample.milliVolts); }
static void printAccelX(Print &out)     { printFloatOrBlank(out, sample.accelValid, sample.ax, 3); }
static void printAccelY(Print &out)     { printFloatOrBlank(out, sample.accelValid, sample.ay, 3); }
static void printAccelZ(Print &out)     { printFloatOrBlank(out, sample.accelValid, sample.az, 3); }
static void printGpsLat(Print &out)     { printFloatOrBlank(out, sample.gpsValid, sample.gpsLat, 6); }
static void printGpsLon(Print &out)     { printFloatOrBlank(out, sample.gpsValid, sample.gpsLon, 6); }
static void printGpsAlt(Print &out)     { printFloatOrBlank(out, sample.gpsValid, sample.gpsAltM, 1); }
static void printGpsSpeed(Print &out)   { printFloatOrBlank(out, sample.gpsValid, sample.gpsSpeedKph, 1); }
static void printWind(Print &out)       { printFloatOrBlank(out, sample.windValid, sample.windMs, 2); }

// Store headers in flash. Each enabled callback is compiled into the table.
const char hTime[] PROGMEM = "Time";
const char hDate[] PROGMEM = "Date";
const char hTempC[] PROGMEM = "TempC";
const char hRH[] PROGMEM = "RH%";
const char hPressure[] PROGMEM = "Pressure_kPa";
const char hAltitude[] PROGMEM = "Altitude_m";
const char hDewPoint[] PROGMEM = "DewPoint_C";
const char hMV[] PROGMEM = "mV";
const char hAccelX[] PROGMEM = "AccelX_ms2";
const char hAccelY[] PROGMEM = "AccelY_ms2";
const char hAccelZ[] PROGMEM = "AccelZ_ms2";
const char hGpsLat[] PROGMEM = "GPS_Lat";
const char hGpsLon[] PROGMEM = "GPS_Lon";
const char hGpsAlt[] PROGMEM = "GPS_Alt_m";
const char hGpsSpeed[] PROGMEM = "GPS_Speed_kph";
const char hWind[] PROGMEM = "Wind_ms";

const Measurement measurements[] PROGMEM = {
  { hTime, printTime },
  { hDate, printDate },
#if USE_BME280 || USE_DHT
  { hTempC, printTempC },
  { hRH, printRH },
#endif
#if USE_BME280
  { hPressure, printPressure },
  { hAltitude, printAltitude },
#endif
#if USE_BME280 || USE_DHT
  { hDewPoint, printDewPoint },
#endif
#if USE_ANALOG_MV
  { hMV, printMilliVolts },
#endif
#if USE_ADXL345
  { hAccelX, printAccelX },
  { hAccelY, printAccelY },
  { hAccelZ, printAccelZ },
#endif
#if USE_GPS
  { hGpsLat, printGpsLat },
  { hGpsLon, printGpsLon },
  { hGpsAlt, printGpsAlt },
  { hGpsSpeed, printGpsSpeed },
#endif
#if USE_WIND
  { hWind, printWind },
#endif
};
const uint8_t MEASUREMENT_COUNT = sizeof(measurements) / sizeof(measurements[0]);

static Measurement readMeasurement(uint8_t index) {
  Measurement m;
  memcpy_P(&m, &measurements[index], sizeof(m));
  return m;
}

// -----------------------------------------------------------------------------
// Initialisation
// -----------------------------------------------------------------------------
static void makeLogFileName() {
  for (uint16_t n = 0; n < 1000; n++) {
    snprintf(logFileName, sizeof(logFileName), "%s%03u%s", LOG_FILE_PREFIX, n, LOG_FILE_EXTENSION);
    if (!SD.exists(logFileName)) {
      return;
    }
  }
  strncpy(logFileName, "LOG999.CSV", sizeof(logFileName));
  logFileName[sizeof(logFileName) - 1] = '\0';
}

static void writeHeader(Print &out) {
  for (uint8_t i = 0; i < MEASUREMENT_COUNT; i++) {
    if (i) out.print(',');
    Measurement m = readMeasurement(i);
    printFlashText(out, m.header);
  }
  out.println();
}

static bool beginStorage() {
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println(F("ERROR: SD card init failed"));
    return false;
  }
  makeLogFileName();
  logFile = SD.open(logFileName, FILE_WRITE);
  if (!logFile) {
    Serial.println(F("ERROR: log file open failed"));
    return false;
  }
  writeHeader(logFile);
  logFile.flush();
  Serial.print(F("Logging to "));
  Serial.println(logFileName);
  return true;
}

static void beginRtc() {
#if USE_RTC
  rtcOk = rtc.begin();
  if (!rtcOk) {
    Serial.println(F("WARN: RTC unavailable; using uptime seconds"));
    return;
  }
  if (rtc.lostPower()) {
    Serial.println(F("WARN: RTC lost power; set clock before deployment"));
  }
#else
  Serial.println(F("INFO: RTC disabled; using uptime seconds"));
#endif
}

static void beginSensors() {
#if USE_BME280
  bmeOk = bme.begin(BME280_ADDRESS);
  Serial.println(bmeOk ? F("BME280 ready") : F("WARN: BME280 unavailable"));
#endif
#if USE_DHT
  dht.begin();
  dhtOk = true; // DHT library has no reliable begin status; reads validate samples.
  Serial.println(F("DHT enabled"));
#endif
#if USE_ADXL345
  accelOk = accel.begin();
  if (accelOk) accel.setRange(ADXL345_RANGE_16_G);
  Serial.println(accelOk ? F("ADXL345 ready") : F("WARN: ADXL345 unavailable"));
#endif
#if USE_GPS
  gpsSerial.begin(9600);
  gpsOk = true;
  Serial.println(F("GPS serial enabled"));
#endif
#if USE_WIND
  pinMode(WIND_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(WIND_PIN), windIsr, FALLING);
  windOk = true;
  Serial.println(F("Wind input enabled"));
#endif
}

// -----------------------------------------------------------------------------
// Sampling
// -----------------------------------------------------------------------------
static void pollFastSensors() {
#if USE_GPS
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }
#endif
}

static void readSensors() {
  memset(&sample, 0, sizeof(sample));

#if USE_BME280
  if (bmeOk) {
    sample.tempC = bme.readTemperature();
    sample.rh = bme.readHumidity();
    sample.pressureKPa = bme.readPressure() / 1000.0f;
    sample.altitudeM = bme.readAltitude(1013.25f);
    sample.dewPointC = dewPointMagnus(sample.tempC, sample.rh);
    sample.envValid = isfinite(sample.tempC) && isfinite(sample.rh) && isfinite(sample.pressureKPa);
  }
#elif USE_DHT
  if (dhtOk) {
    sample.tempC = dht.readTemperature();
    sample.rh = dht.readHumidity();
    sample.dewPointC = dewPointMagnus(sample.tempC, sample.rh);
    sample.envValid = isfinite(sample.tempC) && isfinite(sample.rh);
  }
#endif

#if USE_ANALOG_MV
  uint16_t raw = analogRead(ANALOG_MV_PIN);
  unsigned long mv = ((unsigned long)raw * ANALOG_REFERENCE_MV * ANALOG_DIVIDER_NUMERATOR) /
                     (1023UL * ANALOG_DIVIDER_DENOMINATOR);
  sample.milliVolts = (mv > 65535UL) ? 65535U : (uint16_t)mv;
  sample.analogValid = true;
#endif

#if USE_ADXL345
  if (accelOk) {
    sensors_event_t event;
    accel.getEvent(&event);
    sample.ax = event.acceleration.x;
    sample.ay = event.acceleration.y;
    sample.az = event.acceleration.z;
    sample.accelValid = isfinite(sample.ax) && isfinite(sample.ay) && isfinite(sample.az);
  }
#endif

#if USE_GPS
  sample.gpsValid = gpsOk && gps.location.isValid();
  if (sample.gpsValid) {
    sample.gpsLat = gps.location.lat();
    sample.gpsLon = gps.location.lng();
    sample.gpsAltM = gps.altitude.isValid() ? gps.altitude.meters() : NAN;
    sample.gpsSpeedKph = gps.speed.isValid() ? gps.speed.kmph() : NAN;
  }
#endif

#if USE_WIND
  if (windOk) {
    noInterrupts();
    unsigned long pulses = windPulses;
    windPulses = 0;
    interrupts();
    sample.windMs = ((float)pulses * WIND_MS_PER_PULSE * 1000.0f) / (float)LOG_INTERVAL_MS;
    sample.windValid = true;
  }
#endif
}

static void writeRow(Print &out) {
  for (uint8_t i = 0; i < MEASUREMENT_COUNT; i++) {
    if (i) out.print(',');
    Measurement m = readMeasurement(i);
    m.printValue(out);
  }
  out.println();
}

static void logSample() {
  if (!sdOk) return;
  readSensors();
  writeRow(logFile);
  rowsSinceFlush++;
  if (rowsSinceFlush >= FLUSH_EVERY_N_ROWS) {
    logFile.flush();
    rowsSinceFlush = 0;
  }
}

void setup() {
  Serial.begin(9600);
  Wire.begin();
  Serial.println(F("Modular data logger starting"));
  beginRtc();
  beginSensors();
  sdOk = beginStorage();
  nextSampleMs = millis();
}

void loop() {
  pollFastSensors();
  const unsigned long now = millis();
  if ((long)(now - nextSampleMs) >= 0) {
    logSample();
    nextSampleMs += LOG_INTERVAL_MS;
    if ((long)(millis() - nextSampleMs) >= 0) {
      nextSampleMs = millis() + LOG_INTERVAL_MS;
    }
  }
}

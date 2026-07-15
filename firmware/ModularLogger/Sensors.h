#ifndef MODULAR_LOGGER_SENSORS_H
#define MODULAR_LOGGER_SENSORS_H
#include <Arduino.h>

void sensorsBegin();
void sensorsPollFast();
void sensorsUpdate();

bool envHasTemp(); float envTempC();
bool envHasHumidity(); float envHumidityPct();
bool envHasPressure(); float envPressureHpa();
bool envHasAltitude(); float envAltitudeM();
bool envHasDewPoint(); float envDewPointC();
bool envHasGasResistance(); float envGasResistanceOhm();

bool motionValid(); float accelX(); float accelY(); float accelZ(); float gyroX(); float gyroY(); float gyroZ();
bool gpsValid(); float gpsLat(); float gpsLon(); float gpsAltM(); float gpsSpeedKmh(); float gpsCourseDeg(); uint8_t gpsSatellites(); float gpsHdop(); uint32_t gpsFixAgeMs();
bool windSpeedValid(); float windSpeedMs();
bool windDirectionValid(); float windDirectionDeg();
bool rainValid(); float rainfallMm();
bool mqValid(); uint16_t mqAdc(); uint16_t mqVoltageMv();

#endif

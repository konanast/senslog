# senslog

Production-oriented Arduino Pro Mini modular data logger firmware. The project targets small AVR boards with limited SRAM and flash, a mandatory microSD module, optional DS3231/DS1307 RTC, and compile-time-selected sensor presets.

## Recommended architecture

The firmware uses a low-memory, compile-time modular architecture:

- `Config.h` selects one preset and enables only the required sensor adapters.
- `presets/*.h` provide ready-to-build configurations.
- `Sensors.*` owns sensor-specific code and independent update intervals.
- `Timestamp.*` provides DS3231, DS1307, or no-RTC uptime timestamps.
- `Logger.*` contains the measurement descriptor table and writes CSV directly to the SD file.
- `SDFileManager.*` creates sequential files such as `LOG0000.CSV`, writes `INFO.TXT`, and provides safe flush/close helpers.
- `Health.*` reports `Vcc_mV`, battery voltage, free RAM, reset cause, and subsystem counters.

The central CSV logger does not know sensor-specific details. It iterates a `PROGMEM` measurement descriptor table for both the header and every data row, keeping column order identical while avoiding dynamic allocation and Arduino `String`.

## Project structure

```text
firmware/ModularLogger/
  ModularLogger.ino
  Config.h
  Logger.h / Logger.cpp
  SDFileManager.h / SDFileManager.cpp
  Timestamp.h / Timestamp.cpp
  Sensors.h / Sensors.cpp
  Health.h / Health.cpp
  Status.h / Status.cpp
  SENSOR_PRESETS.md
  README_BUILD.md
  presets/
    Preset_BMP280.h
    Preset_BME280.h
    Preset_BME680.h
    Preset_GY21.h
    Preset_Mobile_BME280.h
    Preset_Mobile_BME680.h
    Preset_WeatherStation.h
```

## Required presets

Select a preset by editing `LOGGER_PRESET` in `firmware/ModularLogger/Config.h`:

| Preset | Purpose |
| --- | --- |
| `PRESET_BMP280` | Temperature, pressure, altitude logger. |
| `PRESET_BME280` | Temperature, humidity, pressure, altitude, dew point logger. |
| `PRESET_BME680` | BME280-style fields plus gas resistance. |
| `PRESET_GY21` | GY-21 / HTU21D / compatible Si7021 temperature-humidity logger. |
| `PRESET_MOBILE_BME280` | BME280, MPU6050, GPS, battery monitoring, SD, optional RTC. |
| `PRESET_MOBILE_BME680` | BME680, MPU6050, GPS, battery monitoring, SD, optional RTC. |
| `PRESET_WEATHER_STATION` | BME280, pulse anemometer, analogue wind direction, rain gauge, battery monitoring, optional RTC. |

## Target hardware and wiring

Supported boards:

- Arduino Pro Mini 3.3 V / 8 MHz.
- Arduino Pro Mini 5 V / 16 MHz.

Use the 3.3 V board when possible because microSD cards are 3.3 V devices. Some low-cost SD modules include regulators or level shifters, but many do not safely tolerate 5 V logic. Confirm the module before connecting it to a 5 V Pro Mini.

The low-cost SPI microSD module wiring and 4-pin RTC wiring are documented in `firmware/ModularLogger/SENSOR_PRESETS.md`. The microSD CS pin is configurable with `SD_CS_PIN` in `Config.h`.

## Measurements

Depending on enabled preset flags, the CSV may include:

`Date`, `Time`, `TimestampSource`, `Record_ID`, `Boot_ID`, `Uptime_s`, `SampleInterval_ms`, `Temp_C`, `RH_pct`, `Pressure_hPa`, `Altitude_m`, `DewPoint_C`, `GasResistance_ohm`, `GasADC`, `GasVoltage_mV`, `AccelX_mps2`, `AccelY_mps2`, `AccelZ_mps2`, `GyroX_dps`, `GyroY_dps`, `GyroZ_dps`, `Latitude`, `Longitude`, `GPSAltitude_m`, `Speed_kmh`, `Course_deg`, `Satellites`, `HDOP`, `GPSFixValid`, `GPSFixAge_ms`, `WindSpeed_ms`, `WindDirection_deg`, `Rainfall_mm`, `Vcc_mV`, `Battery_mV`, `Battery_pct`, `FreeRAM_B`, `ResetCause`, `SensorStatus`, `RTC_Status`, `SD_Status`, `SD_ErrorCount`, `InvalidReadingCount`, `LoopTime_ms`, and `SD_WriteTime_ms`.

Invalid, timed-out, missing, or unavailable readings are emitted as empty fields by default. Set `USE_EMPTY_FOR_INVALID 0` in `Config.h` if you prefer `NA`.

## Metadata

Each boot appends static configuration metadata to `INFO.TXT`, including device ID, firmware version, build date, board voltage/clock label, preset ID, enabled sensors, logging interval, sensor intervals, RTC type/availability, start time, SD CS pin, and battery divider values. Static metadata is intentionally not repeated in every CSV row.

## Battery and supply monitoring

`Vcc_mV` is measured from the AVR internal 1.1 V reference where supported. This is the MCU supply voltage, not battery voltage.

`Battery_mV` is measured separately through a divider:

```text
Battery+ -> R1 -> ADC pin -> R2 -> GND
Battery_mV = ADC_mV * (R1 + R2) / R2
```

Configure `BATTERY_R1_OHMS`, `BATTERY_R2_OHMS`, and `BATTERY_ADC_REF_MV` in `Config.h`. Keep the ADC pin voltage below the board Vcc. `Battery_pct` is optional because it depends on battery chemistry and calibration.

## Build instructions

See `firmware/ModularLogger/README_BUILD.md` for the required library list, preset selection, and basic test procedure. Install the Arduino `SD` library for every build (`arduino-cli lib install "SD"`), then install only the optional sensor libraries required by the selected preset; do not compile every optional library into every build.

## Adding a sensor or CSV field

1. Add a compile-time flag to `Config.h` and, if useful, to one preset file.
2. Add the sensor object and update function to `Sensors.cpp` behind `#if YOUR_FLAG`.
3. Expose a lightweight getter in `Sensors.h`.
4. Add a header string and callback to the descriptor table in `Logger.cpp` behind the same flag.
5. Do not modify `loggerWriteHeader()` or `loggerWriteRow()`.

## Example CSV output

```text
Date,Time,TimestampSource,Record_ID,Boot_ID,Uptime_s,SampleInterval_ms,Temp_C,RH_pct,Pressure_hPa,Altitude_m,DewPoint_C,Vcc_mV,FreeRAM_B,ResetCause,SensorStatus,RTC_Status,SD_Status,SD_ErrorCount,InvalidReadingCount,LoopTime_ms,SD_WriteTime_ms
07/14/26,12:01:05,RTC,1,0,3,10000,24.21,44.50,1012.86,3.24,11.35,3290,812,1,0,0,0,0,0,4,18
```

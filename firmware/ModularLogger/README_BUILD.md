# ModularLogger build instructions

## Directory structure

- `ModularLogger.ino` - Arduino entry point and scheduler.
- `Config.h` - central compile-time configuration and preset selector.
- `presets/*.h` - ready-to-build preset configurations.
- `Logger.*` - CSV descriptor table, header writer, row writer, and metadata writer.
- `SDFileManager.*` - SD initialization, sequential `LOG0001.CSV` file creation, flush/close helpers, and `INFO.TXT` creation.
- `Timestamp.*` - DS3231, DS1307, and no-RTC uptime timestamp provider.
- `Sensors.*` - conditionally compiled sensor adapters.
- `Health.*` - Vcc, battery, free RAM, reset cause, and health counters.
- `Status.*` - subsystem status bitmasks and counters.
- `SENSOR_PRESETS.md` - wiring, voltage, power, and extension notes.

## Required libraries by preset

Always required: Arduino AVR core, built-in `Wire` and `SPI`, plus the Arduino `SD` library. If `#include <SD.h>` fails, install the SD library with `arduino-cli lib install "SD"`.

Minimal setup for every preset:

```bash
arduino-cli core update-index
arduino-cli core install arduino:avr
arduino-cli lib install "SD"
```

Optional libraries are compiled only when the related preset flag is enabled:

- RTC: Adafruit `RTClib` for DS3231 or DS1307.
- BMP280: Adafruit `Adafruit_BMP280`.
- BME280: Adafruit `Adafruit_BME280`.
- BME680: Adafruit `Adafruit_BME680`.
- GY-21 / HTU21D / Si7021: Adafruit `Adafruit_HTU21DF`.
- MPU6050: Adafruit `Adafruit_MPU6050` and `Adafruit Unified Sensor`.
- ADXL345: Adafruit `Adafruit_ADXL345_U` and `Adafruit Unified Sensor`.
- GPS: `TinyGPSPlus` and Arduino `SoftwareSerial`.

## Select a preset

Edit `LOGGER_PRESET` in `Config.h`, or pass a build flag from your build system:

```cpp
#define LOGGER_PRESET PRESET_BME280
```

Available presets:

1. `PRESET_BMP280`
2. `PRESET_BME280`
3. `PRESET_BME680`
4. `PRESET_GY21`
5. `PRESET_MOBILE_BME280`
6. `PRESET_MOBILE_BME680`
7. `PRESET_WEATHER_STATION`

## RTC date/time setting

If a DS3231 reports lost power, a DS1307 is not running, or the RTC returns a year before 2020, the firmware normally falls back to uptime. This build now defaults `RTC_SET_ON_INVALID` to `1`, so an invalid connected RTC is automatically adjusted to the sketch compile time (`__DATE__` / `__TIME__`) during startup. This is convenient after flashing, but the value is the computer compile time, not an internet-synchronized clock. Recompile immediately before uploading for the best result.

Leave `RTC_SET_ON_EVERY_BOOT` at `0` for deployed loggers. Set it to `1` only for a one-time clock-setting upload, then set it back to `0` and upload again; otherwise every reset rewrites the RTC to the old compile time.

## Basic test procedure

1. Format the microSD card as FAT16/FAT32.
2. Select a preset in `Config.h`.
3. Install the always-required `SD` library plus only the optional sensor libraries needed by that preset.
4. If compilation fails with `fatal error: SD.h: No such file or directory`, run `arduino-cli lib install "SD"` and compile again.
5. Compile for `Arduino Pro or Pro Mini` with the correct processor option: `ATmega328P (3.3V, 8 MHz)` or `ATmega328P (5V, 16 MHz)`.
6. Upload with the SD card inserted and Serial Monitor at 9600 baud.
7. Confirm `INFO.TXT` and a sequential `LOG0000.CSV`/`LOG0001.CSV` file are created.
8. Confirm the CSV header columns exactly match the data columns.
9. Disconnect and reconnect power to verify the next sequential log file is created without overwriting existing files.

## Example CSV output

```text
Date,Time,TimestampSource,Record_ID,Boot_ID,Uptime_s,SampleInterval_ms,Temp_C,RH_pct,Pressure_hPa,Altitude_m,DewPoint_C,Vcc_mV,FreeRAM_B,ResetCause,SensorStatus,RTC_Status,SD_Status,SD_ErrorCount,InvalidReadingCount,LoopTime_ms,SD_WriteTime_ms
07/14/26,12:01:05,RTC,1,0,3,10000,24.21,44.50,1012.86,3.24,11.35,3290,812,1,0,0,0,0,0,4,18
07/14/26,12:01:15,RTC,2,0,13,10000,24.22,44.48,1012.84,3.41,11.35,3292,812,1,0,0,0,0,0,4,17
```

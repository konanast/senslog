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

Always required: the selected Arduino core, built-in `Wire` and `SPI`, plus an
Arduino-compatible `SD` library for that core. If `#include <SD.h>` fails,
install the library with `arduino-cli lib install "SD"`.

Minimal setup for the default Arduino Pro Mini build:

```bash
arduino-cli core update-index
arduino-cli core install arduino:avr
arduino-cli lib install "SD"
```

For ESP32-C6, ESP32-H2, RP2040, or RP2350, install the matching board core
instead of `arduino:avr`; the exact Fully Qualified Board Name (FQBN) depends
on the board package and vendor. Keep the required `SD` library compatible
with that core.

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

## Select the main board

Set `LOGGER_BOARD` in `Config.h` before compiling. The default is
`BOARD_PRO_MINI_3V3`; available values are `BOARD_ESP32_C6`,
`BOARD_ESP32_H2`, `BOARD_RP2040`, and `BOARD_RP2350`. This setting records the
board in `INFO.TXT` and supplies conservative 3.3 V battery-ADC defaults. It
does **not** select the SD pins or install a board core.

All listed boards use 3.3 V I/O. Connect the SD card through the board's
hardware SPI pins, set `SD_CS_PIN` to the GPIO used for CS, and check the
pinout of the exact board (for example, XIAO ESP32-C6 or Waveshare H2 Zero)
before wiring. The default CS value `10` is for the Arduino Pro Mini and must
be overridden for boards whose selected CS pin differs. Do not connect 5 V
logic to ESP32 or RP2040/RP2350 pins.

The AVR internal-bandgap `Vcc_mV` measurement is available only on the Pro
Mini/ATmega328P build. Other boards leave `Vcc_mV` empty; their regulator's
nominal voltage is metadata, not a measured supply value. `FreeRAM_B` is `-1`
when the selected core has no portable free-RAM implementation.

## Startup logging behaviour

`SENSOR_ENV_INTERVAL_MS` is the interval between environmental-sensor read
attempts; it is **not** a logging delay by itself.  Because the first attempt
is deliberately made after that interval, a build whose `LOG_INTERVAL_MS` is
shorter than `SENSOR_ENV_INTERVAL_MS` would otherwise write startup CSV rows
with empty environmental fields.

`LOG_WAIT_FOR_INITIAL_SENSOR_READS` in `Config.h` controls this behaviour and
defaults to `1`.  With `1`, the logger waits until each enabled periodic sensor
has made its first read attempt, then writes the first row immediately.  A
failed or disconnected sensor does not block logging forever: its attempt is
considered complete and its fields remain empty (or `NA`, when configured).
GPS is excluded from this startup gate because acquiring a satellite fix can
take much longer.  Set the option to `0` if you explicitly want to record the
startup period, including empty fields.

## RTC date/time setting

If a DS3231 reports lost power, a DS1307 is not running, or the RTC returns a year before 2020, the firmware normally falls back to uptime. This build now defaults `RTC_SET_ON_INVALID` to `1`, so an invalid connected RTC is automatically adjusted to the sketch compile time (`__DATE__` / `__TIME__`) during startup. This is convenient after flashing, but the value is the computer compile time, not an internet-synchronized clock. Recompile immediately before uploading for the best result.

Leave `RTC_SET_ON_EVERY_BOOT` at `0` for deployed loggers. Set it to `1` only for a one-time clock-setting upload, then set it back to `0` and upload again; otherwise every reset rewrites the RTC to the old compile time.

## Basic test procedure

1. Format the microSD card as FAT16/FAT32.
2. Select a preset in `Config.h`.
3. Install the always-required `SD` library plus only the optional sensor libraries needed by that preset.
4. If compilation fails with `fatal error: SD.h: No such file or directory`, run `arduino-cli lib install "SD"` and compile again.
5. Compile for the board selected by `LOGGER_BOARD`; for the default use `Arduino Pro or Pro Mini` / `ATmega328P (3.3V, 8 MHz)`. Use the matching ESP32-C6, ESP32-H2, RP2040, or RP2350 Arduino core for other selections.
6. Upload with the SD card inserted and Serial Monitor at 9600 baud.
7. Confirm `INFO.TXT` and a sequential `LOG0000.CSV`/`LOG0001.CSV` file are created.
8. Confirm the CSV header columns exactly match the data columns.
9. Disconnect and reconnect power to verify the next sequential log file is created without overwriting existing files.

## Example CSV output

```text
Date,Time,TimestampSource,Record_ID,Boot_ID,Uptime_s,SampleInterval_ms,Temp_C,RH_pct,Pressure_hPa,Altitude_m,DewPoint_C,Vcc_mV,FreeRAM_B,ResetCause,SensorStatus,RTC_Status,SD_Status,SD_ErrorCount,InvalidReadingCount,LoopTime_ms,SD_WriteTime_ms
07/14/26,12:01:10,RTC,1,0,10,1000,24.21,44.50,1012.86,3.24,11.35,3290,812,1,0,0,0,0,0,4,18
07/14/26,12:01:11,RTC,2,0,11,1000,24.22,44.48,1012.84,3.41,11.35,3292,812,1,0,0,0,0,0,4,17
```

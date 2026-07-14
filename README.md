# senslog

Flexible, modular Arduino Pro Mini data logger firmware for hardware variants that share a microSD logger but may use different sensor combinations.

## Recommended architecture

Use a compile-time configured table of measurement descriptors plus small sensor read functions:

| Option | Practical result on Arduino Pro Mini |
| --- | --- |
| One fixed struct per hardware variant | Fast and simple, but every variant needs custom logging code and CSV headers easily drift from row order. |
| Generic classes/interfaces for every sensor | Clean on larger boards, but virtual dispatch and object state add flash/SRAM overhead on ATmega328P. |
| Callback table of measurement descriptors | Recommended. A single logging loop prints the header and rows from the same table, while each callback prints one field. |
| Dynamic registry of sensors/measurements | Flexible, but heap allocation and `String` usage are risky on 2 KB SRAM devices. |
| Compile-time feature flags | Recommended with the callback table. Unused sensors, libraries, columns, and buffers are removed from the build. |

The firmware in [`firmware/ModularLogger/ModularLogger.ino`](firmware/ModularLogger/ModularLogger.ino) uses feature flags such as `USE_BME280`, `USE_RTC`, `USE_ADXL345`, and `USE_GPS`. Enabled measurements are compiled into a single `measurements[]` descriptor table. Header generation and data row generation both iterate over that table, guaranteeing matching column order.

## Files

- `firmware/ModularLogger/ModularLogger.ino` - complete example firmware.

## Example configuration: environmental sensors only

For a BME280 environmental logger with RTC, SD card, and one analogue voltage input, use:

```cpp
#define LOG_INTERVAL_MS 10000UL
#define USE_RTC 1
#define USE_BME280 1
#define USE_DHT 0
#define USE_ANALOG_MV 1
#define USE_ADXL345 0
#define USE_GPS 0
#define USE_WIND 0
```

Example CSV output:

```text
Time,Date,TempC,RH%,Pressure_kPa,Altitude_m,DewPoint_C,mV
10:25:32 AM,07/05/16,24.11,35.96,96.24,386.44,8.08,3293
10:25:42 AM,07/05/16,24.22,34.83,96.25,385.77,7.63,3293
```

## Example configuration: environmental sensors, accelerometer, GPS, and analogue voltage

For a richer mobile logger, use:

```cpp
#define LOG_INTERVAL_MS 1000UL
#define USE_RTC 1
#define USE_BME280 1
#define USE_DHT 0
#define USE_ANALOG_MV 1
#define USE_ADXL345 1
#define USE_GPS 1
#define USE_WIND 0
```

Example CSV output:

```text
Time,Date,TempC,RH%,Pressure_kPa,Altitude_m,DewPoint_C,mV,AccelX_ms2,AccelY_ms2,AccelZ_ms2,GPS_Lat,GPS_Lon,GPS_Alt_m,GPS_Speed_kph
02:18:04 PM,07/14/26,23.84,41.20,98.77,211.40,9.88,3288,0.031,-0.119,9.772,40.712776,-74.005974,18.4,3.2
02:18:05 PM,07/14/26,23.85,41.18,98.77,211.28,9.88,3288,0.042,-0.112,9.781,40.712781,-74.005969,18.6,3.5
```

If the RTC is disabled or missing, the `Time` column contains uptime seconds and the `Date` column contains the literal marker `uptime_s`:

```text
Time,Date,TempC,RH%,DewPoint_C
0,uptime_s,24.10,36.00,8.15
10,uptime_s,24.12,35.90,8.10
```

## Adding a new sensor or measurement

1. Add a feature flag near the top of the sketch, for example `#define USE_LIGHT 1`.
2. Include and instantiate the sensor object inside `#if USE_LIGHT` blocks.
3. Add fields and a validity flag to `SampleCache`.
4. Initialize the sensor in `beginSensors()` and print a clear serial status message.
5. Read the sensor in `readSensors()`. Set the validity flag to `false` when the reading is missing or invalid.
6. Add a print callback such as `static void printLux(Print &out)` that prints a value or leaves the CSV field blank.
7. Add a PROGMEM header string and insert `{ hLux, printLux }` into `measurements[]` under `#if USE_LIGHT`.

Do not change `writeHeader()` or `writeRow()` for normal sensor additions. Those functions are intentionally generic.

## Notes

- **Memory usage:** Header labels are stored in flash with `PROGMEM`, status text uses `F()`, and the sketch avoids `String` and heap allocation. The only per-sample RAM structure is `SampleCache`.
- **CSV formatting:** Missing, invalid, or unavailable readings are emitted as empty fields. This preserves the exact number and order of columns without inventing sentinel values.
- **Timestamp handling:** With a working RTC, the logger writes 12-hour time and `MM/DD/YY` date. Without an RTC, it writes elapsed uptime seconds and a `Date` marker of `uptime_s`.
- **SD-card reliability:** The file is flushed after the header and then every `FLUSH_EVERY_N_ROWS` rows. Lower this value to reduce data-loss risk, or raise it to reduce SD writes and power use.
- **Sampling rates:** Keep `LOG_INTERVAL_MS` slower than the slowest enabled sensor. DHT sensors often need about 2 seconds between reads, GPS parsing must be polled often in `loop()`, and wind/anemometer pulse counting should use interrupts.
- **Pressure units:** The sketch logs pressure in kPa. Change the callback/header together if you prefer hPa, Pa, atm, or bar.
- **Altitude:** BME280 altitude is estimated from pressure and the configured sea-level pressure. It is not a replacement for surveyed altitude or GPS altitude.

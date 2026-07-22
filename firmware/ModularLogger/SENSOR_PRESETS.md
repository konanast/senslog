# Sensor presets, wiring, and extension notes

The logger targets Arduino Pro Mini 3.3 V / 8 MHz and 5 V / 16 MHz boards. SRAM, flash, UARTs, pins, and power are limited; enable only the adapters needed by the selected preset.

## Required microSD module

Low-cost SPI microSD module pins:

| microSD pin | Arduino Pro Mini pin |
| --- | --- |
| `3V3` | 3.3 V supply. Do not power the SD card from 5 V unless the module explicitly supports it. |
| `CS` | `D10` by default; configurable with `SD_CS_PIN`. |
| `MOSI` | `D11` hardware SPI MOSI. |
| `CLK` | `D13` hardware SPI SCK. |
| `MISO` | `D12` hardware SPI MISO. |
| `GND` | `GND`. |

Warning: many cheap SD breakout boards do not tolerate 5 V logic on `CS`, `MOSI`, or `CLK`. Use a 3.3 V Pro Mini or level shifting when required. SD writes can draw current spikes; use a stable regulator and local decoupling.

## Optional RTC module

Common 4-pin DS3231/DS1307 RTC wiring:

| RTC pin | Arduino Pro Mini pin |
| --- | --- |
| `GND` | `GND` |
| `VCC` | Module-rated supply, commonly 3.3 V or 5 V |
| `SCL` | `A5` I2C clock |
| `SDA` | `A4` I2C data |

Set `USE_RTC 1` and select `RTC_TYPE_DS3231` or `RTC_TYPE_DS1307`. If the RTC is missing, unavailable, or invalid, `TimestampSource` becomes `UPTIME` and `Time` uses seconds from `millis()`.

## Implemented primary environmental adapters

Enable only one of these primary environmental adapters at a time in the current firmware. Multiple environmental sensors on the same build are possible as a future extension, but the current CSV descriptor and environment getter names intentionally assume one primary environmental source to keep the AVR build small and simple:

| Sensor/module | Flag | Measurements | Library |
| --- | --- | --- | --- |
| BMP280 | `USE_BMP280` | `Temp_C`, `Pressure_hPa`, `Altitude_m` | `Adafruit_BMP280` |
| BME280 | `USE_BME280` | `Temp_C`, `RH_pct`, `Pressure_hPa`, `Altitude_m`, `DewPoint_C` | `Adafruit_BME280` |
| BME680 | `USE_BME680` | BME280 fields plus `GasResistance_ohm` | `Adafruit_BME680` |
| GY-21 / HTU21D / compatible Si7021 | `USE_GY21` | `Temp_C`, `RH_pct`, `DewPoint_C` | `Adafruit_HTU21DF` |

## Working preset files

| Preset file | Configuration |
| --- | --- |
| `Preset_BMP280.h` | BMP280 logger. |
| `Preset_BME280.h` | BME280 logger. |
| `Preset_BME680.h` | BME680 logger. |
| `Preset_GY21.h` | GY-21/HTU21D/Si7021 logger. |
| `Preset_Mobile_BME280.h` | BME280 + MPU6050 + GPS + battery. |
| `Preset_Mobile_BME680.h` | BME680 + MPU6050 + GPS + battery. |
| `Preset_WeatherStation.h` | BME280 + anemometer + wind direction + rain gauge + battery. |

## Common low-cost extension patterns

These modules should be added with the same adapter pattern: feature flag in `Config.h`, conditionally compiled driver code in `Sensors.cpp`, getter in `Sensors.h`, and descriptor callback in `Logger.cpp`.

| Sensor/module | Recommended logged fields | Notes |
| --- | --- | --- |
| BMP180 | `Temp_C`, `Pressure_hPa`, `Altitude_m` | Similar to BMP280, older/lower resolution. |
| DHT11, DHT22, AM2302 | `Temp_C`, `RH_pct`, `DewPoint_C` | Slow; use a 2 s or slower sensor interval. |
| SHT21, SHT30, SHT31 | `Temp_C`, `RH_pct`, `DewPoint_C` | I2C humidity sensors; SHT21 can share the GY-21 pattern. |
| AHT10, AHT20 | `Temp_C`, `RH_pct`, `DewPoint_C` | I2C; low cost and common. |
| LM35 | `Temp_C` | Analogue; conversion depends on ADC reference and board noise. |
| DS18B20 | `Temp_C` | OneWire; waterproof probes are common, conversion can take up to 750 ms. |
| MQ analogue gas sensors | `GasADC`, `GasVoltage_mV` | Log raw ADC and voltage by default. Do not claim ppm without calibration, load resistor details, temperature/humidity compensation, and burn-in. |
| CCS811 | eCO2/TVOC raw fields | I2C; needs burn-in and environmental compensation. |
| SGP30 | eCO2/TVOC raw fields | I2C; baseline management required. |
| PMS5003 | particulate counts/mass fields | UART; high current draw, fan wear, and frame parsing required. |
| SDS011 | particulate mass fields | UART; high current draw and duty-cycle control recommended. |
| MPU6050 | `Accel*`, `Gyro*` | Implemented in mobile presets; I2C. |
| ADXL345 | `Accel*` | Implemented as optional adapter; lower flash/SRAM than full IMU stacks. |
| LIS3DH | `Accel*` | I2C/SPI; similar descriptor pattern to ADXL345. |
| NEO-6M, NEO-M8N GPS | GPS fields | Implemented with TinyGPSPlus; SoftwareSerial uses CPU time and pins. Hardware serial conflicts with upload/debug. |
| Pulse-output anemometers | `WindSpeed_ms` | Implemented with interrupts; calibration depends on rotor model. |
| Analogue wind-direction sensors | `WindDirection_deg` | Implemented as simple 0-1023 to 0-360 placeholder; calibrate resistor ladder. |
| Tipping-bucket rain gauges | `Rainfall_mm` | Implemented with interrupts; calibrate mm per tip. |
| Light, UV, soil-moisture, generic analogue sensors | ADC and voltage fields | Keep names explicit and avoid overclaiming calibrated physical units. |

## Power and size limitations

- GPS, PMS5003/SDS011, SD writes, and radio modules can exceed small regulator budgets.
- SoftwareSerial GPS plus SD writes can cause missed characters at high data rates.
- BME680, GPS, and IMU libraries together may approach AVR flash limits.
- Keep sample intervals realistic: DHT/DS18B20 are slow, particulate sensors need warm-up, and GPS should be polled frequently.

# Sensor presets and low-cost module wiring

The firmware is intended for Arduino Pro Mini variants with a low-cost microSD module and optional 4-pin I2C RTC. Enable only the sensors installed on a given build.

## Required microSD module

The common low-cost SPI microSD module pinout is:

| microSD pin | Arduino Pro Mini pin |
| --- | --- |
| `3v3` | `VCC` on a 3.3 V Pro Mini, or a regulated 3.3 V supply |
| `cs` | `D10` by default (`SD_CS_PIN`) |
| `mosi` | `D11` |
| `clk` | `D13` |
| `miso` | `D12` |
| `gnd` | `GND` |

Use a 3.3 V Pro Mini when possible. If using a 5 V Pro Mini, confirm the SD module has safe level shifting for `cs`, `mosi`, and `clk`.

## Optional RTC module

The common 4-pin DS3231/DS1307 RTC pinout is:

| RTC pin | Arduino Pro Mini pin |
| --- | --- |
| `GND` | `GND` |
| `VCC` | module-rated supply, commonly 3.3 V or 5 V |
| `SCL` | `A5` |
| `SDA` | `A4` |

Set `USE_RTC 1` for DS3231/DS1307 modules supported by Adafruit RTClib. If the RTC is missing or disabled, the logger writes uptime seconds instead.

## Primary environmental sensor presets

Enable only one primary environmental preset at a time:

| Sensor/module | Flags | Measurements | Typical bus/library |
| --- | --- | --- | --- |
| BMP280 | `USE_BMP280 1` | temperature, pressure, altitude | I2C/SPI, `Adafruit_BMP280` |
| BME280 | `USE_BME280 1` | temperature, RH, pressure, altitude, dew point | I2C/SPI, `Adafruit_BME280` |
| BME680 | `USE_BME680 1` | temperature, RH, pressure, altitude, dew point, gas resistance | I2C/SPI, `Adafruit_BME680` |
| GY-21 / HTU21D / SHT21 | `USE_GY21 1` | temperature, RH, dew point | I2C, `Adafruit_HTU21DF` |
| DHT11/DHT22/AM2302 | `USE_DHT 1` | temperature, RH, dew point | one-wire-style GPIO, `DHT` |

## Other common low-cost sensor/module presets

The sketch includes documented flags or callback slots for common inexpensive modules. Some are implemented directly; others are reserved presets for adding a lightweight driver in the same descriptor-table pattern.

| Sensor/module | Flag | Notes |
| --- | --- | --- |
| Analogue voltage/battery divider | `USE_ANALOG_MV` | Implemented; logs `mV` from `ANALOG_MV_PIN`. |
| ADXL345 accelerometer | `USE_ADXL345` | Implemented; logs X/Y/Z acceleration. |
| NEO-6M/compatible GPS | `USE_GPS` | Implemented with TinyGPSPlus and SoftwareSerial. |
| Reed-switch anemometer | `USE_WIND` | Implemented by interrupt pulse counting. |
| MQ-2/MQ-135 analogue gas modules | `USE_MQ_ANALOG` | Implemented; logs raw ADC counts and sensor output mV from `MQ_ANALOG_PIN`. |
| DS18B20 waterproof temperature | `USE_DS18B20` | Reserved preset; add OneWire/DallasTemperature readings if fitted. |
| BH1750 light sensor | `USE_BH1750` | Implemented with direct I2C reads; logs lux. |
| MPU-6050 accelerometer/gyro | `USE_MPU6050` | Reserved preset for 6-axis motion logging. |

When adding a reserved preset, add the library object behind its `#if` flag, add fields to `SampleCache`, add a `print...()` callback, and insert the descriptor in `measurements[]`.

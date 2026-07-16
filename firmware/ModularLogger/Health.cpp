#include "Health.h"
#include <avr/wdt.h>

static uint8_t s_resetCause = 0;

void healthBegin() {
  s_resetCause = MCUSR;
  MCUSR = 0;
  wdt_disable();
}

uint16_t readVccMillivolts() {
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2);
  ADCSRA |= _BV(ADSC);
  while (bit_is_set(ADCSRA, ADSC)) {}
  uint16_t adc = ADC;
  if (adc == 0) return 0;
  return (uint16_t)(1125300UL / adc); // 1.1 V bandgap * 1023 * 1000.
#else
  return 0;
#endif
}

uint16_t readBatteryMillivolts() {
#if USE_BATTERY
  uint16_t raw = analogRead(BATTERY_PIN);
  unsigned long adcMv = ((unsigned long)raw * BATTERY_ADC_REF_MV) / 1023UL;
  unsigned long batMv = (adcMv * (BATTERY_R1_OHMS + BATTERY_R2_OHMS)) / BATTERY_R2_OHMS;
  return batMv > 65535UL ? 65535U : (uint16_t)batMv;
#else
  return 0;
#endif
}

uint8_t readBatteryPercent() {
#if USE_BATTERY && BATTERY_PCT_ENABLED
  uint16_t mv = readBatteryMillivolts();
  if (mv <= BATTERY_EMPTY_MV) return 0;
  if (mv >= BATTERY_FULL_MV) return 100;
  return (uint8_t)(((unsigned long)(mv - BATTERY_EMPTY_MV) * 100UL) / (BATTERY_FULL_MV - BATTERY_EMPTY_MV));
#else
  return 255;
#endif
}

int freeRamBytes() {
  extern int __heap_start, *__brkval;
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

uint8_t resetCause() { return s_resetCause; }

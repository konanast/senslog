#include "Timestamp.h"
#include "Config.h"
#include "Status.h"
#include <Wire.h>

#if USE_RTC
#include <RTClib.h>
#if RTC_TYPE_DS3231
static RTC_DS3231 rtc;
#else
static RTC_DS1307 rtc;
#endif
static bool s_rtcOk = false;
static DateTime s_startTime;
#endif

static void print2(Print &out, uint8_t v) {
  if (v < 10) out.print('0');
  out.print(v);
}

void timestampBegin() {
#if USE_RTC
  s_rtcOk = rtc.begin();
  if (!s_rtcOk) {
    g_rtcStatus |= STATUS_RTC_MISSING;
    g_timestampSource = TS_UPTIME;
    return;
  }
#if RTC_TYPE_DS3231
  if (rtc.lostPower()) {
    g_rtcStatus |= STATUS_RTC_INVALID;
    g_timestampSource = TS_UPTIME;
    return;
  }
#endif
  s_startTime = rtc.now();
  if (s_startTime.year() < 2020) {
    g_rtcStatus |= STATUS_RTC_INVALID;
    g_timestampSource = TS_UPTIME;
    return;
  }
  g_timestampSource = TS_RTC;
#else
  g_rtcStatus |= STATUS_RTC_MISSING;
  g_timestampSource = TS_UPTIME;
#endif
}

bool rtcAvailable() {
#if USE_RTC
  return s_rtcOk && g_timestampSource == TS_RTC;
#else
  return false;
#endif
}

void printDate(Print &out) {
#if USE_RTC
  if (rtcAvailable()) {
    DateTime now = rtc.now();
    print2(out, now.month()); out.print('/'); print2(out, now.day()); out.print('/'); print2(out, now.year() % 100);
    return;
  }
#endif
  out.print(F("NA"));
}

void printTime(Print &out) {
#if USE_RTC
  if (rtcAvailable()) {
    DateTime now = rtc.now();
    print2(out, now.hour()); out.print(':'); print2(out, now.minute()); out.print(':'); print2(out, now.second());
    return;
  }
#endif
  out.print(millis() / 1000UL);
}

void printTimestampSource(Print &out) {
  if (g_timestampSource == TS_RTC) out.print(F("RTC"));
  else if (g_timestampSource == TS_GPS) out.print(F("GPS"));
  else out.print(F("UPTIME"));
}

void printRtcStatus(Print &out) { out.print(g_rtcStatus); }

void printStartTime(Print &out) {
#if USE_RTC
  if (rtcAvailable()) {
    print2(out, s_startTime.month()); out.print('/'); print2(out, s_startTime.day()); out.print('/'); out.print(s_startTime.year()); out.print(' ');
    print2(out, s_startTime.hour()); out.print(':'); print2(out, s_startTime.minute()); out.print(':'); print2(out, s_startTime.second());
    return;
  }
#endif
  out.print(F("UPTIME"));
}

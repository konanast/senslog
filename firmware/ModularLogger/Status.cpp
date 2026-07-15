#include "Status.h"
uint16_t g_sensorStatus = STATUS_OK;
uint16_t g_rtcStatus = STATUS_OK;
uint16_t g_sdStatus = STATUS_OK;
uint16_t g_sdErrorCount = 0;
uint16_t g_invalidReadingCount = 0;
uint32_t g_recordId = 0;
uint32_t g_bootId = 0;
uint16_t g_lastLoopTimeMs = 0;
uint16_t g_lastSdWriteTimeMs = 0;
TimestampSource g_timestampSource = TS_UPTIME;

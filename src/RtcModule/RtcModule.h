#ifndef TIME_HANDLER_H
#define TIME_HANDLER_H

#include <time.h>

time_t GetTimeFromRtc();
void SetRtcTime(time_t currentTimeInt);

#endif
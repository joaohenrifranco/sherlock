#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <time.h>
#include <string.h>

class ClockHandler
{
	time_t timeOffset;
public:
	ClockHandler();
	void setTimeWithInt(time_t currentTime);
	bool setTimeWithNTP();
	time_t getUnixTime();
};


#endif
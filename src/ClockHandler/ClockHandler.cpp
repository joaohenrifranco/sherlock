extern "C" {
	#include <user_interface.h>
}
#include <Arduino.h>
#include "ClockHandler.h"
#include "../config.h"

#define TIMEOUT 10000

ClockHandler::ClockHandler() {
	timeOffset = 0;
}

void ClockHandler::setTimeWithInt(time_t newTime) {
	timeOffset = newTime - millis()/1000;
}

bool ClockHandler::setTimeWithNTP() { 
	configTime(TIMEZONE * 3600, 0, "pool.ntp.org", "a.ntp.br");
	
	time_t start = millis();
	
	while(!time(nullptr) && (millis() - start) < TIMEOUT)
	{
		Serial.print(".");
		delay(10);
	}
	if (!time(nullptr))
		return false;
	
	timeOffset = time(nullptr) - millis()/1000;

	return true;
}

time_t ClockHandler::getUnixTime() {
	return (millis()/1000) + timeOffset;
}

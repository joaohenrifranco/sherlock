#include <Wire.h>
#include <RtcDS3231.h>

#include "./RtcModule.h"

time_t GetTimeFromRtc() {
	RtcDS3231<TwoWire> rtcObject(Wire);
	rtcObject.Begin();
	RtcDateTime currentTimeObject = rtcObject.GetDateTime();
	return (time_t) currentTimeObject.Epoch64Time();
}

void SetRtcTime(time_t currentTimeInt) {
 RtcDS3231<TwoWire> rtcObject(Wire);
 rtcObject.Begin();
 RtcDateTime currentTimeObject;
 currentTimeObject.InitWithEpoch64Time(currentTimeInt);
 rtcObject.SetDateTime(currentTimeObject);
}

#ifndef PROBE_H
#define PROBE_H

#include <time.h>

#define SSID_MAX_SIZE 32

class Probe
{

public:
// TODO: Make these private
	char dstMac[18];
	char srcMac[18];
	char bssidMac[18];
	char ssid[SSID_MAX_SIZE + 1];
	int rssi;
	int channel;
	time_t capturedAt;

	Probe();
	void setRssi(int newRssi);
	void setChannel(int newChannel);
	void setCapturedAt(time_t newCapturedAt);
};

#endif

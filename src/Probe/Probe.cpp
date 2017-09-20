#include "Probe.h"

Probe::Probe(){}

void Probe::setRssi(int newRssi)
{
	rssi = newRssi;
}
void Probe::setChannel(int newChannel)
{
	channel = newChannel;
}
void Probe::setCapturedAt(time_t newCapturedAt)
{
	capturedAt = newCapturedAt;
}

#ifndef REPORT_H
#define REPORT_H

#include <string>
#include "../Probe/Probe.h"
#include "../config.h"

class Report
{
	char beaconMac[18];
	Probe probeArray[MAX_PROBES_PER_REPORT];
	int probeCount;
public:
	Report();
	void setBeaconMac(char newBeaconMac[]);
	void insertProbe(Probe newProbe);
	void send();
	String buildJson();
};

#endif

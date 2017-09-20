#include <stdio.h>

#include "PacketParser.h"
#include "config.h"

void getDataSpan(uint16_t start, uint16_t size, uint8_t* data, char dataSpan[]) {
	dataSpan[0] = '\0';
	for(uint16_t i = start; i < DATA_LENGTH && i < start+size; i++) {
		sprintf(dataSpan, "%s%c", dataSpan, data[i]);
	}
}
	
void getMac(char *addr, uint8_t* data, uint16_t offset) {
	sprintf(addr, "%02x:%02x:%02x:%02x:%02x:%02x", data[offset+0], data[offset+1], data[offset+2], data[offset+3], data[offset+4], data[offset+5]);
}

SnifferPacket getFrameData (uint8_t *buffer) {
	struct SnifferPacket *snifferPacket = (struct SnifferPacket*) buffer;
	return *snifferPacket;
}

unsigned int getFrameControl (SnifferPacket snifferPacket) {
	return ((unsigned int)snifferPacket.data[1] << 8) + snifferPacket.data[0];
}

bool isProbeRequest(SnifferPacket snifferPacket) {
	unsigned int frameControl = getFrameControl(snifferPacket);
		
	if (getFrameType(frameControl) != TYPE_MANAGEMENT || getFrameSubType(frameControl) != SUBTYPE_PROBE_REQUEST)
		return false;

	return true;
}

uint8_t getVersion (unsigned int frameControl) {
	return (frameControl & 0b0000000000000011) >> 0;
}
uint8_t getFrameType (unsigned int frameControl) {
	return (frameControl & 0b0000000000001100) >> 2;
}
uint8_t getFrameSubType (unsigned int frameControl) {
	return (frameControl & 0b0000000011110000) >> 4;
}
uint8_t getToDs (unsigned int frameControl) {
	return (frameControl & 0b0000000100000000) >> 8;
}
uint8_t getFromDs (unsigned int frameControl) {
	return (frameControl & 0b0000001000000000) >> 9;
}

void getDstMac(SnifferPacket snifferPacket, char *dstMac) {
	getMac(dstMac, snifferPacket.data, 4);
}

void getSrcMac(SnifferPacket snifferPacket, char *srcAddr) {
	getMac(srcAddr, snifferPacket.data, 10);
}

void getBssidMac(SnifferPacket snifferPacket, char *bssidBrodAddr) {
	getMac(bssidBrodAddr, snifferPacket.data, 16);
}

void getSsid(SnifferPacket snifferPacket, char *ssid) {
	uint8_t SSID_length = snifferPacket.data[25];
	getDataSpan(26, SSID_length, snifferPacket.data, ssid);
}

int getRssi(SnifferPacket snifferPacket) {
	return snifferPacket.rx_ctrl.rssi;
}



#ifndef PACKET_PARSER_H
#define PACKET_PARSER_H

extern "C" { 
	#include <user_interface.h>
}
#include <queue> 
#include "../Probe/Probe.h"

#define DATA_LENGTH           112

#define TYPE_MANAGEMENT       0x00
#define TYPE_CONTROL          0x01
#define TYPE_DATA             0x02
#define SUBTYPE_PROBE_REQUEST 0x04

// 802.11 protocol data structures
struct RxControl {
	signed rssi:8; // signal intensity of packet
	unsigned rate:4;
	unsigned is_group:1;
	unsigned:1;
	unsigned sig_mode:2; // 0:is 11n packet; 1:is not 11n packet;
	unsigned legacy_length:12; // if not 11n packet, shows length of packet.
	unsigned damatch0:1;
	unsigned damatch1:1;
	unsigned bssidmatch0:1;
	unsigned bssidmatch1:1;
	unsigned MCS:7; // if is 11n packet, shows the modulation and code used (range from 0 to 76)
	unsigned CWB:1; // if is 11n packet, shows if is HT40 packet or not
	unsigned HT_length:16;// if is 11n packet, shows length of packet.
	unsigned Smoothing:1;
	unsigned Not_Sounding:1;
	unsigned:1;
	unsigned Aggregation:1;
	unsigned STBC:2;
	unsigned FEC_CODING:1; // if is 11n packet, shows if is LDPC packet or not.
	unsigned SGI:1;
	unsigned rxend_state:8;
	unsigned ampdu_cnt:8;
	unsigned channel:4; //which channel this packet in.
	unsigned:12;
};
   
struct SnifferPacket {
	struct RxControl rx_ctrl;
	uint8_t data[DATA_LENGTH];
	uint16_t cnt;
	uint16_t len;
};

void getDataSpan(uint16_t start, uint16_t size, uint8_t* data, char dataSpan[]);

void getMac(char *addr, uint8_t* data, uint16_t offset);

SnifferPacket getFrameData(uint8_t *buffer);

bool isProbeRequest(SnifferPacket snifferPacket);

uint8_t getVersion (unsigned int frameControl);
uint8_t getFrameType (unsigned int frameControl);
uint8_t getFrameSubType (unsigned int frameControl);
uint8_t getToDs (unsigned int frameControl);
uint8_t getFromDs (unsigned int frameControl);

void getDstMac(SnifferPacket snifferPacket, char *dstMac);
void getSrcMac(SnifferPacket snifferPacket, char *scrMac);
void getBssidMac(SnifferPacket snifferPacket, char *bssidMac);
void getSsid(SnifferPacket snifferPacket, char *ssid);
int getRssi(SnifferPacket snifferPacket);


#endif
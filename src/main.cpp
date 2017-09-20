extern "C" {
	#include <user_interface.h>
}
#include <queue>
#include <time.h>
#include <SPI.h>
#include <Wire.h>
#include <RtcDS3231.h>
#include "SdFat.h"
#include "ESP8266WiFi.h"

#include "config.h"
#include "ChannelSwitcher/ChannelSwitcher.h"
#include "Report/Report.h"
#include "Probe/Probe.h"
#include "PacketParser/PacketParser.h"
#include "RtcModule/RtcModule.h"


// Putting macro values from config.h into variables

bool sdMode  = SD_MODE;
bool httpPostMode = HTTP_POST_MODE;
const char* ssid     = AP_SSID;
const char* password = AP_PASSWORD;
int timezone = TIMEZONE;
const bool exclusiveMacMode  = EXCLUSIVE_MAC_MODE;
const char* exclusiveMacTarget = EXCLUSIVE_MAC_TARGET;
bool printProbeEnable = PRINT_PROBES;

SdFat SD;
File probesFile;
int fileSizeCounter = 0;
int fileCounter = 0;
bool isChangingFile = false;

std::queue<Probe> probeQueue;

time_t lastSent;
bool isConnectedToAp = false;
bool isInPromiscuosMode = false;
bool getTimeFromNtp = false;

uint8_t beaconMac[6];
char beaconMacString[18];

void PrintProbe(Probe probe, int queueSize) {
	Serial.print(queueSize);

	Serial.printf(") RSSI: %d - Ch: %d - dstMac: %s - srcMac: %s - bssidMac: %s - SSID: %s\n", 
										probe.rssi, probe.channel, probe.dstMac, probe.srcMac, probe.bssidMac, probe.ssid);
}

void OpenAvailableFile() {
	isChangingFile = true;
	char fileName[14];
	do {
		snprintf(fileName, 14, "probes%d.txt", fileCounter);
		probesFile = SD.open(fileName, FILE_WRITE);
		fileCounter++;
		fileSizeCounter =	probesFile.fileSize();
	} while (probesFile.fileSize() > MAX_FILE_SIZE);
	
	if (probesFile)
		Serial.printf("SD CARD: Using file \"%s\"\n", fileName);
	else {
		Serial.println("SD CARD: File opening failed. SD disabled.");
		sdMode = DISABLE;
	}
	isChangingFile = false;

}

void WriteProbeToFile(Probe probe) {
	if (fileSizeCounter > MAX_FILE_SIZE) {
		probesFile.close();
		if (!isChangingFile)
			OpenAvailableFile();
	}
			
	if (probesFile) {
			probesFile.printf("%d;%d;%s;%s;%s;%s;%d\r\n", 
								probe.rssi, probe.channel, probe.dstMac, 
								probe.srcMac, probe.bssidMac, probe.ssid,
								probe.capturedAt);
			probesFile.flush();
			fileSizeCounter += 128;
			Serial.println("SD CARD: Written to SD!");
		}
		else
		{
			Serial.printf("SD CARD: Error opening file, writing to SD Card disabled\n");
			sdMode = DISABLE;
		}
}

static void ICACHE_FLASH_ATTR snifferCallback(uint8_t *buffer, uint16_t length) {
	SnifferPacket snifferPacket = getFrameData(buffer);
	
	if (!isProbeRequest(snifferPacket)) 
		return;
	
	char srcAddr[18] = "00:00:00:00:00:00";
	getSrcMac(snifferPacket, srcAddr);

	// Only look for probe request from specific MAC address
	if ((strcmp(srcAddr, exclusiveMacTarget) != 0) && exclusiveMacMode)
		return;

	Probe probe;
	strcpy(probe.srcMac, srcAddr);
	getDstMac(snifferPacket, probe.dstMac);
	getBssidMac(snifferPacket, probe.bssidMac);
	getSsid(snifferPacket, probe.ssid);
	probe.setRssi(getRssi(snifferPacket));
	probe.setCapturedAt(time(nullptr));
	probe.setChannel(wifi_get_channel());
	probeQueue.push(probe);
	
	if (printProbeEnable == true)
		PrintProbe(probe, probeQueue.size());
	if (sdMode == ENABLE)
		WriteProbeToFile(probe);
	if (httpPostMode == DISABLE)
		probeQueue.pop();
}

bool IsSsidInRange(String setupSsid) {
	int networkCount = WiFi.scanNetworks();
	Serial.println("STATION MODE: Scanning networks...");
	if (networkCount == 0) {
		Serial.println("STATION MODE: No networks found.");
		return false;
	}
	else {
		Serial.printf("STATION MODE: %d networks found.\n", networkCount);
		for (int i = 0; i < networkCount; ++i) {
			if (WiFi.SSID(i) == setupSsid) {
			Serial.println("STATION MODE: Setup network \"" + setupSsid + "\" found!");
			return true;
			}
		}
	}
	Serial.println("STATION MODE: \"" + setupSsid + "\" not found. Disabling NTP time update and POST to server.");
	return false;
}

void EnterPromiscuosMode(){
	isInPromiscuosMode = true;
	isConnectedToAp = false;
	Serial.println("PROMISCUOS MODE: Setting up...");
	wifi_promiscuous_enable(DISABLE);
	delay(10);
	wifi_set_promiscuous_rx_cb(snifferCallback);
	delay(10);
	wifi_promiscuous_enable(ENABLE);
	Serial.println("PROMISCUOS MODE: Setup sucessful.");
}

void ConnectAcessPoint() {
	isConnectedToAp = true;
	isInPromiscuosMode = false;
	WiFi.mode(WIFI_STA);
	delay(10);
	wifi_promiscuous_enable(DISABLE);
	delay(10);
	WiFi.begin(ssid, password);
	Serial.printf("STATION MODE: Connecting to \"%s\"", ssid);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	Serial.printf("\n");
	Serial.printf("STATION MODE: Connected. IP address: ");
	Serial.print(WiFi.localIP());
	Serial.printf("\n");
}

void setup() {
	Serial.begin(115200);
	Serial.println("");
	
	// Getting beacon MAC
	WiFi.macAddress(beaconMac);
	getMac(beaconMacString, beaconMac, 0);
	
	Serial.printf("BEACON MAC: %s\n", beaconMacString);

	// SD Card setup
	if (sdMode == ENABLE) {
		Serial.println("SD CARD: Initializing...");
		if (!SD.begin(SD_READER_PIN)) {
			Serial.println("SD CARD: Initialization failed! No data will be recorded.");
			sdMode = DISABLE;
		}
		else {
			Serial.println("SD CARD: Initialization successful!");
			OpenAvailableFile();
		}
	}
	else
		Serial.println("SD CARD: Disabled.");

	// Check if beacon is at "base"
	if (IsSsidInRange(ssid))
		getTimeFromNtp = true;
	else
		httpPostMode = false;

	// Time setup
	time_t now;
	if (getTimeFromNtp) {
		ConnectAcessPoint();
		configTime(timezone * 3600, 0, "pool.ntp.org", "a.ntp.br");
		Serial.print("TIME SETTING: Retrieving time info from NTP server");
		while (!time(nullptr)) {
			Serial.print(".");
			delay(500);
		}
		Serial.printf("\n");
		now = time(nullptr);
		SetRtcTime(now);
	}
	else {
		Serial.println("TIME SETTING: Retrieving time from module...");
		now = GetTimeFromRtc();
	}
	Serial.printf("TIME SETTING: %s", ctime(&now));
	
	// Sets lastSent flag
	lastSent = now;
	
	// Start probe listening
	EnterPromiscuosMode();

	SetChannelHop((void *)&isConnectedToAp);
}

void loop() {
	
	if (httpPostMode == ENABLE) {
		if (probeQueue.size() > MAX_QUEUE_SIZE || time(nullptr) - lastSent > BUFFER_TIMEOUT) {
			Serial.printf("HTTP POST: Connection with server triggered: ");
		
			if (probeQueue.size() > MAX_QUEUE_SIZE)
				Serial.printf("queue full\n");
			else
				Serial.printf("buffer timeout (%d s)\n", BUFFER_TIMEOUT);

			ConnectAcessPoint();

			// Multiple posts are being sent to reduce payload size and prevent crashes while json building or http posting	
			while (probeQueue.size() != 0) {
				Report report;
				report.setBeaconMac(beaconMacString);

				int probeReportIndex;

				for (probeReportIndex = 0; probeReportIndex < MAX_PROBES_PER_REPORT && !(probeQueue.empty()); probeReportIndex++)
				{
					report.insertProbe(probeQueue.front());
					probeQueue.pop();
				}

				lastSent = time(nullptr);
				Serial.printf("HTTP POST: Report built with %d probes.\n", probeReportIndex);

				report.send();
				Serial.printf("HTTP POST: Report sent with %d probes.\n", probeReportIndex);
			}
			Serial.println("HTTP POST: Finished sending reports.");

			EnterPromiscuosMode();
		}
	}
}
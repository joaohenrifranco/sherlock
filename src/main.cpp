extern "C" {
  #include <user_interface.h>
}
#include <queue>
#include <time.h>
#include <SPI.h>
#include "SdFat.h"
#include "ESP8266WiFi.h"

#include "config.h"
#include "ChannelSwitcher/ChannelSwitcher.h"
#include "Report/Report.h"
#include "Probe/Probe.h"
#include "PacketParser/PacketParser.h"

bool sdMode  = SD_MODE;
bool httpPostMode = HTTP_POST_MODE;
const char* ssid     = AP_SSID;
const char* password = AP_PASSWORD;
int timezone = TIMEZONE;
const bool exclusiveMacMode  = EXCLUSIVE_MAC_MODE;
const char* exclusiveMacTarget = EXCLUSIVE_MAC_TARGET;
bool printProbeEnable = PRINT_PROBES;
bool setTime = SET_TIME_NTP;

SdFat SD;

std::queue<Probe> probeQueue;

time_t lastSent;
bool isSendingReport = false;

uint8_t beaconMac[6];
char beaconMacString[18];

void connectAcessPoint() {
  WiFi.begin(ssid, password);

  Serial.printf("Connecting to \"%s\"", ssid);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.printf("\n");
  Serial.printf("Connected. IP address: ");
  Serial.print(WiFi.localIP());
  Serial.printf("\n");
}

void PrintProbe(Probe probe, int queueSize) {
	Serial.print(queueSize);

	Serial.printf(") RSSI: %d - Ch: %d - dstMac: %s - srcMac: %s - bssidMac: %s - SSID: %s\n", 
	                  probe.rssi, probe.channel, probe.dstMac, probe.srcMac, probe.bssidMac, probe.ssid);
}

void WriteProbeToFile(Probe probe)
{
		File probesFile = SD.open("probes.txt", FILE_WRITE);
		if (probesFile) {
			probesFile.printf("%d;%d;%s;%s;%s;%s;%d\r\n", 
							  probe.rssi, probe.channel, probe.dstMac, 
							  probe.srcMac, probe.bssidMac, probe.ssid,
							  probe.capturedAt);
			probesFile.close();
			Serial.println("Written to SD!");
		}
		else
		{
			Serial.println("Error opening file, writing to SD Card disabled");
		}
}

// Callback for promiscuous mode
static void ICACHE_FLASH_ATTR snifferCallback(uint8_t *buffer, uint16_t length) {
	
	SnifferPacket snifferPacket = getFrameData(buffer);

	if (!isProbeRequest(snifferPacket)) 
		return;
	
	// Only look for probe request from specific MAC address
	char srcAddr[18] = "00:00:00:00:00:00";
	getSrcMac(snifferPacket, srcAddr);

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

void setup() {
  Serial.begin(115200);
	Serial.println("");
	
  WiFi.macAddress(beaconMac);
  getMac(beaconMacString, beaconMac, 0);

  // Initialize SD card
  if (sdMode == ENABLE) {
    Serial.println("Initializing SD card...");
    if (!SD.begin(SD_READER_PIN)) {
      Serial.println("Initialization failed! No data will be recorded.");
      sdMode = DISABLE;
    }
    else {
      Serial.println("SD initialization successful!");
    }
	}
	else
		Serial.println("SD card disabled.");

	time_t now = 0;

  // Connect to defined network
  if (setTime) {
		WiFi.mode(WIFI_STA);
		connectAcessPoint();

		// Time setup
		configTime(timezone * 3600, 0, "pool.ntp.org", "a.ntp.br");
		Serial.print("Retrieving time info from NTP server");
		while (!time(nullptr)) {
			Serial.print(".");
			delay(500);
		}
		Serial.printf("\n");
		now = time(nullptr);
		Serial.print("Datetime: ");
		Serial.println(ctime(&now));
	}

	else {
		Serial.println("Time setting disabled.");
		Serial.print("Datetime: ");
		Serial.println(ctime(&now));
	}
	
	// Resets lastSent flag
	lastSent = now;
	
	// Start probe listening
  Serial.println("Setting up promiscuous mode...");
  wifi_promiscuous_enable(DISABLE);
  delay(10);
  wifi_set_promiscuous_rx_cb(snifferCallback);
  delay(10);
	wifi_promiscuous_enable(ENABLE);
	Serial.println("Promiscuos mode set.");
	
	SetChannelHop((void *)&isSendingReport);
}

void loop() {
	
	if (httpPostMode = ENABLE) {
		
		if (probeQueue.size() > MAX_QUEUE_SIZE || time(nullptr) - lastSent > BUFFER_TIMEOUT) {
		Serial.printf("Connection with server triggered: ");
		
			if (probeQueue.size() > MAX_QUEUE_SIZE)
				Serial.printf("queue full\n");
			else
				Serial.printf("buffer timeout (%d s)\n", BUFFER_TIMEOUT);
			
			Serial.printf("Entering station mode...");
			isSendingReport = true;
			WiFi.mode(WIFI_STA);
			delay(10);
			wifi_promiscuous_enable(DISABLE);
			delay(10);
			connectAcessPoint();

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
				Serial.printf("Report built with %d probes.\n", probeReportIndex);

				report.send();
				Serial.printf("Report sent with %d probes.\n", probeReportIndex);
			}
			Serial.println("Finished sending reports.");
			Serial.println("Setting up promiscuous mode...");
			wifi_promiscuous_enable(DISABLE);
			delay(10);
			wifi_set_promiscuous_rx_cb(snifferCallback);
			delay(10);
			wifi_promiscuous_enable(ENABLE);
			Serial.println("Promiscuos mode set.");
			isSendingReport = false;
		}
	

	}
}
extern "C" {
	#include <user_interface.h>
}
#include <queue>
#include <SPI.h>
#include <Wire.h>
#include <RtcDS3231.h>
#include <time.h>
#include "SdFat.h"
#include "ESP8266WiFi.h"

#include "config.h"
#include "ChannelSwitcher/ChannelSwitcher.h"
#include "Report/Report.h"
#include "Probe/Probe.h"
#include "PacketParser/PacketParser.h"
#include "RtcModule/RtcModule.h"
#include "ClockHandler/ClockHandler.h"


// Puts macro values from config.h into consts and variables
bool sdMode  = SD_LOGGING_MODE;
bool httpPostMode = HTTP_POST_MODE;
const char* ssid     = AP_SSID;
const char* password = AP_PASSWORD;
const bool exclusiveMacMode  = EXCLUSIVE_MAC_MODE;
const char* exclusiveMacTarget = EXCLUSIVE_MAC_TARGET;
bool printProbeEnable = PRINT_PROBES;
bool getTimeFromNtp = NTP_UPDATE;

// SD Card
SdFat SD;
File probesFile;
int fileSizeCounter = 0;
int fileCounter = 0;
bool isChangingFile = false;

std::queue<Probe> probeQueue;

// State Control
time_t lastSent;
bool isConnectedToAp = false;
bool isInPromiscuosMode = false;

// Beacon MAC
uint8_t beaconMac[6];
char beaconMacString[18];

// Button behavior variables
int personCounter = 0;
int buttonLedState = OFF;
int buttonState;
int lastButtonState = ON;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

time_t currentTime;
ClockHandler clockHandler;

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
	probe.setCapturedAt(clockHandler.getUnixTime());
	probe.setChannel(wifi_get_channel());
	
	if (httpPostMode)
		probeQueue.push(probe);
	if (printProbeEnable)
		PrintProbe(probe, probeQueue.size());
	if (sdMode)
		WriteProbeToFile(probe);

	// Turns ON LED. LED is turned off on each loop() run. (Check below)
	digitalWrite(PROBE_RECEIVED_LED_PIN, ON);
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
	Serial.println("STATION MODE: \"" + setupSsid + 
	               "\" not found. Disabling NTP time update and POST to server.");
	return false;
}

void EnterPromiscuosMode(){
	digitalWrite(WIFI_STA_LED_PIN, OFF);
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
	digitalWrite(WIFI_STA_LED_PIN, ON);	
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
	pinMode(POWER_LED_PIN, OUTPUT); 
	pinMode(WIFI_STA_LED_PIN, OUTPUT);
	pinMode(PROBE_RECEIVED_LED_PIN, OUTPUT);  

	digitalWrite(POWER_LED_PIN, ON);

	Serial.begin(115200);
	Serial.println("");
	
	// Getting beacon MAC
	WiFi.macAddress(beaconMac);

	getMac(beaconMacString, beaconMac, 0);
	
	Serial.printf("BEACON MAC: %s\n", beaconMacString);

	// Check if beacon is at "base"
	if (getTimeFromNtp || httpPostMode)
		if (!IsSsidInRange(ssid)) {
			getTimeFromNtp = false;
			httpPostMode = false;
		}

	// Time setup

	if (getTimeFromNtp) {
		ConnectAcessPoint();
		Serial.print("TIME SETTING: Retrieving time info from NTP server");
		if (clockHandler.setTimeWithNTP()) {
			currentTime = clockHandler.getUnixTime();
			SetRtcTime(currentTime);
		}
		else { 
			Serial.println("TIME SETTING: NTP Connection timed out. Getting time from RTC.");
			getTimeFromNtp = false;
		}
	}
	if (!getTimeFromNtp) {
		Serial.println("TIME SETTING: Retrieving time from module...");
		currentTime = GetTimeFromRtc();
		clockHandler.setTimeWithInt(currentTime);
	}
	
	Serial.printf("TIME SETTING: %s", ctime(&currentTime));

	// Button setup
	if (BUTTON_MODE)
	{
		pinMode(BUTTON_PIN, INPUT);
		pinMode(BUTTON_LED_PIN, OUTPUT);
		digitalWrite(BUTTON_LED_PIN, ON);
	}

	// SD Card setup
	if (sdMode) {
		Serial.println("SD CARD: Initializing...");
		
		if (SD.begin(SD_READER_PIN)) {
			Serial.println("SD CARD: Initialization successful!");
			
			File logFile  = SD.open("log.txt", FILE_WRITE);
			if (logFile) {
				logFile.printf("DEVICE \"%s\" BOOTED AT: %d\r\n", beaconMacString, currentTime);
				logFile.close();
				Serial.println("SD CARD: Boot logged successfully.");
			}
			else {
				Serial.println("SD CARD: Failed to log boot.");
			}
		
			OpenAvailableFile();
		}
		else {
			Serial.println("SD CARD: Initialization failed! No data will be recorded.");
			sdMode = DISABLE;
		}
	}
	else
	Serial.println("SD CARD: Disabled in config.h file.");
	
	// Sets lastSent flag
	lastSent = currentTime;
	
	// Start probe listening
	EnterPromiscuosMode();

	SetChannelHop((void *)&isConnectedToAp);
}

void loop() {
	// Turns off probe led on every loop
	digitalWrite(PROBE_RECEIVED_LED_PIN, OFF);

	// HTTP POST
	if (httpPostMode == ENABLE) {
		if (probeQueue.size() > MAX_QUEUE_SIZE || millis() - lastSent > BUFFER_TIMEOUT) {
			Serial.printf("HTTP POST: Connection with server triggered: ");
		
			if (probeQueue.size() > MAX_QUEUE_SIZE)
				Serial.printf("queue full\n");
			else
				Serial.printf("buffer timeout (%d s)\n", BUFFER_TIMEOUT);

			ConnectAcessPoint();

			// Multiple posts are being sent to reduce payload size and prevent crashes 
			// while json building or http posting	
			while (probeQueue.size() != 0) {
				Report report;
				report.setBeaconMac(beaconMacString);

				int probeReportIndex;

				for (probeReportIndex = 0; 
						 probeReportIndex < MAX_PROBES_PER_REPORT && !(probeQueue.empty()); 
						 probeReportIndex++)
				{
					report.insertProbe(probeQueue.front());
					probeQueue.pop();
				}

				lastSent = millis();
				Serial.printf("HTTP POST: Report built with %d probes.\n", probeReportIndex);

				report.send();
				Serial.printf("HTTP POST: Report sent with %d probes.\n", probeReportIndex);
			}
			Serial.println("HTTP POST: Finished sending reports.");

			EnterPromiscuosMode();
		}
	}

	// Button implementation with debounce
	if (BUTTON_MODE) {
		int reading = digitalRead(BUTTON_PIN);

		if (reading != lastButtonState) {
			lastDebounceTime = millis();
			buttonLedState = OFF;
		}
		
		if ((millis() - lastDebounceTime) > debounceDelay) {
			if (reading != buttonState) {
				buttonState = reading;
				currentTime = clockHandler.getUnixTime();	
				
				if (buttonState == HIGH) {
					Serial.printf("BUTTON PRESSED: Person number %d at %s", personCounter, ctime(&currentTime));
					personCounter++;
					buttonLedState = ON;
					
					if (personCounter > MAX_PEOPLE)
						personCounter = 0;

					if (probesFile && sdMode) {
						probesFile.printf("BUTTON: PERSON %d AT %s", personCounter, ctime(&currentTime));
						probesFile.flush();
						fileSizeCounter += 128;
						Serial.println("SD CARD: Written to SD!");
					}
					else {
						if (sdMode) {
							Serial.printf("SD CARD: Error opening file, writing to SD Card disabled\n");
							sdMode = DISABLE;
						}
					}
				}
			}
		}
		
		digitalWrite(BUTTON_LED_PIN, buttonLedState);		
		// save the reading. Next time through the loop, it'll be the lastButtonState:
		lastButtonState = reading;
		}

}
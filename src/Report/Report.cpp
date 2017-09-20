#include <string.h>
#include <ArduinoJson.h>
#include "Report.h"

#include "../ESP8266HTTPClient/ESP8266HTTPClient.h"

Report::Report()
{
	probeCount = 0;
}
void Report::setBeaconMac(char newBeaconMac[])
{
	strcpy(beaconMac, newBeaconMac);
}
void Report::insertProbe(Probe newProbe)
{
	probeArray[probeCount] = newProbe;
	probeCount++;
}
void Report::send()
{
	HTTPClient http;
	http.begin(HTTP_SERVER_ADDRESS);
	http.addHeader("Host", "api-sherlock.rappdo.com");
	http.addHeader("Accept-Encoding", "gzip, deflate, br");
	http.addHeader("Accept-Language", "pt-BR,pt;q=0.8,en-US;q=0.6,en;q=0.4");
	http.addHeader("User-Agent", "Sherlock/ESP8266");
	http.addHeader("Content-Type", "application/json");
	http.addHeader("Accept", "application/json, text/plain, */*");
	http.addHeader("Connection", "close");
	String jsonString;
	jsonString = buildJson();
	Serial.println(jsonString);
	int httpCode = http.POST(jsonString);
	if (httpCode != 200)
	{
		Serial.printf("HTTP POST Failed. Error %d: ", httpCode);
		Serial.print(http.errorToString(httpCode) + "\n");
	}
	else
	{
		String payload = http.getString();
		Serial.println("HTTP POST sucessful: " + payload);
	}
	http.end();
}

String Report::buildJson()
{

	DynamicJsonBuffer jsonBuffer;

	JsonObject& root = jsonBuffer.createObject();
	JsonObject& report = root.createNestedObject("report");
	report["beaconMac"] = beaconMac;
	JsonArray& probes = report.createNestedArray("probes");

	for(int i = 0; i < probeCount; i++)
	{
		JsonObject& probe = probes.createNestedObject();
		probe["rssi"] = probeArray[i].rssi;
		probe["channel"] = probeArray[i].channel;
		probe["dstMac"] = probeArray[i].dstMac;
		probe["srcMac"] = probeArray[i].srcMac;
		probe["bssidMac"] = probeArray[i].bssidMac;
		probe["ssid"] = probeArray[i].ssid;
		probe["capturedAt"] = probeArray[i].capturedAt;
	}

	String retJson;
	report.prettyPrintTo(retJson);
	return retJson;
}

#ifndef CONFIG_H
#define CONFIG_H

#define DISABLE	0
#define ENABLE  1

#define ON 0
#define OFF 1

// Internet connection settings
// Used in time setting and data http posting to server
#define AP_SSID     "Little John"
#define AP_PASSWORD "doublerainbow"

// Timezone setting
#define TIMEZONE -3

// Only look for probe request from specific MAC address
#define EXCLUSIVE_MAC_MODE   DISABLE
#define EXCLUSIVE_MAC_TARGET "00:00:00:00:00:00"

#define HTTP_POST_MODE  DISABLE
#define NTP_UPDATE ENABLE

// Settings for SD card
#define SD_LOGGING_MODE         ENABLE
#define SD_READER_PIN   0

#define MAX_FILE_SIZE 1024*1024*100

// Interval waited until switch listening to next channel
#define CHANNEL_HOP_INTERVAL_MS 1000

// Server data sending triggers
#define BUFFER_TIMEOUT    360
#define MAX_QUEUE_SIZE    300

#define MAX_PROBES_PER_REPORT 10
#define HTTP_SERVER_ADDRESS   "http://api-sherlock.rappdo.com/api/1/report/new.json"

#define PRINT_PROBES ENABLE

// 16-> Main embedded led
// 2->  Secondary embbeded led

#define POWER_LED_PIN 0
#define PROBE_RECEIVED_LED_PIN 15
#define WIFI_STA_LED_PIN 2

#define BUTTON_PIN 16
#define BUTTON_MODE ENABLE
#define BUTTON_LED_PIN 0
#define MAX_PEOPLE 8

#endif
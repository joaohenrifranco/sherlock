#ifndef CONFIG_H
#define CONFIG_H

#define DISABLE	0
#define ENABLE  1

// Internet connection settings
// Used in time setting and data http posting to server
#define AP_SSID     "test"
#define AP_PASSWORD "doublerainbow"

// Timezone setting
#define TIMEZONE -3

// Only look for probe request from specific MAC address
#define EXCLUSIVE_MAC_MODE   DISABLE
#define EXCLUSIVE_MAC_TARGET "00:00:00:00:00:00"

#define HTTP_POST_MODE  ENABLE

// Settings for SD card
#define SD_MODE         ENABLE
// GPIO Pin where SD Module CS is connected
#define SD_READER_PIN   0

#define MAX_FILE_SIZE 1024*1024*100

// Interval waited until switch listening to next channel
#define CHANNEL_HOP_INTERVAL_MS 1000

// Server data sending triggers
#define BUFFER_TIMEOUT    360
#define MAX_QUEUE_SIZE    300

#define MAX_PROBES_PER_REPORT 10
#define HTTP_SERVER_ADDRESS   "http://api-sherlock.rappdo.com/api/1/report/new.json"
//#define HTTP_SERVER_ADDRESS   "https://requestb.in/1l8yui31"

#define PRINT_PROBES ENABLE

#endif
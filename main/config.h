/*
  mitsubishi2mqtt - Mitsubishi Heat Pump to MQTT control for Home Assistant.
  Copyright (c) 2023 gysmo38, dzungpv, shampeon, endeavour, jascdk, chrdavis, alekslyse.  All right reserved.
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include "FS.h" // SPIFFS for store config
#ifdef ESP32
#include <WiFi.h> // WIFI for ESP32
#include <WiFiUdp.h>
#include <ESPmDNS.h>  // mDNS for ESP32
#include <AsyncTCP.h> // ESPAsyncWebServer for ESP32
#include "SPIFFS.h"   // ESP32 SPIFFS for store config
#define U_PART U_SPIFFS
extern "C"
{
#include "freertos/FreeRTOS.h" //AsyncMqttClient
#include "freertos/timers.h"
}
#include "esp_ota_ops.h"       // IDF api to get app version
#include "esp_app_format.h"    // IDF api to get app version infor
#include "nvs.h"               // nvs ESP IDF to save wifi settings
#include "nvs_flash.h"         //nvs ESP IDF to save wifi settings
#include "esp_flash_encrypt.h" //encryption check
#include "esp_secure_boot.h"   //secure boot check
#else
#include <ESP8266WiFi.h> // WIFI for ESP8266
#include <WiFiClient.h>
#include <ESP8266mDNS.h> // mDNS for ESP8266
#include <ESPAsyncTCP.h> // ESPAsyncWebServer for ESP8266
#define U_PART U_FS
#endif
#include <espMqttClient.h>     // espMqttClient
#include <ESPAsyncWebServer.h> // ESPAsyncWebServer
AsyncWebServer server(80);     // Async Web server
#define WEBSOCKET_ENABLE 1     // Uncomment to enable websocket
#ifdef WEBSOCKET_ENABLE
AsyncWebSocket ws("/ws"); // Async Web socket
#endif
AsyncEventSource events("/events"); // Create an Event Source on /events

#include <ArduinoJson.h> // json to process MQTT: ArduinoJson 6.11.4
#include <DNSServer.h>   // DNS for captive portal
#include <math.h>        // for rounding to Fahrenheit values
#include <ArduinoOTA.h>  // for OTA
// #define ARDUINO_OTA 1      // Uncomment to enable Arduino OTA over ip

#include <HeatPump.h> // SwiCago library: https://github.com/SwiCago/HeatPump
#include <Ticker.h>   // for LED status (Using a Wemos D1-Mini)
#include "time.h"     // time lib

#ifdef ESP32
// Let Encrypt isrgrootx1.pem
const char rootCA_LE[] = R"====(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)====";
#endif

// wifi, mqtt and heatpump client instances
MqttClient* mqttClient = nullptr; // espMqttClient support both in secure and secure mqtt

#ifdef ESP32
TimerHandle_t mqttReconnectTimer; // timer for esp32 AsyncMqttClient
TimerHandle_t wifiReconnectTimer; // timer for esp32 AsyncMqttClient
#elif defined(ESP8266)
WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker mqttReconnectTimer; // timer for esp8266 AsyncMqttClient
Ticker wifiReconnectTimer; // timer for esp8266 AsyncMqttClient
#endif

bool mqtt_connected = false;
uint8_t mqtt_disconnect_reason = -1;

Ticker ticker;

// Captive portal variables, only used for config page
const byte DNS_PORT = 53;
IPAddress apIP(8, 8, 8, 8);
IPAddress netMsk(255, 255, 255, 0);
DNSServer dnsServer;

boolean captive = false;
boolean mqtt_config = false;
boolean wifi_config = false;
boolean remoteTempActive = false;

// HVAC
HeatPump hp;
unsigned long lastAliveMsgSend;
unsigned long lastTempSend;
unsigned long lastMqttRetry;
unsigned long lastHpSync;
unsigned int hpConnectionRetries;
unsigned int hpConnectionTotalRetries;
unsigned long lastRemoteTemp;

// Local state
StaticJsonDocument<JSON_OBJECT_SIZE(12)> rootInfo;
String wifi_list = "";                            // cache wifi scan result
const String localApIpUrl = "http://8.8.8.8";     // a string version of the local IP with http, used for redirecting clients to your webpage
String unique_id = "";                            // cache board unique id

// Web OTA
int uploaderror = 0;
size_t ota_content_len;

// For Asynce reboot after timeout
bool requestReboot = false;
unsigned long requestRebootTime = 0;
// For MQTTAsync hp update callback
bool requestHpUpdate = false;
unsigned long requestHpUpdateTime = 0;
// For async wifi scan
bool requestWifiScan = false;
unsigned long requestWifiScanTime = 0;
#define WIFI_SCAN_PERIOD 120000
unsigned lastWifiScanMillis;

const PROGMEM char *m2mqtt_version = "2025.06.15";

// Define global variables for files
int HP_TX = 0; // variable for the ESP32 custom TX pin, 0 is the defautl and it use hardware serial 0
int HP_RX = 0; // variable for the ESP32 custom RX pin, 0 is the defautl and it use hardware serial 0
#ifdef ESP32
const PROGMEM char *wifi_conf = "/wifi.json";
const PROGMEM char *mqtt_conf = "/mqtt.json";
const PROGMEM char *unit_conf = "/unit.json";
const PROGMEM char *console_file = "/console.log";
const PROGMEM char *others_conf = "/others.json";
// pinouts
const PROGMEM uint8_t blueLedPin = 2; // The ESP32 has an internal blue LED at D2 (GPIO 02)
#else
const PROGMEM char *wifi_conf = "wifi.json";
const PROGMEM char *mqtt_conf = "mqtt.json";
const PROGMEM char *unit_conf = "unit.json";
const PROGMEM char *console_file = "console.log";
const PROGMEM char *others_conf = "others.json";
// pinouts
const PROGMEM uint8_t blueLedPin = LED_BUILTIN; // Onboard LED = digital pin 2 "D4" (blue LED on WEMOS D1-Mini)
#endif
const PROGMEM uint8_t redLedPin = 0;

// Define global variables for network
const PROGMEM char *appName = "Mitsubishi2MQTT";
const PROGMEM char *manufacturer = "MITSUBISHI ELECTRIC";
const PROGMEM char *model = "HVAC MITSUBISHI";
const PROGMEM char *hostnamePrefix = "HVAC-";
static const char *const TAG = "mitsu_cn105";


unsigned long wifi_timeout;
unsigned long wifi_reconnect_timeout;
unsigned long mqtt_reconnect_timeout;

String hostname = "";
String ap_ssid;
String ap_pwd;
String ota_pwd;

// WiFi static addresses
String wifi_static_ip;
String wifi_static_gateway_ip;
String wifi_static_subnet;
String wifi_static_dns_ip;

// HTML Response
char* html_response = NULL;
unsigned int html_resp_length = 0;

// time and time zone
String ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;
const time_t min_valid_date = 1748000000;  // min time to consider device date valid
time_t device_boot_time = 0;

// Define global variables for MQTT
String mqtt_fn;
String mqtt_server;
String mqtt_port;
String mqtt_username;
String mqtt_password;
String mqtt_topic = "mitsubishi2mqtt";
String mqtt_root_ca_cert;
String mqtt_client_id;
const PROGMEM char *mqtt_payload_available = "online";
const PROGMEM char *mqtt_payload_unavailable = "offline";
const PROGMEM char *default_mqtt_topic = "mitsubishi2mqtt";

// Define global variables for Others settings
bool others_haa;
String others_haa_topic;
String timezone = "ICT-7"; // Set timezone to Vietnam Standard Time

// Define global variables for HA topics
String ha_power_set_topic;
String ha_mode_set_topic;
String ha_temp_set_topic;
String ha_remote_temp_set_topic;
String ha_fan_set_topic;
String ha_vane_set_topic;
String ha_wide_vane_set_topic;
String ha_state_topic;
String ha_system_info_topic;
String ha_system_set_topic;
String ha_system_setting_info;
String ha_system_setting_request;
String ha_system_setting_respond;
String ha_debug_pckts_topic;
String ha_debug_pckts_set_topic;
String ha_debug_logs_topic;
String ha_debug_logs_set_topic;
String ha_discovery_topic;
String ha_custom_packet;
String ha_availability_topic;
String ha_birth_topic;
String hvac_name;

// login
String login_username = "admin";
String login_password;

// debug mode logs, when true, will send all debug messages to topic heatpump_debug_logs_topic
// this can also be set by sending "on" to heatpump_debug_set_topic
bool _debugModeLogs = false;
// debug mode packets, when true, will send all packets received from the heatpump to topic heatpump_debug_packets_topic
// this can also be set by sending "on" to heatpump_debug_set_topic
bool _debugModePckts = false;

bool _webPanelDisable = false; // when true allow disable webpanel when using mqtt

// Customization
const uint8_t min_temp = 16; // Minimum temperature, in your selected unit, check value from heatpump remote control
const uint8_t max_temp = 31; // Maximum temperature, in your selected unit, check value from heatpump remote control
String temp_step = "1";      // Temperature setting step, check value from heatpump remote control

// sketch settings
const PROGMEM uint32_t PREVENT_UPDATE_INTERVAL_MS = 3000;  // interval to prevent HA setting change after send settings to HP
const PROGMEM uint32_t SEND_ALIVE_MSG_INTERVAL_MS = 30000; // interval send mqtt keep alive message
const PROGMEM uint32_t SEND_ROOM_TEMP_INTERVAL_MS = 30000; // 45 seconds (anything less may cause bouncing)
const PROGMEM uint32_t WIFI_RETRY_INTERVAL_MS = 300000;
const PROGMEM uint32_t WIFI_RECONNECT_INTERVAL_MS = 10000;     // 10 seconds
const PROGMEM uint32_t CHECK_REMOTE_TEMP_INTERVAL_MS = 300000; // 5 minutes
const PROGMEM uint32_t MQTT_RETRY_INTERVAL_MS = 1000;          // 1 second
const PROGMEM uint32_t MQTT_RECONNECT_INTERVAL_MS = 10000;     // 10 seconds
const PROGMEM uint32_t REBOOT_REQUEST_INTERVAL_MS = 1000;      // 1 seconds
const PROGMEM uint32_t HP_RETRY_INTERVAL_MS = 1000;            // 1 second
const PROGMEM uint32_t HP_MAX_RETRIES = 10;                    // Double the interval between retries up to this many times, then keep retrying forever at that maximum interval.
// Default values give a final retry interval of 1000ms * 2^10, which is 1024 seconds, about 17 minutes.

// temp settings
bool useFahrenheit = false;
// support heat mode settings, some model do not support heat mode
bool supportHeatMode = true;
// support quiet mode for fan settings, some model do not support quite mode: MSZ-GE35VA, MSZ-GE71VA, MSZ-GE25VA https://github.com/SwiCago/HeatPump/issues/162
bool supportQuietMode = true;

// info in status page
String version = "";         /* Git version */
String app_name = "";        /* App name */
String build_date_time = ""; /* build date time from app desc */

// Multi language support, all store in the flash, just change in the Unit settings
byte system_language_index = 0; // default language index 0:en
// #define MITSU2MQTT_EN_ONLY 1   // un comment to enable English only for debug
#include "language_util.h" // Multi languages support

// Languages supported. Note: the order is important and must match locale_translations.h
#if defined(MITSU2MQTT_TEST) || defined(MITSU2MQTT_EN_ONLY)
// in Debug mode use one language (en) to save flash memory needed for the tests
const char *const languages[] = {MITSU2MQTT_LOCALE_EN};
const char *const language_names[] = {"English"};
#elif defined(MITSU2MQTT_VI_ONLY)
const char *const languages[] = {MITSU2MQTT_LOCALE_VI};
const char *const language_names[] = {"Tiếng Việt"};
#else
const char *const languages[] = {
    MITSU2MQTT_LOCALE_EN,
    MITSU2MQTT_LOCALE_VI,
    MITSU2MQTT_LOCALE_DA,
    MITSU2MQTT_LOCALE_DE,
    MITSU2MQTT_LOCALE_ES,
    MITSU2MQTT_LOCALE_FR,
    MITSU2MQTT_LOCALE_IT,
    MITSU2MQTT_LOCALE_JA,
    MITSU2MQTT_LOCALE_ZH,
    MITSU2MQTT_LOCALE_CA
};
const char *const language_names[] = {
    "English",
    "Tiếng Việt",
    "Dansk",
    "Deutsch",
    "Español",
    "Français",
    "Italiano",
    "日本語",
    "中文",
    "Català"
};
#endif

const byte ENT_ROOM_TEMPERATURE = 0;
const byte ENT_CONNECTION_STATE = 1;
const byte ENT_UP_TIME = 2;
const byte ENT_FREE_HEAP = 3;
const byte ENT_RSSI = 4;
const byte ENT_BSSI = 5;
const byte ENT_COMPR_FRQ = 6;
const byte ENT_RESTART_BTN = 7;
const byte ENT_WEB_PANEL = 8;

const byte MAX_ENTITY_ID = ENT_WEB_PANEL;

static constexpr uint8_t NUM_LANGUAGES = sizeof(languages) / sizeof(const char *);

// #define METRICS 1   // un comment to enable Prometheus exporter

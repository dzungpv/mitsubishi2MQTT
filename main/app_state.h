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

#pragma once

#include "platform.h"

// Shared firmware state. Definitions live in app_state.cpp so every module
// sees one copy. File names, JSON keys, and topic suffixes are a device contract.

extern AsyncWebServer server;
#ifdef WEBSOCKET_ENABLE
extern AsyncWebSocket ws;
#endif
extern AsyncEventSource events;

extern MqttClient *mqttClient;
extern bool mqtt_connected;
extern uint8_t mqtt_disconnect_reason;
#ifdef ESP32
extern TimerHandle_t mqttReconnectTimer;
#elif defined(ESP8266)
extern WiFiEventHandler wifiConnectHandler;
extern WiFiEventHandler wifiDisconnectHandler;
extern Ticker mqttReconnectTimer;
#endif
extern Ticker ticker;

extern bool captive;
extern bool mqtt_config;
extern bool wifi_config;
extern bool remoteTempActive;

extern HeatPump hp;
extern unsigned long lastAliveMsgSend;
extern unsigned long lastHpSync;
extern unsigned int hpConnectionRetries;
extern unsigned int hpConnectionTotalRetries;
extern unsigned long lastRemoteTemp;

extern StaticJsonDocument<JSON_OBJECT_SIZE(12)> rootInfo;

constexpr byte DNS_PORT = 53;
extern IPAddress apIP;
extern IPAddress netMsk;
extern DNSServer dnsServer;
extern const String localApIpUrl;
extern String wifi_list;
extern String unique_id;

extern bool requestReboot;
extern unsigned long requestRebootTime;
extern bool requestHpUpdate;
extern unsigned long requestHpUpdateTime;
extern bool requestWifiScan;
extern unsigned long requestWifiScanTime;
constexpr uint32_t WIFI_SCAN_PERIOD = 120000;
extern unsigned long lastWifiScanMillis;

extern const char *m2mqtt_version;
extern int HP_TX;
extern int HP_RX;

// SPIFFS paths. ESP32 keeps the leading slash; ESP8266 does not.
extern const char *wifi_conf;
extern const char *mqtt_conf;
extern const char *unit_conf;
extern const char *console_file;
extern const char *others_conf;

extern const uint8_t blueLedPin;
extern const uint8_t blueLedDisabled;

extern const char *appName;
extern const char *manufacturer;
extern const char *model;
extern const char *hostnamePrefix;

extern unsigned long wifi_timeout;
extern unsigned long wifi_reconnect_timeout;
extern unsigned long mqtt_reconnect_timeout;

extern String hostname;
extern String ap_ssid;
extern String ap_pwd;
extern String ota_pwd;
extern String wifi_static_ip;
extern String wifi_static_gateway_ip;
extern String wifi_static_subnet;
extern String wifi_static_dns_ip;

extern String ntpServer;
constexpr long gmtOffset_sec = 3600;
constexpr int daylightOffset_sec = 3600;
constexpr time_t min_valid_date = 1748000000;
extern time_t device_boot_time;

extern String mqtt_fn;
extern String mqtt_server;
extern String mqtt_port;
extern String mqtt_username;
extern String mqtt_password;
extern String mqtt_topic;
extern String mqtt_root_ca_cert;
extern String mqtt_client_id;
extern const char *mqtt_payload_available;
extern const char *mqtt_payload_unavailable;
extern const char *default_mqtt_topic;

extern bool others_haa;
extern String others_haa_topic;
extern String timezone;

extern String ha_power_set_topic;
extern String ha_mode_set_topic;
extern String ha_temp_set_topic;
extern String ha_remote_temp_set_topic;
extern String ha_fan_set_topic;
extern String ha_vane_set_topic;
extern String ha_wide_vane_set_topic;
extern String ha_state_topic;
extern String ha_system_info_topic;
extern String ha_system_set_topic;
extern String ha_system_setting_info;
extern String ha_system_setting_request;
extern String ha_system_setting_respond;
extern String ha_debug_pckts_topic;
extern String ha_debug_pckts_set_topic;
extern String ha_debug_logs_topic;
extern String ha_debug_logs_set_topic;
extern String ha_custom_packet;
extern String ha_availability_topic;
extern String ha_birth_topic;

extern String login_username;
extern String login_password;

extern bool debugLogsEnabled;
extern bool debugPacketsEnabled;
extern bool webPanelDisabled;

constexpr uint8_t min_temp = 16;
constexpr uint8_t max_temp = 31;
extern String temp_step;
extern bool useFahrenheit;
extern bool supportHeatMode;
extern bool supportQuietMode;

constexpr uint32_t PREVENT_UPDATE_INTERVAL_MS = 3000;
constexpr uint32_t SEND_ALIVE_MSG_INTERVAL_MS = 30000;
constexpr uint32_t WIFI_RETRY_INTERVAL_MS = 300000;
constexpr uint32_t WIFI_RECONNECT_INTERVAL_MS = 10000;
constexpr uint32_t CHECK_REMOTE_TEMP_INTERVAL_MS = 300000;
constexpr uint32_t MQTT_RECONNECT_INTERVAL_MS = 10000;
constexpr uint32_t REBOOT_REQUEST_INTERVAL_MS = 1000;
constexpr uint32_t HP_RETRY_INTERVAL_MS = 1000;
constexpr uint32_t HP_MAX_RETRIES = 10;

extern String version;
extern String build_date_time;
extern byte system_language_index;

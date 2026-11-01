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

#include "app_state.h"

AsyncWebServer server(80);
#ifdef WEBSOCKET_ENABLE
AsyncWebSocket ws("/ws");
#endif
AsyncEventSource events("/events");

MqttClient *mqttClient = nullptr;
bool mqtt_connected = false;
uint8_t mqtt_disconnect_reason = 255; // historical initializer was -1
#ifdef ESP32
TimerHandle_t mqttReconnectTimer = nullptr;
#elif defined(ESP8266)
WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker mqttReconnectTimer;
#endif
Ticker ticker;

bool captive = false;
bool mqtt_config = false;
bool wifi_config = false;
bool remoteTempActive = false;

HeatPump hp;
unsigned long lastAliveMsgSend = 0;
unsigned long lastHpSync = 0;
unsigned int hpConnectionRetries = 0;
unsigned int hpConnectionTotalRetries = 0;
unsigned long lastRemoteTemp = 0;

StaticJsonDocument<JSON_OBJECT_SIZE(12)> rootInfo;

IPAddress apIP(8, 8, 8, 8);
IPAddress netMsk(255, 255, 255, 0);
DNSServer dnsServer;
const String localApIpUrl = "http://8.8.8.8";
String wifi_list;
String unique_id;

bool requestReboot = false;
unsigned long requestRebootTime = 0;
bool requestHpUpdate = false;
unsigned long requestHpUpdateTime = 0;
bool requestWifiScan = false;
unsigned long requestWifiScanTime = 0;
unsigned long lastWifiScanMillis = 0;

const char *m2mqtt_version = "2025.10.30";
int HP_TX = 0;
int HP_RX = 0;

#ifdef ESP32
const char *wifi_conf = "/wifi.json";
const char *mqtt_conf = "/mqtt.json";
const char *unit_conf = "/unit.json";
const char *console_file = "/console.log";
const char *others_conf = "/others.json";
const uint8_t blueLedPin = 2;
const uint8_t blueLedDisabled = LOW;
#else
const char *wifi_conf = "wifi.json";
const char *mqtt_conf = "mqtt.json";
const char *unit_conf = "unit.json";
const char *console_file = "console.log";
const char *others_conf = "others.json";
const uint8_t blueLedPin = LED_BUILTIN;
const uint8_t blueLedDisabled = HIGH;
#endif

const char *appName = "Mitsubishi2MQTT";
const char *manufacturer = "MITSUBISHI ELECTRIC";
const char *model = "HVAC MITSUBISHI";
const char *hostnamePrefix = "HVAC-";

unsigned long wifi_timeout = 0;
unsigned long wifi_reconnect_timeout = 0;
unsigned long mqtt_reconnect_timeout = 0;

String hostname;
String ap_ssid;
String ap_pwd;
String ota_pwd;
String wifi_static_ip;
String wifi_static_gateway_ip;
String wifi_static_subnet;
String wifi_static_dns_ip;

String ntpServer = "pool.ntp.org";
time_t device_boot_time = 0;

String mqtt_fn;
String mqtt_server;
String mqtt_port;
String mqtt_username;
String mqtt_password;
String mqtt_topic = "mitsubishi2mqtt";
String mqtt_root_ca_cert;
String mqtt_client_id;
const char *mqtt_payload_available = "online";
const char *mqtt_payload_unavailable = "offline";
const char *default_mqtt_topic = "mitsubishi2mqtt";

bool others_haa = false;
String others_haa_topic;
String timezone = "ICT-7";

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
String ha_custom_packet;
String ha_availability_topic;
String ha_birth_topic;

String login_username = "admin";
String login_password;

bool debugLogsEnabled = false;
bool debugPacketsEnabled = false;
bool webPanelDisabled = false;

String temp_step = "1";
bool useFahrenheit = false;
bool supportHeatMode = true;
bool supportQuietMode = true;

String version;
String build_date_time;
byte system_language_index = 0;

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

#include "config.h"            // config file
#include "htmls/html_common.h"       // common code HTML (like header, footer)
#include "htmls/javascript_common.h" // common code javascript (like refresh page)
#include "htmls/html_init.h"         // code html for initial config
#include "htmls/html_menu.h"         // code html for menu
#include "htmls/html_pages.h"        // code html for pages
#ifdef METRICS
#include "htmls/html_metrics.h" // prometheus metrics
#endif

// Start header for build with IDF and Platformio
bool loadWifi();
bool loadMqtt();
bool loadUnit();
bool loadOthers();
void saveMqtt(String mqttFn, const String& mqttHost, String mqttPort, const String& mqttUser, const String& mqttPwd, String mqttTopic, const String& mqttRootCaCert);
void saveUnit(String tempUnit, String supportMode, String supportFanMode, String loginPassword, String tempStep, String languageIndex);
void saveWifi(String apSsid, const String& apPwd, String hostName, const String& otaPwd);
void saveOthers(const String& haa, const String& haat, const String& debugPckts, const String& debugLogs, const String& txPin, const String& rxPin);
void saveCurrentOthers();
void initCaptivePortal();
void initMqtt();
void initOTA();
void setDefaults();
boolean initWifi();
void sendWrappedHTML(AsyncWebServerRequest *request, const String &content);
void handleNotFound(AsyncWebServerRequest *request);
void handleSaveWifiAndMqtt(AsyncWebServerRequest *request);
void handleReboot(AsyncWebServerRequest *request);
void handleRoot(AsyncWebServerRequest *request);
void handleInitSetup(AsyncWebServerRequest *request);
void handleSetup(AsyncWebServerRequest *request);
void handleOthers(AsyncWebServerRequest *request);
void handleMqtt(AsyncWebServerRequest *request);
void handleUnit(AsyncWebServerRequest *request);
void handleWifi(AsyncWebServerRequest *request);
void handleStatus(AsyncWebServerRequest *request);
void handleControl(AsyncWebServerRequest *request);
void handleMetrics(AsyncWebServerRequest *request);
void handleLogin(AsyncWebServerRequest *request);
void handleUpgrade(AsyncWebServerRequest *request);
void handleUploadDone(AsyncWebServerRequest *request);
void handleUploadLoop(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final);
void write_log(const String& log);
heatpumpSettings change_states(AsyncWebServerRequest *request, heatpumpSettings settings);
void readHeatPumpSettings();
void hpSettingsChanged();
String hpGetMode(heatpumpSettings hpSettings);
String hpGetAction(heatpumpStatus hpStatus, heatpumpSettings hpSettings);
void hpStatusChanged(heatpumpStatus currentStatus);
void hpCheckRemoteTemp();
void hpPacketDebug(byte *packet, unsigned int length, const char *packetDirection);
void hpSendLocalState();
void mqttCallback(const char *topic, const uint8_t *payload, const unsigned int length);
void sendHaConfig();
void mqttConnect();
bool connectWifi();
float toFahrenheit(float fromCelcius);
float toCelsius(float fromFahrenheit);
float convertCelsiusToLocalUnit(float temperature, bool isFahrenheit);
float convertLocalUnitToCelsius(float temperature, bool isFahrenheit);
String getTemperatureScale();
String getId();
bool is_authenticated(AsyncWebServerRequest *request);
void checkLogin(AsyncWebServerRequest *request);
// AsyncMQTT
#ifdef ESP32
void WiFiEvent(WiFiEvent_t event);
#elif defined(ESP8266)
void onWifiConnect(const WiFiEventStationModeGotIP &event);
void onWifiDisconnect(const WiFiEventStationModeDisconnected &event);
#endif

void onMqttConnect(bool sessionPresent);
void onMqttDisconnect(espMqttClientTypes::DisconnectReason reason);
void onMqttSubscribe(uint16_t packetId, const espMqttClientTypes::SubscribeReturncode* codes, size_t len);
void onMqttUnsubscribe(uint16_t packetId);
void onMqttMessage(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, size_t len, size_t index, size_t total);
void onMqttPublish(uint16_t packetId);

String getValueBySeparator(const String& data, char separator, int index);
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);

void sendRebootRequest(unsigned long nextSeconds);
void checkRebootRequest();
void checkHpUpdateRequest();
void checkWifiScanRequest();

String getWifiOptions(bool send);
void getWifiList();
bool isSecureEnable();
String getBuildDatetime();
String getAppVersion();
void initNVS();
String getCurrentTime();
time_t getUpTimeSeconds();
String getUpTime();
void tick(); // led blink tick
void factoryReset();
void otaUpdateProgress(size_t prg, size_t sz);
String getFanModeFromHa(String modeFromHa);
String getFanModeFromHp(String modeFromHp);
String getWifiBSSID();
void sendDeviceInfo();
// End  header for build with IDF and Platformio

#ifdef ESP8266
#define ESP_LOGE(tag, format, ...)
#define ESP_LOGW(tag, format, ...)
#define ESP_LOGI(tag, format, ...)
#define ESP_LOGD(tag, format, ...)
#define ESP_LOGV(tag, format, ...)
#endif

void setup()
{
  // Start serial for debug before HVAC connect to serial
  Serial.begin(115200);
#ifdef ESP32
  Serial.setDebugOutput(true);
  initNVS();
#endif
  ESP_LOGD(TAG, "Starting  %s", appName);
  // Mount SPIFFS filesystem
  if (SPIFFS.begin())
  {
    ESP_LOGD(TAG, "Mounted file system");
  }
  else
  {
    ESP_LOGD(TAG, "Failed to mount FS -> formating");
    SPIFFS.format();
    if (SPIFFS.begin())
    {
      ESP_LOGD(TAG, "Mounted file system after formating");
    }
  }
  // set led pin as output
  pinMode(blueLedPin, OUTPUT);
  ticker.attach(1, tick); // every seconds
  setDefaults();
  wifi_config = loadWifi();
  if (hostname.isEmpty())
  {
    // set default hostname
    hostname += hostnamePrefix;
    hostname += getId();
  }
  loadOthers();
  loadUnit();
#ifdef ESP32
  WiFi.setHostname(hostname.c_str());
#else
  WiFi.hostname(hostname.c_str());
#endif
  wifi_reconnect_timeout = 0;
  mqtt_reconnect_timeout = 0;
  if (loadMqtt())
  {
    // write_log("Starting MQTT");
    //  setup HA topics
    ha_power_set_topic = mqtt_topic + "/" + mqtt_fn + "/power/set";
    ha_mode_set_topic = mqtt_topic + "/" + mqtt_fn + "/mode/set";
    ha_temp_set_topic = mqtt_topic + "/" + mqtt_fn + "/temp/set";
    ha_remote_temp_set_topic = mqtt_topic + "/" + mqtt_fn + "/remote_temp/set";
    ha_fan_set_topic = mqtt_topic + "/" + mqtt_fn + "/fan/set";
    ha_vane_set_topic = mqtt_topic + "/" + mqtt_fn + "/vane/set";
    ha_wide_vane_set_topic = mqtt_topic + "/" + mqtt_fn + "/wide-vane/set";
    ha_settings_topic = mqtt_topic + "/" + mqtt_fn + "/settings";
    ha_state_topic = mqtt_topic + "/" + mqtt_fn + "/state";
    //
    ha_system_setting_info = mqtt_topic + "/" + mqtt_fn + "/system/info"; // for device info
    ha_debug_pckts_topic = mqtt_topic + "/" + mqtt_fn + "/debug/packets";
    ha_debug_pckts_set_topic = mqtt_topic + "/" + mqtt_fn + "/debug/packets/set";
    ha_debug_logs_topic = mqtt_topic + "/" + mqtt_fn + "/debug/logs";
    ha_debug_logs_set_topic = mqtt_topic + "/" + mqtt_fn + "/debug/logs/set";
    //
    ha_custom_packet = mqtt_topic + "/" + mqtt_fn + "/custom/send";
    ha_availability_topic = mqtt_topic + "/" + mqtt_fn + "/availability";
    ha_system_set_topic = mqtt_topic + "/" + mqtt_fn + "/system/set"; // for control over mqtt
    if (others_haa)
    {
      //ha_config_topic = others_haa_topic + "/climate/" + mqtt_fn + "/config";
      ha_config_topic = others_haa_topic + "/climate/hvac_" + getId() + "/config";
    }
    else
    {
      //ha_config_topic = "homeassistant/climate/" + mqtt_fn + "/config";
      ha_config_topic = "homeassistant/climate//hvac_" + getId() + "/config";
    }
    // startup mqtt connection
    initMqtt();
  }
  else
  {
    // write_log("Not found MQTT config go to configuration page");
  }
  if (initWifi())
  {
    if (SPIFFS.exists(console_file))
    {
      SPIFFS.remove(console_file);
    }
    // write_log("Starting Mitsubishi2MQTT");
    MDNS.begin(hostname); // DNS service for .local address access
    // Web interface
    server.on("/", handleRoot);
    server.on("/control", handleControl);
    server.on("/setup", handleSetup);
    server.on("/mqtt", handleMqtt);
    server.on("/wifi", handleWifi);
    server.on("/unit", handleUnit);
    server.on("/status", handleStatus);
    server.on("/others", handleOthers);
#ifdef METRICS
    server.on("/metrics", handleMetrics);
#endif
    server.onNotFound(handleNotFound);
    if (login_password.length() > 0)
    {
      server.on("/login", handleLogin);
    }
    if (!isSecureEnable())
    {
      server.on("/upgrade", handleUpgrade);
      server.on("/upload", WebRequestMethod::HTTP_ANY, handleUploadDone, handleUploadLoop);
#ifdef ESP32
      Update.onProgress(otaUpdateProgress);
#endif
    }
    // web socket
#ifdef WEBSOCKET_ENABLE
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);
#endif
    // event source client
    events.onConnect([](AsyncEventSourceClient *client)
                     { client->send("hello!", NULL, millis(), 1000); });
    server.addHandler(&events);
    server.begin();
    ESP_LOGD(TAG, "Connection to HVAC. Stop serial log.");
    // write_log("Connection to HVAC");
    lastMqttRetry = 0;
    lastHpSync = 0;
    hpConnectionRetries = 0;
    hpConnectionTotalRetries = 0;
    hp.setSettingsChangedCallback(hpSettingsChanged);
    hp.setStatusChangedCallback(hpStatusChanged);
    hp.setPacketCallback(hpPacketDebug);
    // Allow Remote/Panel
    hp.enableExternalUpdate();
    hp.enableAutoUpdate();
#if defined(ESP32)
    if (HP_TX > 0 && HP_RX > 0)
    {
      hp.connect(&Serial1, HP_RX, HP_TX);
    }
    else
    {
      hp.connect(&Serial);
      esp_log_level_set("*", ESP_LOG_NONE); // disable all logs because we use UART0 connect to HP
    }
#else
    hp.connect(&Serial);
#endif
    hp.setFastSync(true); // enable fast sync because we are not care about timer nad other package
    heatpumpStatus currentStatus = hp.getStatus();
    heatpumpSettings currentSettings = hp.getSettings();
    rootInfo["roomTemperature"] = convertCelsiusToLocalUnit(currentStatus.roomTemperature, useFahrenheit);
    rootInfo["temperature"] = convertCelsiusToLocalUnit(currentSettings.temperature, useFahrenheit);
    rootInfo["fan"] = getFanModeFromHp(currentSettings.fan);
    rootInfo["vane"] = currentSettings.vane;
    rootInfo["wideVane"] = currentSettings.wideVane;
    rootInfo["mode"] = hpGetMode(currentSettings);
    rootInfo["action"] = hpGetAction(currentStatus, currentSettings);
    rootInfo["compressorFrequency"] = currentStatus.compressorFrequency;
    lastTempSend = millis();
    MDNS.addService("http", "tcp", 80);
  }
  else
  {
    dnsServer.start(DNS_PORT, "*", apIP);
    initCaptivePortal();
  }
#ifdef ARDUINO_OTA
  initOTA();
#endif
}

void otaUpdateProgress(size_t prg, size_t sz)
{
  ESP_LOGD(TAG, "Progress: %d%%\n", (prg * 100) / ota_content_len);
}

void tick()
{
  // toggle state
  int state = digitalRead(blueLedPin); // get the current state of GPIO2 pin
  digitalWrite(blueLedPin, !state);    // set pin to the opposite state
}

bool loadWifi()
{
  ap_ssid = "";
  ap_pwd = "";
  if (!SPIFFS.exists(wifi_conf))
  {
    ESP_LOGE(TAG, "Wifi config file not exist!");
    return false;
  }
  File configFile = SPIFFS.open(wifi_conf, "r");
  if (!configFile)
  {
    ESP_LOGE(TAG, "Failed to open wifi config file");
    return false;
  }
  size_t size = configFile.size();
  if (size > 1024)
  {
    ESP_LOGE(TAG, "Wifi config file size is too large");
    return false;
  }
  // Allocate document capacity.
  const size_t capacity = JSON_OBJECT_SIZE(8) + 130 + 4*(16 /*ipv4 addr*/ + 15 /*max key size*/);
  DynamicJsonDocument doc(capacity);
  deserializeJson(doc, configFile);
  // Check key exist to prevent data is "null"
  if (doc.containsKey("hostname"))
  {
    hostname = doc["hostname"].as<String>();
  }
  else
  {
    hostname.clear();
  }
  if (doc.containsKey("ap_ssid"))
  {
    ap_ssid = doc["ap_ssid"].as<String>();
  }
  else
  {
    ap_ssid.clear();
  }
  if (doc.containsKey("ap_pwd"))
  {
    ap_pwd = doc["ap_pwd"].as<String>();
  }
  else
  {
    ap_pwd.clear();
  }
  if (doc.containsKey("ota_pwd"))
  {
    ota_pwd = doc["ota_pwd"].as<String>();
  }
  else
  {
    ota_pwd.clear();
  }

  // static IP configuration
  if (doc.containsKey("static_ip"))
  {
    wifi_static_ip = doc["static_ip"].as<String>();
  }
  else
  {
    wifi_static_ip.clear();
  }
  if (doc.containsKey("static_gw_ip"))
  {
    wifi_static_gateway_ip = doc["static_gw_ip"].as<String>();
  }
  else
  {
    wifi_static_gateway_ip.clear();
  }
  if (doc.containsKey("static_subnet"))
  {
    wifi_static_subnet = doc["static_subnet"].as<String>();
  }
  else
  {
    wifi_static_subnet.clear();
  }
  if (doc.containsKey("static_dns_ip"))
  {
    wifi_static_dns_ip = doc["static_dns_ip"].as<String>();
  }
  else
  {
    wifi_static_dns_ip.clear();
  }

  return true;
}

bool loadMqtt()
{
  if (!SPIFFS.exists(mqtt_conf))
  {
    ESP_LOGE(TAG, "MQTT config file not exist!");
    return false;
  }
  // write_log("Loading MQTT configuration");
  File configFile = SPIFFS.open(mqtt_conf, "r");
  if (!configFile)
  {
    ESP_LOGE(TAG, "Failed to open MQTT config file");
    return false;
  }

  size_t size = configFile.size();
  if (size > 5000)
  {
    ESP_LOGE(TAG, "Wifi config file size is too large");
    return false;
  }
  // Allocate document capacity.
  const size_t capacity = JSON_OBJECT_SIZE(7) + 400 + 2650;
  DynamicJsonDocument doc(capacity);
  deserializeJson(doc, configFile);
  // check key to prevent data is "null" if not exist
  if (doc.containsKey("mqtt_fn"))
  {
    mqtt_fn = doc["mqtt_fn"].as<String>();
    ;
  }
  else
  {
    mqtt_fn = "";
  }
  if (doc.containsKey("mqtt_host"))
  {
    mqtt_server = doc["mqtt_host"].as<String>();
  }
  else
  {
    mqtt_server = "";
  }
  if (doc.containsKey("mqtt_port"))
  {
    mqtt_port = doc["mqtt_port"].as<String>();
  }
  else
  {
    mqtt_port = "";
  }
  if (doc.containsKey("mqtt_user"))
  {
    mqtt_username = doc["mqtt_user"].as<String>();
  }
  else
  {
    mqtt_username = "";
  }
  if (doc.containsKey("mqtt_pwd"))
  {
    mqtt_password = doc["mqtt_pwd"].as<String>();
  }
  else
  {
    mqtt_password = "";
  }
  if (doc.containsKey("mqtt_topic"))
  {
    mqtt_topic = doc["mqtt_topic"].as<String>();
  }
  else
  {
    mqtt_topic = "";
  }
  if (doc.containsKey("mqtt_root_ca_cert"))
  {
    mqtt_root_ca_cert = doc["mqtt_root_ca_cert"].as<String>();
  }
  else
  {
    mqtt_root_ca_cert = "";
  }
  // write_log("=== START DEBUG MQTT ===");
  // write_log("Friendly Name" + mqtt_fn);
  // write_log("IP Server " + mqtt_server);
  // write_log("IP Port " + mqtt_port);
  // write_log("Username " + mqtt_username);
  // write_log("Password " + mqtt_password);
  // write_log("Topic " + mqtt_topic);
  // write_log("=== END DEBUG MQTT ===");

  mqtt_config = (!mqtt_fn.isEmpty() && !mqtt_server.isEmpty() && !mqtt_port.isEmpty() && !mqtt_topic.isEmpty());
  return true;
}

bool loadUnit()
{
  if (!SPIFFS.exists(unit_conf))
  {
    // Serial.println(F("Unit config file not exist!"));
    return false;
  }
  File configFile = SPIFFS.open(unit_conf, "r");
  if (!configFile)
  {
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024)
  {
    return false;
  }
  // Allocate document capacity.
  const size_t capacity = JSON_OBJECT_SIZE(3) + 200;
  DynamicJsonDocument doc(capacity);
  deserializeJson(doc, configFile);
  // unit
  String unit_tempUnit = doc["unit_tempUnit"].as<String>();
  if (unit_tempUnit == "fah")
    useFahrenheit = true;
  temp_step = doc["temp_step"].as<String>();
  // mode
  String supportMode = doc["support_mode"].as<String>();
  if (supportMode == "nht")
    supportHeatMode = false;
  // quiet
  String quietMode = doc["quiet_mode"].as<String>();
  if (quietMode == "nqm")
    supportQuietMode = false;
  // prevent login password is "null" if not exist key
  if (doc.containsKey("login_password"))
  {
    login_password = doc["login_password"].as<String>();
  }
  else
  {
    login_password = "";
  }
  if (doc.containsKey("language_index"))
  {
    system_language_index = doc["language_index"].as<byte>();
  }
  else
  {
    system_language_index = 0;
  }
  return true;
}

bool loadOthers()
{
  if (!SPIFFS.exists(others_conf))
  {
    // Serial.println(F("Others config file not exist!"));
    return false;
  }
  File configFile = SPIFFS.open(others_conf, "r");
  if (!configFile)
  {
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024)
  {
    return false;
  }
  // Allocate document capacity.
  const size_t capacity = JSON_OBJECT_SIZE(9) + 366;
  DynamicJsonDocument doc(capacity);
  deserializeJson(doc, configFile);
  // unit
  String unit_tempUnit = doc["unit_tempUnit"].as<String>();
  if (unit_tempUnit == "fah")
    useFahrenheit = true;
  others_haa_topic = doc["haat"].as<String>();
  String haa = doc["haa"].as<String>();
  String debugPckts = doc["debugPckts"].as<String>();
  String debugLogs = doc["debugLogs"].as<String>();
  if (strcmp(haa.c_str(), "OFF") == 0)
  {
    others_haa = false;
  }
  if (strcmp(debugPckts.c_str(), "ON") == 0)
  {
    _debugModePckts = true;
  }
  if (strcmp(debugLogs.c_str(), "ON") == 0)
  {
    _debugModeLogs = true;
  }
  // custom tx rx pin
  if (doc.containsKey("txPin") && doc.containsKey("rxPin")) // check key to prevent data is "null" if not exist
  {
    String txPin = doc["txPin"].as<String>();
    String rxPin = doc["rxPin"].as<String>();
    if (!txPin.isEmpty() && !rxPin.isEmpty())
    {
      HP_TX = atoi(txPin.c_str());
      HP_RX = atoi(rxPin.c_str());
    }
  }
  if (doc.containsKey("tz")) // check key to prevent data is "null" if not exist
  {
    timezone = doc["tz"].as<String>();
  }
  if (doc.containsKey("ntp"))
  {
    ntpServer = doc["ntp"].as<String>();
  }
  return true;
}

void saveMqtt(String mqttFn, const String& mqttHost, String mqttPort, const String& mqttUser, const String& mqttPwd, String mqttTopic, const String& mqttRootCaCert)
{
  // Allocate document capacity.
  const size_t capacity = JSON_OBJECT_SIZE(7) + 400 + 2560;
  DynamicJsonDocument doc(capacity);
  // if mqtt port is empty, we use default port
  if (mqttPort.isEmpty())
  {
    mqttPort = "1883";
  }
  if (mqttFn.isEmpty())
  {
    // set default fn
    mqttFn += hostnamePrefix;
    mqttFn += getId();
    mqttFn.toLowerCase();
  }
  if (mqttTopic.isEmpty())
  {
    // set default topic
    mqttTopic += getId();
    mqttTopic.toLowerCase();
  }
  doc["mqtt_fn"] = mqttFn;
  doc["mqtt_host"] = mqttHost;
  doc["mqtt_port"] = mqttPort;
  doc["mqtt_user"] = mqttUser;
  doc["mqtt_pwd"] = mqttPwd;
  doc["mqtt_topic"] = mqttTopic;
  if (!mqttRootCaCert.isEmpty() && mqttRootCaCert.length() > 500)
  {
    doc["mqtt_root_ca_cert"] = mqttRootCaCert;
  }
  File configFile = SPIFFS.open(mqtt_conf, "w");
  if (!configFile)
  {
    ESP_LOGD(TAG, "Failed to open config file for writing");
  }
  serializeJson(doc, configFile);
  configFile.close();
}

void saveUnit(String tempUnit, String supportMode, String supportFanMode, String loginPassword, String tempStep, String languageIndex)
{
  // Allocate document capacity.
  const size_t capacity = JSON_OBJECT_SIZE(6) + 200;
  DynamicJsonDocument doc(capacity);
  // if temp unit is empty, we use default celcius
  if (tempUnit.isEmpty())
    tempUnit = "cel";
  doc["unit_tempUnit"] = tempUnit;
  // if tempStep is empty, we use default 1
  if (tempStep.isEmpty())
    tempStep = 1;
  doc["temp_step"] = tempStep;
  // if support mode is empty, we use default all mode
  if (supportMode.isEmpty())
    supportMode = "all";
  doc["support_mode"] = supportMode;
  // if support fan mode is empty, we use default all mode
  if (supportFanMode.isEmpty())
    supportFanMode = "allf";
  doc["quiet_mode"] = supportFanMode;
  // if login password is empty, we use empty
  if (loginPassword.isEmpty())
    loginPassword = "";

  doc["login_password"] = loginPassword;
  if (languageIndex.isEmpty())
    languageIndex = "0";
  doc["language_index"] = languageIndex;
  File configFile = SPIFFS.open(unit_conf, "w");
  if (!configFile)
  {
    // Serial.println(F("Failed to open config file for writing"));
  }
  serializeJson(doc, configFile);
  configFile.close();
}

void saveWifi(String apSsid, const String& apPwd, String hostName, const String& otaPwd, const String& local_ip, const String& gw_ip, const String& subnet_ip, const String& dns_ip)
{
  // Allocate document capacity.
  const size_t capacity = JSON_OBJECT_SIZE(8) + 130 + 4*(16 /*ipv4 addr*/ + 15 /*max key size*/);
  DynamicJsonDocument doc(capacity);
  if (hostName.isEmpty())
  {
    hostName = hostname;
  }
  doc["ap_ssid"] = apSsid;
  doc["ap_pwd"] = apPwd;
  doc["hostname"] = hostName;
  doc["ota_pwd"] = otaPwd;
  if (!local_ip.isEmpty() && local_ip.length() > 6 && !gw_ip.isEmpty() && gw_ip.length() > 6 && !subnet_ip.isEmpty() && subnet_ip.length() > 6) {
      doc["static_ip"] = local_ip;
      doc["static_gw_ip"] = gw_ip;
      doc["static_subnet"] = subnet_ip;
      if (!dns_ip.isEmpty() && dns_ip.length() > 6) {
          doc["static_dns_ip"] = dns_ip;
      }
  }
  File configFile = SPIFFS.open(wifi_conf, "w");
  if (!configFile)
  {
    ESP_LOGD(TAG, "Failed to open wifi file for writing");
  }
  serializeJson(doc, configFile);
  configFile.close();
}

void saveOthers(const String& haa, const String& haat, const String& debugPckts, const String& debugLogs, const String& txPin, const String& rxPin, const String& tz, const String &ntp)
{
  // Allocate document capacity.
  const size_t capacity = JSON_OBJECT_SIZE(9) + 366;
  DynamicJsonDocument doc(capacity);
  doc["haa"] = haa;
  doc["haat"] = haat;
  doc["debugPckts"] = debugPckts;
  doc["debugLogs"] = debugLogs;
  doc["txPin"] = txPin;
  doc["rxPin"] = rxPin;
  doc["tz"] = tz;
  doc["ntp"] = ntp;
  File configFile = SPIFFS.open(others_conf, "w");
  if (!configFile)
  {
    ESP_LOGD(TAG, "Failed to open other config file for writing");
  }
  serializeJson(doc, configFile);
  configFile.close();
}

void saveCurrentOthers()
{
  String haa = others_haa ? "ON" : "OFF";
  String debugPckts = _debugModePckts ? "ON" : "OFF";
  String debugLogs = _debugModeLogs ? "ON" : "OFF";
  saveOthers(haa, others_haa_topic, debugPckts, debugLogs, String(HP_TX), String(HP_RX), timezone, ntpServer);
}

// Initialize captive portal page
void initCaptivePortal()
{
  ESP_LOGD(TAG, "Starting captive portal");
  // Required
  server.on("/connecttest.txt", [](AsyncWebServerRequest *request)
            { request->redirect("http://logout.net"); }); // windows 11 captive portal workaround
  server.on("/wpad.dat", [](AsyncWebServerRequest *request)
            { request->send(404); }); // Honestly don't understand what this is but a 404 stops win 10 keep calling this repeatedly and panicking the esp32 :)

  // Background responses: Probably not all are Required, but some are. Others might speed things up?
  // A Tier (commonly used by modern systems)
  server.on("/generate_204", [](AsyncWebServerRequest *request)
            { request->redirect(localApIpUrl); }); // android captive portal redirect
  server.on("/redirect", [](AsyncWebServerRequest *request)
            { request->redirect(localApIpUrl); }); // microsoft redirect
  server.on("/hotspot-detect.html", [](AsyncWebServerRequest *request)
            { request->redirect(localApIpUrl); }); // apple call home
  server.on("/canonical.html", [](AsyncWebServerRequest *request)
            { request->redirect(localApIpUrl); }); // firefox captive portal call home
  server.on("/success.txt", [](AsyncWebServerRequest *request)
            { request->send(200); }); // firefox captive portal call home
  server.on("/ncsi.txt", [](AsyncWebServerRequest *request)
            { request->redirect(localApIpUrl); }); // windows call home

  server.on("/", handleInitSetup);
  server.on("/save", handleSaveWifiAndMqtt);
  server.on("/reboot", handleReboot);
  server.on("/control", handleControl);
  server.on("/status", handleStatus);
  if (!isSecureEnable())
  {
    server.on("/upgrade", handleUpgrade);
    server.on("/upload", WebRequestMethod::HTTP_ANY, handleUploadDone, handleUploadLoop);
#ifdef ESP32
    Update.onProgress(otaUpdateProgress);
#endif
  }
  server.onNotFound(handleNotFound);
  // web socket
#ifdef WEBSOCKET_ENABLE
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
#endif
  server.begin();
  captive = true;
}

void initMqtt()
{
  ESP_LOGD(TAG, "Setup Async Mqtt...");
  if (mqttClient != nullptr)
  {
    delete mqttClient;
    mqttClient = nullptr;
  }
  if (atoi(mqtt_port.c_str()) == 8883)
  {
#ifdef ESP32
    mqttClient = static_cast<MqttClient *>(new espMqttClientSecure);
    if (!mqtt_root_ca_cert.isEmpty() && mqtt_root_ca_cert.length() > 500)
    {
      static_cast<espMqttClientSecure *>(mqttClient)->setCACert(mqtt_root_ca_cert.c_str());
    }
    else
    {
      static_cast<espMqttClientSecure *>(mqttClient)->setCACert(rootCA_LE);
    }
    static_cast<espMqttClientSecure *>(mqttClient)->onConnect(onMqttConnect);
    static_cast<espMqttClientSecure *>(mqttClient)->onDisconnect(onMqttDisconnect);
    static_cast<espMqttClientSecure *>(mqttClient)->onSubscribe(onMqttSubscribe);
    static_cast<espMqttClientSecure *>(mqttClient)->onUnsubscribe(onMqttUnsubscribe);
    static_cast<espMqttClientSecure *>(mqttClient)->onMessage(onMqttMessage);
    static_cast<espMqttClientSecure *>(mqttClient)->onPublish(onMqttPublish);

    static_cast<espMqttClientSecure *>(mqttClient)->setServer(mqtt_server.c_str(), atoi(mqtt_port.c_str()));
    static_cast<espMqttClientSecure *>(mqttClient)->setCredentials(mqtt_username.c_str(), mqtt_password.c_str());
    static_cast<espMqttClientSecure *>(mqttClient)->setClientId(mqtt_client_id.c_str());
    static_cast<espMqttClientSecure *>(mqttClient)->setWill(ha_availability_topic.c_str(), 1, false, mqtt_payload_unavailable);
#endif
  }
  else
  {
    mqttClient = static_cast<MqttClient *>(new espMqttClient);
    static_cast<espMqttClient*>(mqttClient)->onConnect(onMqttConnect);
    static_cast<espMqttClient*>(mqttClient)->onDisconnect(onMqttDisconnect);
    static_cast<espMqttClient*>(mqttClient)->onSubscribe(onMqttSubscribe);
    static_cast<espMqttClient*>(mqttClient)->onUnsubscribe(onMqttUnsubscribe);
    static_cast<espMqttClient *>(mqttClient)->onMessage(onMqttMessage);
    static_cast<espMqttClient *>(mqttClient)->onPublish(onMqttPublish);

    static_cast<espMqttClient *>(mqttClient)->setServer(mqtt_server.c_str(), atoi(mqtt_port.c_str()));
    static_cast<espMqttClient *>(mqttClient)->setCredentials(mqtt_username.c_str(), mqtt_password.c_str());
    static_cast<espMqttClient *>(mqttClient)->setClientId(mqtt_client_id.c_str());
    static_cast<espMqttClient *>(mqttClient)->setWill(ha_availability_topic.c_str(), 1, false, mqtt_payload_unavailable);
  }

  const char *apipch = mqtt_server.c_str();
  IPAddress apip;
  if (apip.fromString(apipch))
  { // try to parse into the IPAddress
    // print the parsed IPAddress
    ESP_LOGD(TAG, "Connecting to MQTT server IP: %s, port: %s", apipch, mqtt_port.c_str());
  }
  else
  {
    ESP_LOGD(TAG, "UnParsable IP");
  }
}

// Enable OTA only when connected as a client.
#ifdef ARDUINO_OTA
void initOTA()
{
  // write_log("Start OTA Listener");
  ArduinoOTA.setHostname(hostname.c_str());
  if (ota_pwd.length() > 0)
  {
    ArduinoOTA.setPassword(ota_pwd.c_str());
  }
  ArduinoOTA.onStart([]()
                     {
                       // write_log("Start");
                     });
  ArduinoOTA.onEnd([]()
                   {
                     // write_log("\nEnd");
                   });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        {
                          //    write_log("Progress: %u%%\r", (progress / (total / 100)));
                        });
  ArduinoOTA.onError([](ota_error_t error)
                     {
      //    write_log("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR)
      {
        ESP_LOGE(TAG, "Auth Failed");
      }
      else if (error == OTA_BEGIN_ERROR)
      {
        ESP_LOGE(TAG, "Begin Failed");
      }
      else if (error == OTA_CONNECT_ERROR)
      {
        ESP_LOGE(TAG, "Connect Failed");
      }
      else if (error == OTA_RECEIVE_ERROR)
      {
        ESP_LOGE(TAG, "Receive Failed");
      }
      else if (error == OTA_END_ERROR)
      {
        ESP_LOGE(TAG, "End Failed");
      } });
  ArduinoOTA.begin();
}
#endif

void setDefaults()
{
  ap_ssid = "";
  ap_pwd = "";
  others_haa = true;
  others_haa_topic = "homeassistant";
  mqtt_client_id = getId();
  mqtt_client_id.toLowerCase();
}

boolean initWifi()
{
  bool connectWifiSuccess = true;
  if (!ap_ssid.isEmpty())
  {
    // wifi connection handle
#ifdef ESP32
    mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(mqttConnect));
    // wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(connectWifi));
    WiFi.onEvent(WiFiEvent);
#elif defined(ESP8266)
    wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
    wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect); // timer for esp8266 AsyncMqttClient
#endif
    connectWifiSuccess = connectWifi();
    if (connectWifiSuccess)
    {
      return true;
    }
    else
    {
      // reset hostname back to default before starting AP mode for privacy
      hostname = hostnamePrefix;
      hostname += getId();
    }
  }
  ESP_LOGE(TAG, "Starting in AP mode, host name: %s", hostname.c_str());
  // WiFi.disconnect(true);
  // delay(1000);
  WiFi.mode(WIFI_AP);
  // delay(1000);
  wifi_timeout = millis() + WIFI_RETRY_INTERVAL_MS;
#ifdef ESP32
  WiFi.persistent(false); // fix crash esp32 https://github.com/espressif/arduino-esp32/issues/2025
#endif
  if (!connectWifiSuccess)
  {
    // Set AP password when falling back to AP on fail
    WiFi.softAP(hostname.c_str(), login_password.isEmpty() ? hostname.c_str() : login_password.c_str());
  }
  else
  {
    // First time setup does not require password
    WiFi.softAP(hostname.c_str());
  }
  delay(2000); // VERY IMPORTANT
  WiFi.softAPConfig(apIP, apIP, netMsk);
  ESP_LOGE(TAG, "IP address: %s", WiFi.softAPIP().toString().c_str());
  ticker.attach(0.2, tick); // Start LED to flash rapidly to indicate we are ready for setting up the wifi-connection (entered captive portal).
  WiFi.scanNetworks(true);
  delay(2000); // VERY IMPORTANT
  return false;
}

// Handler webserver response

void sendWrappedHTML(AsyncWebServerRequest *request, const String &content)
{
  String headerContent = FPSTR(html_common_header);
  String footerContent = FPSTR(html_common_footer);
  String toSend(headerContent);
  toSend += content;
  toSend += footerContent;
  toSend.replace(F("_UNIT_NAME_"), hostname);
  toSend.replace(F("_VERSION_"), getAppVersion());
  toSend.replace(F("_APP_NAME_"), appName);
  request->send_P(200, "text/html", toSend.c_str());
  headerContent = "";
  footerContent = "";
}

void handleNotFound(AsyncWebServerRequest *request)
{
  if (captive)
  {
    request->redirect(localApIpUrl);
  }
  else
  {
    String menuRootPage = FPSTR(html_menu_root);
    menuRootPage.replace(F("_SHOW_LOGOUT_"), (String)(login_password.length() > 0));
    // not show control button if hp not connected
    menuRootPage.replace(F("_SHOW_CONTROL_"), (String)(hp.isConnected()));
    sendWrappedHTML(request, menuRootPage);
  }
}

void handleSaveWifiAndMqtt(AsyncWebServerRequest *request)
{
  checkLogin(request);
  ESP_LOGD(TAG, "Saving wifi and mqtt config");
  if (request->hasArg("submit"))
  {
    String ssid = request->arg("ssid");
    if (ssid.isEmpty() and request->hasArg("network"))
    {
      ssid = request->arg("network"); // auto scan network
    }
    saveWifi(ssid, request->arg("psk"), request->arg("hn"), request->arg("otapwd"), request->arg("stip"), request->arg("stgw"), request->arg("stmask"), request->arg("stdns"));
    if (request->hasArg("mh"))
    {
      saveMqtt(request->arg("fn"), request->arg("mh"), request->arg("ml"), request->arg("mu"), request->arg("mp"), request->arg("mt"), "");
    }
    if (request->hasArg("language"))
    {
      saveUnit("", "", "", "", "", request->arg("language"));
    }
  }
  String initSavePage = FPSTR(html_init_save);
  initSavePage.replace(F("_CONFIG_ADDR_"), hostname + ".local");
  String countDown = FPSTR(count_down_script_init);
  countDown.replace(F("_HOST_NAME_"), hostname + ".local");
  // localize
  initSavePage.replace("_TXT_INIT_REBOOT_MES_1_", translatedWord(FL_(txt_init_reboot_mes_1)));
  initSavePage.replace("_TXT_INIT_REBOOT_MES_", translatedWord(FL_(txt_init_reboot_mes)));
  sendWrappedHTML(request, initSavePage + countDown);
  sendRebootRequest(2); // Reboot after 1 seconds
}

void handleReboot(AsyncWebServerRequest *request)
{
  checkLogin(request);
  ESP_LOGD(TAG, "Rebooting");
  String initRebootPage = FPSTR(html_init_reboot);
  // localize
  initRebootPage.replace("_TXT_INIT_REBOOT_", translatedWord(FL_(txt_init_reboot)));
  sendWrappedHTML(request, initRebootPage);
  sendRebootRequest(3); // Reboot after 3 seconds
}

void handleRoot(AsyncWebServerRequest *request)
{
  checkLogin(request);
  if (request->hasArg("REBOOT"))
  {
    String rebootPage = FPSTR(html_page_reboot);
    String countDown = FPSTR(count_down_script);
    // localize
    rebootPage.replace("_TXT_M_REBOOT_", translatedWord(FL_(txt_m_reboot)));
    sendWrappedHTML(request, rebootPage + countDown);
    sendRebootRequest(3); // Reboot after 3 seconds
  }
  else
  {
    String menuRootPage = FPSTR(html_menu_root);
    // localize
    menuRootPage.replace("_TXT_HOME_PAGE_", translatedWord(FL_(txt_home_page)));
    menuRootPage.replace("_TXT_CONTROL_", translatedWord(FL_(txt_control)));
    menuRootPage.replace("_TXT_SETUP_", translatedWord(FL_(txt_setup)));
    menuRootPage.replace("_TXT_STATUS_", translatedWord(FL_(txt_status)));
    menuRootPage.replace("_TXT_FW_UPGRADE_", translatedWord(FL_(txt_firmware_upgrade)));
    menuRootPage.replace("_TXT_REBOOT_", translatedWord(FL_(txt_reboot)));
    menuRootPage.replace("_TXT_LOGOUT_", translatedWord(FL_(txt_logout)));
    // set data
    menuRootPage.replace(F("_SHOW_LOGOUT_"), (String)(login_password.length() > 0));
    // not show control button if hp not connected
    menuRootPage.replace(F("_SHOW_CONTROL_"), (String)(hp.isConnected()));
    sendWrappedHTML(request, menuRootPage);
  }
}

void handleInitSetup(AsyncWebServerRequest *request)
{
  getWifiList();
  String initSetupPage = FPSTR(html_init_setup);
  String unitScriptWs = FPSTR(unit_script_ws);
  // localize
  initSetupPage.replace("_TXT_INIT_TITLE_", translatedWord(FL_(txt_init_title)));
  initSetupPage.replace("_TXT_WIFI_TITLE_", translatedWord(FL_(txt_wifi_title)));
  initSetupPage.replace("_TXT_UNIT_LANGUAGE_", translatedWord(FL_(txt_unit_language)));
  initSetupPage.replace("_TXT_WIFI_SSID_ENTER_", translatedWord(FL_(txt_wifi_ssid_enter)));
  initSetupPage.replace("_TXT_WIFI_SSID_SELECT_", translatedWord(FL_(txt_wifi_ssid_select)));
  initSetupPage.replace("_TXT_WIFI_SSID_", translatedWord(FL_(txt_wifi_ssid)));
  initSetupPage.replace("_TXT_WIFI_PSK_", translatedWord(FL_(txt_wifi_psk)));
  initSetupPage.replace("_TXT_WIFI_STATIC_IP_", translatedWord(FL_(txt_wifi_static_ip)));
  initSetupPage.replace("_TXT_WIFI_STATIC_GW_", translatedWord(FL_(txt_wifi_static_gw)));
  initSetupPage.replace("_TXT_WIFI_STATIC_MASK_", translatedWord(FL_(txt_wifi_static_mask)));
  initSetupPage.replace("_TXT_WIFI_STATIC_DNS_", translatedWord(FL_(txt_wifi_static_dns)));
  initSetupPage.replace("_TXT_MQTT_TITLE_", translatedWord(FL_(txt_mqtt_title)));
  initSetupPage.replace("_TXT_MQTT_PH_USER_", translatedWord(FL_(txt_mqtt_ph_user)));
  initSetupPage.replace("_TXT_MQTT_PH_PWD_", translatedWord(FL_(txt_mqtt_ph_pwd)));
  initSetupPage.replace("_TXT_MQTT_HOST_", translatedWord(FL_(txt_mqtt_host)));
  initSetupPage.replace("_TXT_MQTT_PORT_DESC", translatedWord(FL_(txt_mqtt_port_desc)));
  initSetupPage.replace("_TXT_MQTT_PORT_", translatedWord(FL_(txt_mqtt_port)));
  initSetupPage.replace("_TXT_MQTT_USER_", translatedWord(FL_(txt_mqtt_user)));
  initSetupPage.replace("_TXT_MQTT_PASSWORD_", translatedWord(FL_(txt_mqtt_password)));
  initSetupPage.replace("_TXT_SAVE_", translatedWord(FL_(txt_save)));
  initSetupPage.replace("_TXT_FIRMWARE_UPGRADE_", translatedWord(FL_(txt_firmware_upgrade)));
  // set the data
  // language
  String language_list;
  for (uint8_t i = 0; i < NUM_LANGUAGES; i++)
  {
    language_list += "<option value='";
    language_list += i;
    language_list += "'";
    if (i == system_language_index)
    {
      language_list += "selected";
    }
    language_list += ">";
    language_list += language_names[i];
    language_list += "</option>";
  }
  initSetupPage.replace(F("_LANGUAGE_OPTIONS_"), language_list);
  // display wifi list
  String wifiOptions = getWifiOptions(false);
  if (!wifiOptions.isEmpty())
  {
    initSetupPage.replace(F("_WIFI_OPTIONS_"), wifiOptions);
  }
  initSetupPage.replace(F("_WIFI_STATIC_IP_"), wifi_static_ip);
  initSetupPage.replace(F("_WIFI_STATIC_GW_"), wifi_static_gateway_ip);
  initSetupPage.replace(F("_WIFI_STATIC_MASK_"), wifi_static_subnet);
  initSetupPage.replace(F("_WIFI_STATIC_DNS_"), wifi_static_dns_ip);
  initSetupPage.replace(F("_FRIENDLY_NAME_"), mqtt_fn);
  initSetupPage.replace(F("_MQTT_HOST_"), mqtt_server);
  initSetupPage.replace(F("_MQTT_PORT_"), String(mqtt_port));
  initSetupPage.replace(F("_MQTT_USER_"), mqtt_username);
  initSetupPage.replace(F("_MQTT_PASSWORD_"), mqtt_password);
  initSetupPage.replace(F("_MQTT_TOPIC_"), mqtt_topic);
  initSetupPage.replace(F("_FIRMWARE_UPLOAD_"), isSecureEnable() ? "'hidden' style='display: none;' disabled" : "");
  // serve the page
  sendWrappedHTML(request, unitScriptWs + initSetupPage);
  initSetupPage = "";
}

void handleSetup(AsyncWebServerRequest *request)
{
  checkLogin(request);
  if (request->hasArg("RESET"))
  {
    String resetPage = FPSTR(html_page_reset);
    // localize
    resetPage.replace("_TXT_M_RESET_1_", translatedWord(FL_(txt_m_reset_1)));
    resetPage.replace("_TXT_M_RESET_", translatedWord(FL_(txt_m_reset)));
    String countDown = FPSTR(count_down_script);
    sendWrappedHTML(request, resetPage + countDown);
    factoryReset();
    sendRebootRequest(5); // Reboot after 5 seconds
  }
  else
  {
    String menuSetupPage = FPSTR(html_menu_setup);
    // localize
    menuSetupPage.replace("_TXT_SETUP_PAGE_", translatedWord(FL_(txt_setup_page)));
    menuSetupPage.replace("_TXT_MQTT_", translatedWord(FL_(txt_mqtt)));
    menuSetupPage.replace("_TXT_WIFI_", translatedWord(FL_(txt_wifi)));
    menuSetupPage.replace("_TXT_UNIT_", translatedWord(FL_(txt_unit)));
    menuSetupPage.replace("_TXT_OTHERS_", translatedWord(FL_(txt_others)));
    menuSetupPage.replace("_TXT_RESET_CONFIRM_", translatedWord(FL_(txt_reset_confirm)));
    menuSetupPage.replace("_TXT_RESET_", translatedWord(FL_(txt_reset)));
    menuSetupPage.replace("_TXT_BACK_", translatedWord(FL_(txt_back)));
    sendWrappedHTML(request, menuSetupPage);
  }
}

void handleOthers(AsyncWebServerRequest *request)
{
  checkLogin(request);
  if (request->hasArg("save"))
  {
    saveOthers(request->arg("HAA"), request->arg("haat"), request->arg("DebugPckts"), request->arg("DebugLogs"), request->arg("tx_pin"), request->arg("rx_pin"), request->arg("tz"), request->arg("ntp"));
    String saveRebootPage = FPSTR(html_page_save_reboot);
    // localize
    saveRebootPage.replace("_TXT_M_SAVE_", translatedWord(FL_(txt_m_save)));
    String countDown = FPSTR(count_down_script);
    sendWrappedHTML(request, saveRebootPage + countDown);
    sendRebootRequest(5); // Reboot after 5 seconds
  }
  else
  {
    String othersPage = FPSTR(html_page_others);
    // localize
    othersPage.replace("_TXT_OTHERS_TITLE_", translatedWord(FL_(txt_others_title)));
    othersPage.replace("_TXT_OTHERS_HAAUTO_", translatedWord(FL_(txt_others_haauto)));
    othersPage.replace("_TXT_OTHERS_HATOPIC_", translatedWord(FL_(txt_others_hatopic)));
    othersPage.replace("_TXT_OTHERS_DEBUG_PCKTS_", translatedWord(FL_(txt_others_debug_packets)));
    othersPage.replace("_TXT_OTHERS_DEBUG_LOGS_", translatedWord(FL_(txt_others_debug_log)));
    othersPage.replace("_TXT_OTHERS_TIME_ZONE_", translatedWord(FL_(txt_others_tz)));
    othersPage.replace("_TXT_OTHER_NTP_SERVER_", translatedWord(FL_(txt_others_ntp_server)));
    othersPage.replace("_SEE_TZ_LIST", translatedWord(FL_(txt_others_tz_list)));
    othersPage.replace("_TXT_TX_PIN_", translatedWord(FL_(txt_others_tx_pin)));
    othersPage.replace("_TXT_RX_PIN_", translatedWord(FL_(txt_others_rx_pin)));
    othersPage.replace("_TXT_F_ON_", translatedWord(FL_(txt_f_on)));
    othersPage.replace("_TXT_F_OFF_", translatedWord(FL_(txt_f_off)));
    othersPage.replace("_TXT_SAVE_", translatedWord(FL_(txt_save)));
    othersPage.replace("_TXT_BACK_", translatedWord(FL_(txt_back)));
    // set data
    othersPage.replace("_HAA_TOPIC_", others_haa_topic);
    othersPage.replace("_TX_PIN_", String(HP_TX));
    othersPage.replace("_RX_PIN_", String(HP_RX));
    othersPage.replace("_TIME_ZONE_", timezone);
    othersPage.replace("_NTP_SERVER_", String(ntpServer));
    if (others_haa)
    {
      othersPage.replace("_HAA_ON_", "selected");
    }
    else
    {
      othersPage.replace("_HAA_OFF_", "selected");
    }
    if (_debugModePckts)
    {
      othersPage.replace("_DEBUG_PCKTS_ON_", "selected");
    }
    else
    {
      othersPage.replace("_DEBUG_PCKTS_OFF_", "selected");
    }
    if (_debugModeLogs)
    {
      othersPage.replace("_DEBUG_LOGS_ON_", "selected");
    }
    else
    {
      othersPage.replace("_DEBUG_LOGS_OFF_", "selected");
    }
    sendWrappedHTML(request, othersPage);
  }
}

void handleMqtt(AsyncWebServerRequest *request)
{
  checkLogin(request);
  if (request->hasArg("save"))
  {
    saveMqtt(request->arg("fn"), request->arg("mh"), request->arg("ml"), request->arg("mu"), request->arg("mp"), request->arg("mt"), request->arg("mrcc"));
    String saveRebootPage = FPSTR(html_page_save_reboot);
    // localize
    saveRebootPage.replace("_TXT_M_SAVE_", translatedWord(FL_(txt_m_save)));
    String countDown = FPSTR(count_down_script);
    sendWrappedHTML(request, saveRebootPage + countDown);
    sendRebootRequest(5); // Reboot after 5 seconds
  }
  else
  {
    String mqttPage = FPSTR(html_page_mqtt);
    // localize
    mqttPage.replace("_TXT_MQTT_TITLE_", translatedWord(FL_(txt_mqtt_title)));
    mqttPage.replace("_TXT_MQTT_FN_DESC_", translatedWord(FL_(txt_mqtt_fn_desc)));
    mqttPage.replace("_TXT_MQTT_FN_", translatedWord(FL_(txt_mqtt_fn)));
    mqttPage.replace("_TXT_MQTT_PH_USER_", translatedWord(FL_(txt_mqtt_ph_user)));
    mqttPage.replace("_TXT_MQTT_PH_PWD_", translatedWord(FL_(txt_mqtt_ph_pwd)));
    mqttPage.replace("_TXT_MQTT_PH_TOPIC_", translatedWord(FL_(txt_mqtt_ph_topic)));
    mqttPage.replace("_TXT_MQTT_HOST_", translatedWord(FL_(txt_mqtt_host)));
    mqttPage.replace("_TXT_MQTT_PORT_DESC_", translatedWord(FL_(txt_mqtt_port_desc)));
    mqttPage.replace("_TXT_MQTT_PORT_", translatedWord(FL_(txt_mqtt_port)));
    mqttPage.replace("_TXT_MQTT_USER_", translatedWord(FL_(txt_mqtt_user)));
    mqttPage.replace("_TXT_MQTT_PASSWORD_", translatedWord(FL_(txt_mqtt_password)));
    mqttPage.replace("_TXT_MQTT_TOPIC_", translatedWord(FL_(txt_mqtt_topic)));
    mqttPage.replace("_TXT_MQTT_ROOT_CA_CERT_", translatedWord(FL_(txt_mqtt_root_ca_cert)));
    mqttPage.replace("_TXT_SAVE_", translatedWord(FL_(txt_save)));
    mqttPage.replace("_TXT_BACK_", translatedWord(FL_(txt_back)));
    // set data
    mqttPage.replace(F("_MQTT_FN_"), mqtt_fn);
    mqttPage.replace(F("_MQTT_HOST_"), mqtt_server);
    mqttPage.replace(F("_MQTT_PORT_"), String(mqtt_port));
    mqttPage.replace(F("_MQTT_USER_"), mqtt_username);
    mqttPage.replace(F("_MQTT_PASSWORD_"), mqtt_password);
    mqttPage.replace(F("_MQTT_TOPIC_"), mqtt_topic);
    mqttPage.replace(F("_MQTT_ROOT_CA_CERT_"), mqtt_root_ca_cert);
    sendWrappedHTML(request, mqttPage);
  }
}

void handleUnit(AsyncWebServerRequest *request)
{
  checkLogin(request);
  if (request->hasArg("save"))
  {
    String loginPassword = request->arg("lpw");
    String confirmLoginPassword = request->arg("lpwc");
    if (loginPassword == confirmLoginPassword)
    {
      saveUnit(request->arg("tu"), request->arg("md"), request->arg("mdf"), loginPassword, request->arg("temp_step"), request->arg("language"));
      String saveRebootPage = FPSTR(html_page_save_reboot);
      // localize
      saveRebootPage.replace("_TXT_M_SAVE_", translatedWord(FL_(txt_m_save)));
      String countDown = FPSTR(count_down_script);
      sendWrappedHTML(request, saveRebootPage + countDown);
      sendRebootRequest(5); // Reboot after 5 seconds
    }
    else
    {
      String saveRebootPage = FPSTR(html_page_save_reboot);
      // localize
      saveRebootPage.replace("_TXT_M_SAVE_", translatedWord(FL_(txt_unit_password_not_match)));
      String countDown = FPSTR(count_down_script);
      sendWrappedHTML(request, saveRebootPage + countDown);
    }
  }
  else
  {
    String unitPage = FPSTR(html_page_unit);
    String unitScriptWs = FPSTR(unit_script_ws);
    // localize
    unitPage.replace("_TXT_UNIT_TITLE_", translatedWord(FL_(txt_unit_title)));
    unitPage.replace("_TXT_UNIT_LANGUAGE_", translatedWord(FL_(txt_unit_language)));
    unitPage.replace("_TXT_UNIT_TEMP_", translatedWord(FL_(txt_unit_temp)));
    unitPage.replace("_TXT_UNIT_STEPTEMP_", translatedWord(FL_(txt_unit_steptemp)));
    unitPage.replace("_TXT_UNIT_FAN_MODES_", translatedWord(FL_(txt_unit_fan_modes)));
    unitPage.replace("_TXT_UNIT_FAN_MODES_", translatedWord(FL_(txt_unit_fan_modes)));
    unitPage.replace("_TXT_UNIT_MODES_", translatedWord(FL_(txt_unit_modes)));
    unitPage.replace("_TXT_UNIT_LOGIN_USERNAME_", translatedWord(FL_(txt_unit_login_username)));
    unitScriptWs.replace("_TXT_UNIT_PASSWORD_NOT_MATCH_", translatedWord(FL_(txt_unit_password_not_match)));
    unitPage.replace("_TXT_UNIT_PASSWORD_CONFIRM_", translatedWord(FL_(txt_unit_password_confirm)));
    unitPage.replace("_TXT_UNIT_PASSWORD_", translatedWord(FL_(txt_unit_password)));
    unitPage.replace("_TXT_F_CELSIUS_", translatedWord(FL_(txt_f_celsius)));
    unitPage.replace("_TXT_F_FH_", translatedWord(FL_(txt_f_fh)));
    unitPage.replace("_TXT_F_ALLMODES_", translatedWord(FL_(txt_f_allmodes)));
    unitPage.replace("_TXT_F_NOHEAT_", translatedWord(FL_(txt_f_noheat)));
    unitPage.replace("_TXT_F_NOQUIET_", translatedWord(FL_(txt_f_noquiet)));
    unitPage.replace("_TXT_SAVE_", translatedWord(FL_(txt_save)));
    unitPage.replace("_TXT_BACK_", translatedWord(FL_(txt_back)));
    // set data
    // language
    String language_list;
    for (uint8_t i = 0; i < NUM_LANGUAGES; i++)
    {
      language_list += "<option value='";
      language_list += i;
      language_list += "'";
      if (i == system_language_index)
      {
        language_list += "selected";
      }
      language_list += ">";
      language_list += language_names[i];
      language_list += "</option>";
    }
    unitPage.replace(F("_LANGUAGE_OPTIONS_"), language_list);
    // temp
    if (useFahrenheit)
      unitPage.replace(F("_TU_FAH_"), F("selected"));
    else
      unitPage.replace(F("_TU_CEL_"), F("selected"));
    // step
    unitPage.replace(F("_TEMP_STEP_"), String(temp_step));
    // mode
    if (supportHeatMode)
      unitPage.replace(F("_MD_ALL_"), F("selected"));
    else
      unitPage.replace(F("_MD_NONHEAT_"), F("selected"));
    // fan quiet mode
    if (supportQuietMode)
      unitPage.replace(F("_MDF_ALL_"), F("selected"));
    else
      unitPage.replace(F("_MDF_NONQUIET_"), F("selected"));
    // login password
    unitPage.replace(F("_LOGIN_PASSWORD_"), login_password);
    sendWrappedHTML(request, unitScriptWs + unitPage);
  }
}

void handleWifi(AsyncWebServerRequest *request)
{
  checkLogin(request);
  if (request->hasArg("save"))
  {
    String ssid = request->arg("ssid");
    if (ssid.isEmpty() and request->hasArg("network"))
    {
      ssid = request->arg("network"); // auto scan network
    }
    ESP_LOGD(TAG, "handleWifi: %s", ssid.c_str());
    saveWifi(ssid, request->arg("psk"), request->arg("hn"), request->arg("otapwd"), request->arg("stip"), request->arg("stgw"), request->arg("stmask"), request->arg("stdns"));
    String saveRebootPage = FPSTR(html_page_save_reboot);
    // localize
    saveRebootPage.replace("_TXT_M_SAVE_", translatedWord(FL_(txt_m_save)));
    String countDown = FPSTR(count_down_script);
    sendWrappedHTML(request, saveRebootPage + countDown);
    sendRebootRequest(5); // reboot after 5 seconds
  }
  else
  {
    if (wifi_list.isEmpty() || millis() - lastWifiScanMillis > WIFI_SCAN_PERIOD) // only scan every WIFI_SCAN_PERIOD
    {
      requestWifiScan = true;
      requestWifiScanTime = millis() + 50;
    }
    String wifiPage = FPSTR(html_page_wifi);
    // localize
    wifiPage.replace("_TXT_WIFI_TITLE_", translatedWord(FL_(txt_wifi_title)));
    wifiPage.replace("_TXT_WIFI_HOST_DESC_", translatedWord(FL_(txt_wifi_hostname_desc)));
    wifiPage.replace("_TXT_WIFI_HOST_", translatedWord(FL_(txt_wifi_hostname)));
    wifiPage.replace("_TXT_WIFI_SSID_ENTER_", translatedWord(FL_(txt_wifi_ssid_enter)));
    wifiPage.replace("_TXT_WIFI_SSID_SELECT_", translatedWord(FL_(txt_wifi_ssid_select)));
    wifiPage.replace("_TXT_WIFI_SSID_", translatedWord(FL_(txt_wifi_ssid)));
    wifiPage.replace("_TXT_WIFI_PSK_", translatedWord(FL_(txt_wifi_psk)));
    wifiPage.replace("_TXT_WIFI_OTAP_", translatedWord(FL_(txt_wifi_otap)));
    wifiPage.replace("_TXT_WIFI_STATIC_IP_", translatedWord(FL_(txt_wifi_static_ip)));
    wifiPage.replace("_TXT_WIFI_STATIC_GW_", translatedWord(FL_(txt_wifi_static_gw)));
    wifiPage.replace("_TXT_WIFI_STATIC_MASK_", translatedWord(FL_(txt_wifi_static_mask)));
    wifiPage.replace("_TXT_WIFI_STATIC_DNS_", translatedWord(FL_(txt_wifi_static_dns)));
    wifiPage.replace("_TXT_SAVE_", translatedWord(FL_(txt_save)));
    wifiPage.replace("_TXT_BACK_", translatedWord(FL_(txt_back)));
    // set data
    String str_ap_ssid = ap_ssid;
    String str_ap_pwd = ap_pwd;
    String str_ota_pwd = ota_pwd;
    str_ap_ssid.replace("'", F("&apos;")); // fix single quote in password and ssid
    str_ap_pwd.replace("'", F("&apos;"));
    str_ota_pwd.replace("'", F("&apos;"));
    // display wifi list
    String wifiOptions = getWifiOptions(false);
    if (!wifiOptions.isEmpty())
    {
      wifiPage.replace(F("_WIFI_OPTIONS_"), wifiOptions);
    }
    wifiPage.replace(F("_SSID_"), str_ap_ssid);
    wifiPage.replace(F("_PSK_"), str_ap_pwd);
    wifiPage.replace(F("_OTA_PWD_"), str_ota_pwd);
    wifiPage.replace(F("_WIFI_STATIC_IP_"), wifi_static_ip);
    wifiPage.replace(F("_WIFI_STATIC_GW_"), wifi_static_gateway_ip);
    wifiPage.replace(F("_WIFI_STATIC_MASK_"), wifi_static_subnet);
    wifiPage.replace(F("_WIFI_STATIC_DNS_"), wifi_static_dns_ip);
    String fwCheckEvents = FPSTR(fw_check_script_events);
    sendWrappedHTML(request, fwCheckEvents + wifiPage);
  }
}

void handleStatus(AsyncWebServerRequest *request)
{
  String statusPage = FPSTR(html_page_status);
  // localize
  statusPage.replace("_TXT_STATUS_TITLE_", translatedWord(FL_(txt_status_title)));
  statusPage.replace("_TXT_STATUS_HVAC_", translatedWord(FL_(txt_status_hvac)));
  statusPage.replace("_TXT_RETRIES_HVAC_", translatedWord(FL_(txt_retries_hvac)));
  statusPage.replace("_TXT_STATUS_MQTT_", translatedWord(FL_(txt_status_mqtt)));
  statusPage.replace("_TXT_STATUS_WIFI_IP_", translatedWord(FL_(txt_status_wifi_ip)));
  statusPage.replace("_TXT_STATUS_WIFI_", translatedWord(FL_(txt_status_wifi)));
  statusPage.replace("_TXT_BUILD_VERSION_", translatedWord(FL_(txt_build_version)));
  statusPage.replace("_TXT_BUILD_DATE_", translatedWord(FL_(txt_build_date)));
  statusPage.replace("_TXT_STATUS_FREEHEAP_", translatedWord(FL_(txt_status_freeheap)));
  statusPage.replace("_TXT_CURRENT_TIME_", translatedWord(FL_(txt_current_time)));
  statusPage.replace("_TXT_BOOT_TIME", translatedWord(FL_(txt_boot_time)));
  statusPage.replace("_TXT_BACK_", translatedWord(FL_(txt_back)));
  // set data
  if (request->hasArg("mrconn"))
    mqttConnect();
  String connected = F("<font color='green'><b>");
  connected += translatedWord(FL_(txt_status_connect));
  connected += F("</b></font>");
  String disconnected = F("<font color='red'><b>");
  disconnected += translatedWord(FL_(txt_status_disconnect));
  disconnected += F("</b>(_MQTT_REASON_)</font>");
  disconnected.replace("_MQTT_REASON_", String(mqtt_disconnect_reason));
  if (hp.isConnected())
  {
    statusPage.replace(F("_HVAC_STATUS_"), connected);
  }
  else
  {
    statusPage.replace(F("_HVAC_STATUS_"), disconnected);
  }
  statusPage.replace(F("_HVAC_RETRIES_"), String(hpConnectionTotalRetries));
  if (WiFi.localIP().toString() == "0.0.0.0" || WiFi.localIP().toString() == "")
  {
    ESP_LOGD(TAG, "Failed to get IP address");
    String failedIp = F("<font color='red'>");
    failedIp += translatedWord(FL_(txt_failed_get_wifi_ip));
    failedIp += F("</font>");
    statusPage.replace(F("_WIFI_IP_"), failedIp);
  }
  else
  {
    statusPage.replace(F("_WIFI_IP_"), "<font color='blue'><b>" + WiFi.localIP().toString() + "</b></font>");
  }
  if (mqttClient != nullptr && mqttClient->connected())
    statusPage.replace(F("_MQTT_STATUS_"), connected);
  else
    statusPage.replace(F("_MQTT_STATUS_"), disconnected);
  statusPage.replace(F("_WIFI_STATUS_"), String(WiFi.RSSI()));
  statusPage.replace(F("_WIFI_BSSID_"), getWifiBSSID());
  statusPage.replace(F("_WIFI_MAC_"), getId());
  statusPage.replace(F("_BUILD_VERSION_"), getAppVersion());
  statusPage.replace(F("_BUILD_DATE_"), getBuildDatetime());
  // get free heap and percent
#ifdef ESP32
  uint32_t freeHeapBytes = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
  uint32_t totalHeapBytes = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
#else
  uint32_t freeHeapBytes = ESP.getFreeHeap();
  uint32_t totalHeapBytes = 64000;
#endif
  float percentageHeapFree = freeHeapBytes * 100.0f / (float)totalHeapBytes;
  String heap(freeHeapBytes);
  heap += " (";
  heap += String(percentageHeapFree);
  heap += "% )";
  statusPage.replace(F("_FREE_HEAP_"), heap);
  statusPage.replace(F("_CURRENT_TIME_"), "<font color='blue'><b>" + getCurrentTime() + "</b></font>");
  statusPage.replace(F("_BOOT_TIME_"), "<font color='orange'><b>" + getUpTime() + "</b></font>");
  sendWrappedHTML(request, statusPage);
}

void handleControl(AsyncWebServerRequest *request)
{
  checkLogin(request);
  // not connected to hp, redirect to status page
  if (!hp.isConnected())
  {
    AsyncWebServerResponse *response = request->beginResponse(301);
    response->addHeader("Location", "/status");
    response->addHeader("Cache-Control", "no-cache");
    request->send(response);
    return;
  }
  heatpumpSettings settings = hp.getSettings();
  settings = change_states(request, settings);
  String controlPage = FPSTR(html_page_control);
  String controlScript = FPSTR(control_script_events);
  // write_log("Enter HVAC control");
    // localize
  controlPage.replace("_TXT_CTRL_CTEMP_", translatedWord(FL_(txt_ctrl_ctemp)));
  controlPage.replace("_TXT_CTRL_TEMP_", translatedWord(FL_(txt_ctrl_temp)));
  controlPage.replace("_TXT_CTRL_TITLE_", translatedWord(FL_(txt_ctrl_title)));
  controlPage.replace("_TXT_CTRL_POWER_", translatedWord(FL_(txt_ctrl_power)));
  controlPage.replace("_TXT_CTRL_MODE_", translatedWord(FL_(txt_ctrl_mode)));
  controlPage.replace("_TXT_CTRL_FAN_", translatedWord(FL_(txt_ctrl_fan)));
  controlPage.replace("_TXT_CTRL_VANE_", translatedWord(FL_(txt_ctrl_vane)));
  controlPage.replace("_TXT_CTRL_WVANE_", translatedWord(FL_(txt_ctrl_wvane)));
  controlPage.replace("_TXT_F_ON_", translatedWord(FL_(txt_f_on)));
  controlPage.replace("_TXT_F_OFF_", translatedWord(FL_(txt_f_off)));
  controlPage.replace("_TXT_F_AUTO_", translatedWord(FL_(txt_f_auto)));
  controlPage.replace("_TXT_F_HEAT_", translatedWord(FL_(txt_f_heat)));
  controlPage.replace("_TXT_F_DRY_", translatedWord(FL_(txt_f_dry)));
  controlPage.replace("_TXT_F_COOL_", translatedWord(FL_(txt_f_cool)));
  controlPage.replace("_TXT_F_FAN_", translatedWord(FL_(txt_f_fan)));
  controlPage.replace("_TXT_F_QUIET_", translatedWord(FL_(txt_f_quiet)));
  controlPage.replace("_TXT_F_LOW_", translatedWord(FL_(txt_f_low)));
  controlPage.replace("_TXT_F_MEDIUM_", translatedWord(FL_(txt_f_medium)));
  controlPage.replace("_TXT_F_MIDDLE_", translatedWord(FL_(txt_f_middle)));
  controlPage.replace("_TXT_F_HIGH_", translatedWord(FL_(txt_f_high)));
  controlPage.replace("_TXT_F_SWING_", translatedWord(FL_(txt_f_swing)));
  controlPage.replace("_TXT_F_POS_", translatedWord(FL_(txt_f_pos)));
  controlPage.replace("_TXT_BACK_", translatedWord(FL_(txt_back)));
  // set data
  controlScript.replace(F("_MIN_TEMP_"), String(convertCelsiusToLocalUnit(min_temp, useFahrenheit)));
  controlScript.replace(F("_MAX_TEMP_"), String(convertCelsiusToLocalUnit(max_temp, useFahrenheit)));
  controlScript.replace(F("_TEMP_STEP_"), String(temp_step));
  controlPage.replace("_ROOMTEMP_", String(convertCelsiusToLocalUnit(hp.getRoomTemperature(), useFahrenheit)));
  controlPage.replace("_USE_FAHRENHEIT_", (String)useFahrenheit);
  controlPage.replace("_TEMP_SCALE_", getTemperatureScale());
  if (!supportHeatMode)
  {
    controlPage.replace(F("_HEAT_HIDDEN_"), F("'hidden' style='display: none;' disabled"));
  }
  else
  {
    controlPage.replace(F("_HEAT_HIDDEN_"), "");
  }
  controlScript.replace(F("_HEAT_MODE_SUPPORT_"), (String)supportHeatMode);

  if (!(String(settings.power).isEmpty())) // null may crash with multitask
  {
    controlPage.replace(F("_POWER_"), strcmp(settings.power, "ON") == 0 ? "checked" : "");
  }

  if (strcmp(settings.mode, "HEAT") == 0)
  {
    controlPage.replace("_MODE_H_", "selected");
  }
  else if (strcmp(settings.mode, "DRY") == 0)
  {
    controlPage.replace("_MODE_D_", "selected");
  }
  else if (strcmp(settings.mode, "COOL") == 0)
  {
    controlPage.replace("_MODE_C_", "selected");
  }
  else if (strcmp(settings.mode, "FAN") == 0)
  {
    controlPage.replace("_MODE_F_", "selected");
  }
  else if (strcmp(settings.mode, "AUTO") == 0)
  {
    controlPage.replace("_MODE_A_", "selected");
  }

  if (strcmp(settings.fan, "AUTO") == 0)
  {
    controlPage.replace("_FAN_A_", "selected");
  }
  else if (strcmp(settings.fan, "QUIET") == 0)
  {
    controlPage.replace("_FAN_Q_", "selected");
  }
  else if (strcmp(settings.fan, "1") == 0)
  {
    controlPage.replace("_FAN_1_", "selected");
  }
  else if (strcmp(settings.fan, "2") == 0)
  {
    controlPage.replace("_FAN_2_", "selected");
  }
  else if (strcmp(settings.fan, "3") == 0)
  {
    controlPage.replace("_FAN_3_", "selected");
  }
  else if (strcmp(settings.fan, "4") == 0)
  {
    controlPage.replace("_FAN_4_", "selected");
  }

  controlPage.replace("_VANE_V_", settings.vane);
  if (strcmp(settings.vane, "AUTO") == 0)
  {
    controlPage.replace("_VANE_A_", "selected");
  }
  else if (strcmp(settings.vane, "1") == 0)
  {
    controlPage.replace("_VANE_1_", "selected");
  }
  else if (strcmp(settings.vane, "2") == 0)
  {
    controlPage.replace("_VANE_2_", "selected");
  }
  else if (strcmp(settings.vane, "3") == 0)
  {
    controlPage.replace("_VANE_3_", "selected");
  }
  else if (strcmp(settings.vane, "4") == 0)
  {
    controlPage.replace("_VANE_4_", "selected");
  }
  else if (strcmp(settings.vane, "5") == 0)
  {
    controlPage.replace("_VANE_5_", "selected");
  }
  else if (strcmp(settings.vane, "SWING") == 0)
  {
    controlPage.replace("_VANE_S_", "selected");
  }

  controlPage.replace("_WIDEVANE_V_", settings.wideVane);
  if (strcmp(settings.wideVane, "<<") == 0)
  {
    controlPage.replace("_WVANE_1_", "selected");
  }
  else if (strcmp(settings.wideVane, "<") == 0)
  {
    controlPage.replace("_WVANE_2_", "selected");
  }
  else if (strcmp(settings.wideVane, "|") == 0)
  {
    controlPage.replace("_WVANE_3_", "selected");
  }
  else if (strcmp(settings.wideVane, ">") == 0)
  {
    controlPage.replace("_WVANE_4_", "selected");
  }
  else if (strcmp(settings.wideVane, ">>") == 0)
  {
    controlPage.replace("_WVANE_5_", "selected");
  }
  else if (strcmp(settings.wideVane, "<>") == 0)
  {
    controlPage.replace("_WVANE_6_", "selected");
  }
  else if (strcmp(settings.wideVane, "SWING") == 0)
  {
    controlPage.replace("_WVANE_S_", "selected");
  }
  controlPage.replace("_TEMP_", String(convertCelsiusToLocalUnit(hp.getTemperature(), useFahrenheit)));

  sendWrappedHTML(request, controlScript + controlPage);
  controlPage = "";
  // We need to send the page content in chunks to overcome
  // a limitation on the maximum size we can send at one
  // time (approx 6k).
}

#ifdef METRICS
void handleMetrics(AsyncWebServerRequest *request)
{
  String metrics = FPSTR(html_metrics);

  heatpumpSettings currentSettings = hp.getSettings();
  heatpumpStatus currentStatus = hp.getStatus();

  String hppower = strcmp(currentSettings.power, "ON") == 0 ? "1" : "0";

  String hpfan = currentSettings.fan;
  if (hpfan == "AUTO")
    hpfan = "-1";
  if (hpfan == "QUIET")
    hpfan = "0";

  String hpvane = currentSettings.vane;
  if (hpvane == "AUTO")
    hpvane = "-1";
  if (hpvane == "SWING")
    hpvane = "0";

  String hpwidevane = "-2";
  if (strcmp(currentSettings.wideVane, "SWING") == 0)
    hpwidevane = "0";
  if (strcmp(currentSettings.wideVane, "<<") == 0)
    hpwidevane = "1";
  if (strcmp(currentSettings.wideVane, "<") == 0)
    hpwidevane = "2";
  if (strcmp(currentSettings.wideVane, "|") == 0)
    hpwidevane = "3";
  if (strcmp(currentSettings.wideVane, ">") == 0)
    hpwidevane = "4";
  if (strcmp(currentSettings.wideVane, ">>") == 0)
    hpwidevane = "5";
  if (strcmp(currentSettings.wideVane, "<>") == 0)
    hpwidevane = "6";

  String hpmode = "-2";
  if (strcmp(currentSettings.mode, "AUTO") == 0)
    hpmode = "-1";
  if (strcmp(currentSettings.mode, "COOL") == 0)
    hpmode = "1";
  if (strcmp(currentSettings.mode, "DRY") == 0)
    hpmode = "2";
  if (strcmp(currentSettings.mode, "HEAT") == 0)
    hpmode = "3";
  if (strcmp(currentSettings.mode, "FAN") == 0)
    hpmode = "4";
  if (hppower == "0")
    hpmode = "0";

  metrics.replace("_UNIT_NAME_", hostname);
  metrics.replace("_VERSION_", m2mqtt_version);
  metrics.replace("_POWER_", hppower);
  metrics.replace("_ROOMTEMP_", (String)currentStatus.roomTemperature);
  metrics.replace("_TEMP_", (String)currentSettings.temperature);
  metrics.replace("_FAN_", hpfan);
  metrics.replace("_VANE_", hpvane);
  metrics.replace("_WIDEVANE_", hpwidevane);
  metrics.replace("_MODE_", hpmode);
  metrics.replace("_OPER_", (String)currentStatus.operating);
  metrics.replace("_COMPFREQ_", (String)currentStatus.compressorFrequency);
  sendWrappedHTML(request, metrics);
}
#endif

// login page, also called for logout
void handleLogin(AsyncWebServerRequest *request)
{
  bool loginSuccess = false;
  String msg;
  String loginPage = FPSTR(html_page_login);
  // localize
  loginPage.replace("_TXT_LOGIN_TITLE_", translatedWord(FL_(txt_login_title)));
  loginPage.replace("_TXT_LOGIN_PH_USER_", translatedWord(FL_(txt_login_ph_user)));
  loginPage.replace("_TXT_LOGIN_PH_PWD_", translatedWord(FL_(txt_login_ph_pwd)));
  loginPage.replace("_TXT_LOGIN_USERNAME_", translatedWord(FL_(txt_login_username)));
  loginPage.replace("_TXT_LOGIN_PASSWORD_", translatedWord(FL_(txt_login_password)));
  loginPage.replace("_TXT_LOGIN_OPEN_STATUS_", translatedWord(FL_(txt_login_open_status)));
  loginPage.replace("_TXT_LOGIN_", translatedWord(FL_(txt_login)));
  if (request->hasHeader("Cookie"))
  {
    // Found cookie;
    String cookie = request->header("Cookie");
  }
  if (request->hasArg("USERNAME") || request->hasArg("PASSWORD") || request->hasArg("LOGOUT"))
  {
    if (request->hasArg("LOGOUT"))
    {
      // logout
      loginSuccess = false;
    }
    if (request->hasArg("USERNAME") && request->hasArg("PASSWORD"))
    {
      if (request->arg("USERNAME") == login_username && request->arg("PASSWORD") == login_password)
      {
        loginSuccess = true;
        msg = F("<b><font color='red'>");
        msg += translatedWord(FL_(txt_login_sucess));
        msg += F("</font></b>");
        loginPage += F("<script>");
        loginPage += F("setTimeout(function () {");
        loginPage += F("window.location.href= '/';");
        loginPage += F("}, 3000);");
        loginPage += F("</script>");
        // Log in Successful;
      }
      else
      {
        msg = F("<b><font color='red'>");
        msg += translatedWord(FL_(txt_login_fail));
        msg += F("</font></b>");
        // Log in Failed;
      }
    }
  }
  else
  {
    if (is_authenticated(request) or login_password.length() == 0)
    {
      // use javascript in the case browser disable redirect
      String redirectPage = F("<html lang=\"en\" class=\"\"><head><meta charset='utf-8'>");
      redirectPage += F("<script>");
      redirectPage += F("setTimeout(function () {");
      redirectPage += F("window.location.href= '/';");
      redirectPage += F("}, 1000);");
      redirectPage += F("</script>");
      redirectPage += F("</body></html>");
      AsyncWebServerResponse *response = request->beginResponse(301, "text/html", redirectPage);
      response->addHeader("Location", "/");
      response->addHeader("Cache-Control", "no-cache");
      request->send(response);
      return;
    }
  }
  loginPage.replace(F("_LOGIN_SUCCESS_"), (String)loginSuccess);
  loginPage.replace(F("_LOGIN_MSG_"), msg);

  String headerContent = FPSTR(html_common_header);
  String footerContent = FPSTR(html_common_footer);
  String toSend(headerContent);
  toSend += loginPage;
  toSend += footerContent;
  toSend.replace(F("_UNIT_NAME_"), hostname);
  toSend.replace(F("_VERSION_"), getAppVersion());
  toSend.replace(F("_APP_NAME_"), appName);
  AsyncWebServerResponse *response = request->beginResponse(200, "text/html", toSend);
  if (loginSuccess)
  {
    response->addHeader("Set-Cookie", "M2MSESSIONID=1");
    response->addHeader("Cache-Control", "no-cache");
  }
  else
  {
    response->addHeader("Set-Cookie", "M2MSESSIONID=0");
    response->addHeader("Cache-Control", "no-cache");
  }
  request->send(response);
}

void handleUpgrade(AsyncWebServerRequest *request)
{
  checkLogin(request);
  uploaderror = 0;
  String upgradePage = FPSTR(html_page_upgrade);
  // localize
  upgradePage.replace("_TXT_FW_UPDATE_PAGE_", translatedWord(FL_(txt_fw_update_page)));
  upgradePage.replace("_TXT_B_UPGRADE_", translatedWord(FL_(txt_upgrade)));
  upgradePage.replace("_TXT_BACK_", translatedWord(FL_(txt_back)));
  upgradePage.replace("_TXT_UPGRADE_TITLE_", translatedWord(FL_(txt_upgrade_title)));
  upgradePage.replace("_TXT_UPGRADE_INFO_", translatedWord(FL_(txt_upgrade_info)));
  upgradePage.replace("_TXT_UPGRADE_START_", translatedWord(FL_(txt_upgrade_start)));

  sendWrappedHTML(request, upgradePage);
}

void handleUploadDone(AsyncWebServerRequest *request)
{
  ESP_LOGD(TAG, "HTTP: Firmware upload done");
  bool restartflag = false;
  String uploadDonePage = FPSTR(html_page_upload);
  // localize
  uploadDonePage.replace("_TXT_UPLOAD_FW_PAGE_", translatedWord(FL_(txt_upload_fw_page)));
  uploadDonePage.replace("_TXT_BACK_", translatedWord(FL_(txt_back)));

  String content = F("<div style='text-align:center;'><b>");
  content += translatedWord(FL_(txt_upload));
  content += F(" ");
  if (uploaderror)
  {
    content += F("<font color='red'>");
    content += translatedWord(FL_(txt_upload_failed));
    content += F("</font></b><br/><br/>");
    if (uploaderror == 1)
    {
      content += translatedWord(FL_(txt_upload_nofile));
    }
    else if (uploaderror == 2)
    {
      content += translatedWord(FL_(txt_upload_filetoolarge));
    }
    else if (uploaderror == 3)
    {
      content += translatedWord(FL_(txt_upload_fileheader));
    }
    else if (uploaderror == 4)
    {
      content += translatedWord(FL_(txt_upload_flashsize));
    }
    else if (uploaderror == 5)
    {
      content += translatedWord(FL_(txt_upload_buffer));
    }
    else if (uploaderror == 6)
    {
      content += translatedWord(FL_(txt_upload_failed));
    }
    else if (uploaderror == 7)
    {
      content += translatedWord(FL_(txt_upload_aborted));
    }
    else
    {
      content += translatedWord(FL_(txt_upload_error));
      content += String(uploaderror);
    }
    if (Update.hasError())
    {
      content += translatedWord(FL_(txt_upload_code));
      content += String(Update.getError());
    }
  }
  else
  {
    content += F("<b><font color='green'>");
    content += translatedWord(FL_(txt_upload_success));
    content += F("</font></b><br/><br/>");
    content += translatedWord(FL_(txt_upload_refresh));
    content += F(" <span id='count'>30s</span>...");
    content += F("<script>");
    content += F("setTimeout(function () {");
    content += F("window.location.href= '/';");
    content += F("}, 30000);");
    content += F("</script>");
    restartflag = true;
  }
  content += F("</div><br/>");
  uploadDonePage.replace("_UPLOAD_MSG_", content);
  uploadDonePage.replace("_TXT_BACK_", translatedWord(FL_(txt_back)));
  if (restartflag)
  {
    String countDown = FPSTR(count_down_script);
    sendWrappedHTML(request, uploadDonePage + countDown);
    sendRebootRequest(3);
  }
  else
  {
    sendWrappedHTML(request, uploadDonePage);
  }
}

void handleUploadLoop(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final)
{
  // Based on https://github.com/lbernstone/asyncUpdate/blob/master/AsyncUpdate.ino
  if (uploaderror)
  {
    Update.end();
    return;
  }
  if (filename.isEmpty())
  {
    uploaderror = 1;
    return;
  }
  if (!index)
  {
    ESP_LOGD(TAG, "Starting OTA Update");
    // save cpu by disconnect/stop retry mqtt server
    if (mqttClient != nullptr && mqttClient->connected())
    {
      mqttClient->disconnect();
      mqtt_reconnect_timeout = millis() + MQTT_RECONNECT_INTERVAL_MS;
    }
    ota_content_len = request->contentLength();
    // if filename includes spiffs, update the spiffs partition
    int cmd = (filename.indexOf("spiffs") > -1) ? U_PART : U_FLASH;
    // uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000; // 0x1000 start boot loader
#ifdef ESP8266
    Update.runAsync(true);
    if (!Update.begin(ota_content_len, cmd))
    {
#else
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd))
    {
#endif
      uploaderror = 2;
      return;
    }
  }
  if (!Update.hasError())
  {
    if (Update.write(data, len) != len)
    {
      uploaderror = 5;
#ifdef ESP8266
    }
    else
    {
      ESP_LOGD(TAG, "OTA file upload progress: %d%%", (Update.progress() * 100) / Update.size());
#endif
    }
  }
  else
  {
    uploaderror = Update.getError();
    Update.end();
  }
  if (uploaderror)
  {
    Update.end();
    return;
  }

  if (final)
  {
    if (!Update.end(true))
    {
      uploaderror = 6;
      ESP_LOGE(TAG, "Update error");
    }
    else
    {
      ESP_LOGD(TAG, "Update complete");
    }
  }
}

void write_log(const String& log)
{
  File logFile = SPIFFS.open(console_file, "a");
  logFile.println(log);
  logFile.close();
}

heatpumpSettings change_states(AsyncWebServerRequest *request, heatpumpSettings settings)
{
  bool update = false;
  if (request->hasArg("PWRCHK"))
  {
    settings.power = request->hasArg("POWER") ? "ON" : "OFF";
    update = true;
  }
  if (request->hasArg("MODE"))
  {
    ESP_LOGD(TAG, "Settings Mode before: %s", request->arg("MODE").c_str());
    settings.mode = request->arg("MODE").c_str();
    ESP_LOGD(TAG, "Settings Mode after: %s", settings.mode);
    update = true;
  }
  if (request->hasArg("TEMP"))
  {
    settings.temperature = convertLocalUnitToCelsius(request->arg("TEMP").toFloat(), useFahrenheit);
    update = true;
  }
  if (request->hasArg("FAN"))
  {
    ESP_LOGD(TAG, "Settings Fan before: %s", request->arg("FAN").c_str());
    settings.fan = request->arg("FAN").c_str();
    ESP_LOGD(TAG, "Settings Fan after: %s", settings.fan);
    update = true;
  }
  if (request->hasArg("VANE"))
  {
    settings.vane = request->arg("VANE").c_str();
    update = true;
  }
  if (request->hasArg("WIDEVANE"))
  {
    settings.wideVane = request->arg("WIDEVANE").c_str();
    update = true;
  }
  if (update)
  {
    hp.setSettings(settings);
    if (hp.getSettings() == hp.getWantedSettings()) // only update it settings change
    {
      ESP_LOGW(TAG, "Same Settings to HP, Igrore");
    }
    else
    {
      ESP_LOGI(TAG, "Send Settings to HP");
      requestHpUpdate = true;
      requestHpUpdateTime = millis() + 10;
    }
  }
  return settings;
}

void readHeatPumpSettings()
{
  heatpumpSettings currentSettings = hp.getSettings();

  rootInfo.clear();
  rootInfo["temperature"] = convertCelsiusToLocalUnit(currentSettings.temperature, useFahrenheit);
  rootInfo["fan"] = getFanModeFromHp(currentSettings.fan);
  rootInfo["vane"] = currentSettings.vane;
  rootInfo["wideVane"] = currentSettings.wideVane;
  rootInfo["mode"] = hpGetMode(currentSettings);
}

void hpSettingsChanged()
{
  if (millis() - hp.getLastWanted() < PREVENT_UPDATE_INTERVAL_MS) // prevent HA setting change after send update interval we wait for 1 seconds before udpate data
  {
    return;
  }
  // send room temp, operating info and all information
  readHeatPumpSettings();

  if (mqttClient != nullptr && mqttClient->connected())
  {
    String mqttOutput;
    serializeJson(rootInfo, mqttOutput);
    if (!mqttClient->publish(ha_settings_topic.c_str(), 1, false, mqttOutput.c_str()))
    {
      if (_debugModeLogs)
        mqttClient->publish(ha_debug_logs_topic.c_str(), 1, false, (char *)("Failed to publish hp settings"));
    }
  }
  hpStatusChanged(hp.getStatus());
}

// Convert mode for home assistant
String getFanModeFromHp(String modeFromHp)
{
  if (modeFromHp == "QUIET")
  {
    return "diffuse";
  }
  else if (modeFromHp == "1")
  {
    return "low";
  }
  else if (modeFromHp == "2")
  {
    return "medium";
  }
  else if (modeFromHp == "3")
  {
    return "middle";
  }
  else if (modeFromHp == "4")
  {
    return "high";
  }
  else
  { // case "AUTO" or default:
    return "auto";
  }
}

// Convert mode for heatpump lib
String getFanModeFromHa(String modeFromHa)
{
  if (modeFromHa == "diffuse")
  {
    return "QUIET";
  }
  else if (modeFromHa == "low")
  {
    return "1";
  }
  else if (modeFromHa == "medium")
  {
    return "2";
  }
  else if (modeFromHa == "middle")
  {
    return "3";
  }
  else if (modeFromHa == "high")
  {
    return "4";
  }
  else
  { // case "AUTO" or default:
    return "AUTO";
  }
}

String hpGetMode(heatpumpSettings hpSettings)
{
  // Map the heat pump state to one of HA's HVAC_MODE_* values.
  // https://github.com/home-assistant/core/blob/master/homeassistant/components/climate/const.py#L3-L23

  String hppower = String(hpSettings.power);
  if (hppower.equalsIgnoreCase("off"))
  {
    return "off";
  }

  String hpmode = String(hpSettings.mode);
  hpmode.toLowerCase();

  if (hpmode == "fan")
    return "fan_only";
  else if (hpmode == "auto")
    return "heat_cool";
  else
    return hpmode; // cool, heat, dry
}

String hpGetAction(heatpumpStatus hpStatus, heatpumpSettings hpSettings)
{
  // Map heat pump state to one of HA's CURRENT_HVAC_* values.
  // https://github.com/home-assistant/core/blob/master/homeassistant/components/climate/const.py#L80-L86

  String hppower = String(hpSettings.power);
  if (hppower.equalsIgnoreCase("off"))
  {
    return "off";
  }

  String hpmode = String(hpSettings.mode);
  hpmode.toLowerCase();

  if (hpmode == "fan")
    return "fan";
  else if (!hpStatus.operating)
    return "idle";
  else if (hpmode == "auto")
  {
    // If the "operating" flag is true and the heat pump is in "auto" mode,
    // it's either heating or cooling, but its status packets don't explicitly
    // indicate which of those two states it's in. We can infer the state by
    // comparing the room temperature to the set point temperature.
    if (hpStatus.roomTemperature > hpSettings.temperature) // above set point
      return "cooling";
    else if (hpStatus.roomTemperature < hpSettings.temperature) // below set point
      return "heating";
    else
      return hpmode; // unknown
  }
  else if (hpmode == "cool")
    return "cooling";
  else if (hpmode == "heat")
    return "heating";
  else if (hpmode == "dry")
    return "drying";
  else
    return hpmode; // unknown
}

void hpStatusChanged(heatpumpStatus currentStatus)
{
  if (millis() - hp.getLastWanted() < PREVENT_UPDATE_INTERVAL_MS) // prevent HA setting change after send update interval we wait for 1 seconds before udpate data
  {
    return;
  }
  hpCheckRemoteTemp(); // if the remote temperature feed from mqtt is stale, disable it and revert to the internal thermometer.

  // send room temp, operating info and all information
  heatpumpSettings currentSettings = hp.getSettings();

  if (currentStatus.roomTemperature == 0)
    return;

  rootInfo.clear();
  float roomTemperature = convertCelsiusToLocalUnit(currentStatus.roomTemperature, useFahrenheit);
  float temperature = convertCelsiusToLocalUnit(currentSettings.temperature, useFahrenheit);
  rootInfo["roomTemperature"] = roomTemperature;
  rootInfo["temperature"] = temperature;
  rootInfo["compressorFreq"] = currentStatus.compressorFrequency;
  events.send(String(roomTemperature).c_str(), "room_temperature", millis(), 50); // send data to browser
  events.send(String(temperature).c_str(), "temperature", millis(), 60);
  if (!(String(currentSettings.fan).isEmpty())) // null may crash with multitask
  {
    rootInfo["fan"] = getFanModeFromHp(currentSettings.fan);
    events.send(currentSettings.fan, "fan", millis(), 70);
  }
  if (!(String(currentSettings.vane).isEmpty()))
  {
    rootInfo["vane"] = currentSettings.vane;
    events.send(currentSettings.vane, "vane", millis(), 80);
  }
  if (!(String(currentSettings.wideVane).isEmpty()))
  {
    rootInfo["wideVane"] = currentSettings.wideVane;
    events.send(currentSettings.wideVane, "wideVane", millis(), 90);
  }
  rootInfo["mode"] = hpGetMode(currentSettings);
  rootInfo["action"] = hpGetAction(currentStatus, currentSettings);
  events.send(currentSettings.mode, "mode", millis(), 100);
  events.send(currentSettings.power, "power", millis(), 110);
  rootInfo["compressorFrequency"] = currentStatus.compressorFrequency;
  if (mqttClient != nullptr && mqttClient->connected())
  {
    String mqttOutput;
    serializeJson(rootInfo, mqttOutput);
    if (!mqttClient->publish(ha_state_topic.c_str(), 1, false, mqttOutput.c_str()))
    {
      if (_debugModeLogs)
        mqttClient->publish(ha_debug_logs_topic.c_str(), 1, false, (char *)("Failed to publish hp status change"));
    }
  }
}

void hpCheckRemoteTemp()
{
  if (remoteTempActive && (millis() - lastRemoteTemp > CHECK_REMOTE_TEMP_INTERVAL_MS))
  { // if it's been 5 minutes since last remote_temp message, revert back to HP internal temp sensor
    remoteTempActive = false;
    float temperature = 0;
    hp.setRemoteTemperature(temperature);
    hp.update();
  }
}

void sendKeepAlive()
{
  if (millis() - lastAliveMsgSend < SEND_ALIVE_MSG_INTERVAL_MS)
  {
    return;
  }
  lastAliveMsgSend = millis();

  // send keep alive message
  if (mqttClient != nullptr && mqttClient->connected())
  {
    if (!mqttClient->publish(ha_availability_topic.c_str(), 1, false, mqtt_payload_available))
    {
      if (_debugModeLogs)
        mqttClient->publish(ha_debug_logs_topic.c_str(), 1, false, (char *)"Failed to publish avialable status");
    }
    sendDeviceInfo();
    if (hp.isConnected())
    {
        hpStatusChanged(hp.getStatus());
    }
  }
}

void hpPacketDebug(byte *packet, unsigned int length, const char *packetDirection)
{
  if (_debugModePckts)
  {
    String message;
    for (unsigned int idx = 0; idx < length; idx++)
    {
      if (packet[idx] < 16)
      {
        message += "0"; // pad single hex digits with a 0
      }
      message += String(packet[idx], HEX) + " ";
    }

    const size_t bufferSize = JSON_OBJECT_SIZE(10);
    StaticJsonDocument<bufferSize> root;

    root[packetDirection] = message;
    if (mqttClient != nullptr && mqttClient->connected())
    {
      String mqttOutput;
      serializeJson(root, mqttOutput);
      if (!mqttClient->publish(ha_debug_pckts_topic.c_str(), 1, false, mqttOutput.c_str()))
      {
        mqttClient->publish(ha_debug_logs_topic.c_str(), 1, false, (char *)("Failed to publish to heatpump/debug topic"));
      }
    }
  }
}

// Used to send a dummy packet in state topic to validate action in HA interface
// HA change GUI appareance before having a valid state from the unit
void hpSendLocalState()
{
  if (mqttClient != nullptr && mqttClient->connected())
  {
    String mqttOutput;
    serializeJson(rootInfo, mqttOutput);
    mqttClient->publish(ha_debug_pckts_topic.c_str(), 1, false, mqttOutput.c_str());
    if (!mqttClient->publish(ha_state_topic.c_str(), 1, false, mqttOutput.c_str()))
    {
      if (_debugModeLogs)
        mqttClient->publish(ha_debug_logs_topic.c_str(), 1, false, (char *)("Failed to publish dummy hp status change"));
    }
  }
  // Restart counter for waiting enought time for the unit to update before sending a state packet
  lastTempSend = millis();
}

void mqttCallback(const char *topic, const uint8_t *payload, const unsigned int length)
{
  // Copy payload into message buffer
  char *message = new char[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';
  bool update = false;
  // HA topics
  // Receive power topic
  if (strcmp(topic, ha_power_set_topic.c_str()) == 0)
  {
    String modeUpper = message;
    modeUpper.toUpperCase();
    if (modeUpper == "OFF")
    {
      hp.setPowerSetting(modeUpper.c_str());
      update = true;
    }
  }
  else if (strcmp(topic, ha_mode_set_topic.c_str()) == 0)
  {
    String modeUpper = message;
    modeUpper.toUpperCase();
    if (modeUpper == "OFF")
    {
      rootInfo["mode"] = "off";
      rootInfo["action"] = "off";
      hpSendLocalState();
      hp.setPowerSetting("OFF");
      update = true;
    }
    else
    {
      if (modeUpper == "HEAT_COOL")
      {
        rootInfo["mode"] = "heat_cool";
        rootInfo["action"] = "idle";
        modeUpper = "AUTO";
      }
      else if (modeUpper == "HEAT")
      {
        rootInfo["mode"] = "heat";
        rootInfo["action"] = "heating";
      }
      else if (modeUpper == "COOL")
      {
        rootInfo["mode"] = "cool";
        rootInfo["action"] = "cooling";
      }
      else if (modeUpper == "DRY")
      {
        rootInfo["mode"] = "dry";
        rootInfo["action"] = "drying";
      }
      else if (modeUpper == "FAN_ONLY")
      {
        rootInfo["mode"] = "fan_only";
        rootInfo["action"] = "fan";
        modeUpper = "FAN";
      }
      else
      {
        modeUpper = "";
      }

      if (modeUpper.length() > 0) {
        hpSendLocalState();
        hp.setPowerSetting("ON");
        hp.setModeSetting(modeUpper.c_str());
        update = true;
      }
    }
  }
  else if (strcmp(topic, ha_temp_set_topic.c_str()) == 0)
  {
    float temperature = strtof(message, NULL);
    // add to fix HP turn off after change temperature
    heatpumpSettings currentSettings = hp.getSettings();
    hp.setPowerSetting(currentSettings.power);
    hp.setModeSetting(currentSettings.mode);
    //
    float temperature_c = convertLocalUnitToCelsius(temperature, useFahrenheit);
    if (temperature_c < min_temp || temperature_c > max_temp)
    {
      temperature_c = 23;
      rootInfo["temperature"] = convertCelsiusToLocalUnit(temperature_c, useFahrenheit);
    }
    else
    {
      rootInfo["temperature"] = temperature;
    }
    hpSendLocalState();
    hp.setTemperature(temperature_c);
    update = true;
  }
  else if (strcmp(topic, ha_fan_set_topic.c_str()) == 0)
  {
    rootInfo["fan"] = message;
    hpSendLocalState();
    hp.setFanSpeed(getFanModeFromHa(message).c_str());
    update = true;
  }
  else if (strcmp(topic, ha_vane_set_topic.c_str()) == 0)
  {
    rootInfo["vane"] = message;
    hpSendLocalState();
    hp.setVaneSetting(message);
    update = true;
  }
  else if (strcmp(topic, ha_wide_vane_set_topic.c_str()) == 0)
  {
    rootInfo["wideVane"] = (String)message;
    hpSendLocalState();
    hp.setWideVaneSetting(message);
    update = true;
  }

  else if (strcmp(topic, ha_remote_temp_set_topic.c_str()) == 0)
  {
    float temperature = strtof(message, NULL);
    if (temperature == 0)
    {                           // Remote temp disabled by mqtt topic set
      remoteTempActive = false; // clear the remote temp flag
      hp.setRemoteTemperature(0.0);
    }
    else
    {
      remoteTempActive = true;   // Remote temp has been pushed.
      lastRemoteTemp = millis(); // Note time
      hp.setRemoteTemperature(convertLocalUnitToCelsius(temperature, useFahrenheit));
    }

    update = true;
  }
  else if (strcmp(topic, ha_debug_pckts_set_topic.c_str()) == 0)
  { // if the incoming message is on the heatpump_debug_set_topic topic...
    if (strcmp(message, "on") == 0)
    {
      _debugModePckts = true;
      saveCurrentOthers();
      mqttClient->publish(ha_debug_pckts_topic.c_str(), 1, false, (char *)("Debug packets mode enabled"));
    }
    else if (strcmp(message, "off") == 0)
    {
      _debugModePckts = false;
      saveCurrentOthers();
      mqttClient->publish(ha_debug_pckts_topic.c_str(), 1, false, (char *)("Debug packets mode disabled"));
    }
  }
  else if (strcmp(topic, ha_debug_logs_set_topic.c_str()) == 0)
  { // if the incoming message is on the heatpump_debug_set_topic topic...
    if (strcmp(message, "on") == 0)
    {
      _debugModeLogs = true;
      saveCurrentOthers();
      mqttClient->publish(ha_debug_logs_topic.c_str(), 1, false, (char *)"Debug mode enabled");
    }
    else if (strcmp(message, "off") == 0)
    {
      _debugModeLogs = false;
      saveCurrentOthers();
      mqttClient->publish(ha_debug_logs_topic.c_str(), 1, false, (char *)"Debug mode disabled");
    }
  }
  else if (strcmp(topic, ha_system_set_topic.c_str()) == 0)
  { // We receive command for board
    if ((strcmp(message, "restart") == 0) and !requestReboot)
    { // We receive reboot command
      sendRebootRequest(3);
    }
    else if ((strcmp(message, "factory") == 0) and !requestReboot) // factory reset
    {
      sendRebootRequest(5);
      factoryReset();
    }
  }
  else if (strcmp(topic, ha_custom_packet.c_str()) == 0)
  { // send custom packet for advance user
    String custom = message;

    // copy custom packet to char array
    char buffer[(custom.length() + 1)]; // +1 for the NULL at the end
    custom.toCharArray(buffer, (custom.length() + 1));

    byte bytes[20]; // max custom packet bytes is 20
    int byteCount = 0;
    char *nextByte;

    // loop over the byte string, breaking it up by spaces (or at the end of the line - \n)
    nextByte = strtok(buffer, " ");
    while (nextByte != NULL && byteCount < 20)
    {
      bytes[byteCount] = strtol(nextByte, NULL, 16); // convert from hex string
      nextByte = strtok(NULL, "   ");
      byteCount++;
    }

    // dump the packet so we can see what it is. handy because you can run the code without connecting the ESP to the heatpump, and test sending custom packets
    hpPacketDebug(bytes, byteCount, "customPacket");

    hp.sendCustomPacket(bytes, byteCount);
  }
  else
  {
    String msg("heatpump: wrong mqtt topic: ");
    msg += topic;
    mqttClient->publish(ha_debug_logs_topic.c_str(), 1, false, msg.c_str());
  }

  if (update)
  {
    if (hp.getSettings() == hp.getWantedSettings()) // only update it settings change
    {
      ESP_LOGW(TAG, "Same Settings to HP, Igrore");
    } else {
      ESP_LOGI(TAG, "Send Settings to HP");
      requestHpUpdate = true;
      requestHpUpdateTime = millis() + 10;
    }
  }
  delete[] message;
}

String getEntityTag(byte tag_id)
{
  switch (tag_id)
  {
  case ENT_ROOM_TEMPERATURE:
    return F("roomTemperature");
    break;
  case ENT_COMPR_FRQ:
    return F("compressorFreq");
    break;
  case ENT_CONNECTION_STATE:
    return F("connection_state");
    break;
  case ENT_UP_TIME:
    return F("up_time");
    break;
  case ENT_FREE_HEAP:
    return F("free_heap");
    break;
  case ENT_RSSI:
    return F("rssi");
    break;
  case ENT_BSSI:
    return F("bssi");
    break;
  case ENT_RESTART_BTN:
    return F("restart");
    break;
  
  default:
    return F("unknown");
    break;
  }
}

String getEntityName(byte tag_id)
{
  switch (tag_id)
  {
  case ENT_ROOM_TEMPERATURE:
    return F("Room temperature");
    break;
  case ENT_COMPR_FRQ:
    return F("Compressor frequency");
    break;
  case ENT_CONNECTION_STATE:
    return F("Connection state");
    break;
  case ENT_UP_TIME:
    return F("Up time");
    break;
  case ENT_FREE_HEAP:
    return F("Free heap");
    break;
  case ENT_RSSI:
    return F("RSSI");
    break;
  case ENT_BSSI:
    return F("BSSI");
    break;
  case ENT_RESTART_BTN:
    return F("Restart");
    break;
  
  default:
    return F("Unknown");
    break;
  }
}

void haConfigureDevice(DynamicJsonDocument &haConfig)
{
  const size_t capacity = JSON_ARRAY_SIZE(15) + JSON_OBJECT_SIZE(30) + 50;
  DynamicJsonDocument haConnInfo(capacity);  

  String dev_id = getId();

  // device info object
  JsonObject haConfigDevice = haConfig.createNestedObject("dev");
  
  // identifiers array
  haConfigDevice.createNestedArray("ids")[0] = mqtt_fn + "_" + dev_id;

  // connection info (mac)
  haConnInfo.createNestedArray();
  haConnInfo[0] = "mac";
  haConnInfo[1] = dev_id;
  haConfigDevice.createNestedArray("cns")[0] = haConnInfo;

  // other device infos
  haConfigDevice["name"] = mqtt_fn;
  haConfigDevice["sw"] = String(appName) + " " + String(getAppVersion());
  haConfigDevice["mdl"] = model;
  haConfigDevice["mf"] = manufacturer;
  haConfigDevice["cu"] = "http://" + WiFi.localIP().toString();

  // availability topic
  haConfig["avty_t"] = ha_availability_topic;
  haConfig["pl_avail"] = mqtt_payload_available;       // MQTT online message payload
  haConfig["pl_not_avail"] = mqtt_payload_unavailable; // MQTT offline message payload
}

void haConfigSensor(byte tag_id, String unit, String icon, bool is_diagnostic = false)
{
  // send HA config packet for up time
  const size_t capacity = JSON_ARRAY_SIZE(15) + JSON_OBJECT_SIZE(30) + 250;
  DynamicJsonDocument haConfig(capacity);

  haConfig["icon"] = icon;
  haConfig["name"] = getEntityName(tag_id);
  
  // Set unique ID and value template
  String tag = getEntityTag(tag_id);
  haConfig["unique_id"] = getId() + "_" + tag;
  haConfig["val_tpl"] = "{{ value_json." + tag + " }}";

  if (tag_id == ENT_ROOM_TEMPERATURE)
  {
    haConfig["dev_cla"] = "temperature";
    haConfig["unit_of_meas"] = useFahrenheit ? "°F" : "°C";
    haConfig["stat_t"] = ha_state_topic;
  }
  else if (tag_id == ENT_COMPR_FRQ)
  {
    haConfig["dev_cla"] = "frequency";
    haConfig["unit_of_meas"] = unit;
    haConfig["stat_t"] = ha_state_topic;
  }
  else if (tag_id == ENT_CONNECTION_STATE)
  {
    haConfig["dev_cla"] = "connectivity";
    haConfig["payload_on"] = "online";
    haConfig["payload_off"] = "offline";
    haConfig["stat_t"] = ha_system_setting_info;
  }
  else if (tag_id == ENT_UP_TIME)
  {
    haConfig["dev_cla"] = "timestamp";
    haConfig["val_tpl"] = "{{ as_datetime(value_json." + tag + ") }}";
    // haConfig["unit_of_meas"] = unit;
    haConfig["stat_t"] = ha_system_setting_info;
  }
  else if (tag_id == ENT_FREE_HEAP)
  {
    haConfig["unit_of_meas"] = unit;
    haConfig["stat_t"] = ha_system_setting_info;
  }
  else if (tag_id == ENT_RSSI)
  {
    haConfig["unit_of_meas"] = unit;
    haConfig["stat_t"] = ha_system_setting_info;
  }
  else if (tag_id == ENT_BSSI)
  {
    haConfig["stat_t"] = ha_system_setting_info;
  }

  if (is_diagnostic)
    haConfig["ent_cat"] = "diagnostic";

  // add device info
  haConfigureDevice(haConfig);
  
  String mqttOutput;
  serializeJson(haConfig, mqttOutput);

  String ha_entity_type;
  if (tag_id == ENT_CONNECTION_STATE)
  {
    ha_entity_type = "binary_sensor";
  }
  else
  {
    ha_entity_type = "sensor";
  }

  String ha_config_topic_sensor;
  ha_config_topic_sensor = "homeassistant/" + ha_entity_type + "/hvac_" + getId() + "/" + tag + "/config";
  mqttClient->publish(ha_config_topic_sensor.c_str(), 1, true, mqttOutput.c_str());
}

void haConfigButton(byte tag_id, String payload_press, String icon)
{
  // send HA config packet for button
  const size_t capacity = JSON_ARRAY_SIZE(15) + JSON_OBJECT_SIZE(30) + 250;
  DynamicJsonDocument haConfig(capacity);

  haConfig["icon"] = icon;
  haConfig["name"] = getEntityName(tag_id);
  
  // Set unique ID and value template
  String tag = getEntityTag(tag_id);
  haConfig["unique_id"] = getId() + "_" + tag;

  haConfig["dev_cla"] = payload_press;
  haConfig["payload_press"] = payload_press; //"restart", "factory", "upgrade" ;
  haConfig["command_topic"] = ha_system_set_topic;
  haConfig["ent_cat"] = "config";
  
  // add device info
  haConfigureDevice(haConfig);

  String mqttOutput;
  serializeJson(haConfig, mqttOutput);
  String ha_config_topic_button = "homeassistant/button/hvac_" + getId() + "/" + tag + "/config";
  mqttClient->publish(ha_config_topic_button.c_str(), 1, true, mqttOutput.c_str());
}

void sendDeviceInfo()
{
  // send HA config packet for device info
  const size_t capacity = JSON_ARRAY_SIZE(15) + JSON_OBJECT_SIZE(30) + 150;
  DynamicJsonDocument haConfigInfo(capacity);

  haConfigInfo[getEntityTag(ENT_CONNECTION_STATE)] = hp.isConnected() ? "online" : "offline";
  // get free heap in percent
#ifdef ESP32
  uint32_t freeHeapBytes = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
  uint32_t totalHeapBytes = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
#else
  uint32_t freeHeapBytes = ESP.getFreeHeap();
  uint32_t totalHeapBytes = 64000;
#endif
  // we round to 0.5 (half) to avoid continue changes
  float percentageHeapFree = 0.5 * round(2.0*(freeHeapBytes * 100.0f / (float)totalHeapBytes));
  String heap(percentageHeapFree);
  haConfigInfo[getEntityTag(ENT_FREE_HEAP)] = heap;
  // get wifi rssi
  haConfigInfo[getEntityTag(ENT_RSSI)] = String(WiFi.RSSI());
  haConfigInfo[getEntityTag(ENT_BSSI)] = getWifiBSSID();
  haConfigInfo[getEntityTag(ENT_UP_TIME)] = getUpTimeSeconds();

  String mqttOutput;
  serializeJson(haConfigInfo, mqttOutput);
  mqttClient->publish(ha_system_setting_info.c_str(), 1, false, mqttOutput.c_str());
}

void haConfigClimate()
{

  // send HA config packet
  // setup HA payload device
  const size_t capacity = JSON_ARRAY_SIZE(5) + 2 * JSON_ARRAY_SIZE(6) + JSON_ARRAY_SIZE(7) + JSON_OBJECT_SIZE(24) + 2048;
  DynamicJsonDocument haConfig(capacity);

  haConfig["name"] = nullptr;
  haConfig["unique_id"] = getId();

  JsonArray haConfigModes = haConfig.createNestedArray("modes");
  haConfigModes.add("heat_cool"); // native AUTO mode
  haConfigModes.add("cool");
  haConfigModes.add("dry");
  if (supportHeatMode)
  {
    haConfigModes.add("heat");
  }
  haConfigModes.add("fan_only"); // native FAN mode
  haConfigModes.add("off");

  haConfig["mode_cmd_t"] = ha_mode_set_topic;
  haConfig["mode_stat_t"] = ha_state_topic;
  haConfig["mode_stat_tpl"] = F("{{ value_json.mode if (value_json is defined and value_json.mode is defined and value_json.mode|length) else 'off' }}"); // Set default value for fix "Could not parse data for HA"
  haConfig["temp_cmd_t"] = ha_temp_set_topic;
  haConfig["temp_stat_t"] = ha_state_topic;
  haConfig["pow_cmd_t"] = ha_power_set_topic;

  // Set default value for fix "Could not parse data for HA"
  String temp_stat_tpl_str = F("{% if (value_json is defined and value_json.temperature is defined) %}{% if (value_json.temperature|int >= ");
  temp_stat_tpl_str += (String)convertCelsiusToLocalUnit(min_temp, useFahrenheit) + " and value_json.temperature|int <= ";
  temp_stat_tpl_str += (String)convertCelsiusToLocalUnit(max_temp, useFahrenheit) + ") %}{{ value_json.temperature }}";
  temp_stat_tpl_str += "{% elif (value_json.temperature|int < " + (String)convertCelsiusToLocalUnit(min_temp, useFahrenheit) + ") %}" + (String)convertCelsiusToLocalUnit(min_temp, useFahrenheit) + "{% elif (value_json.temperature|int > " + (String)convertCelsiusToLocalUnit(max_temp, useFahrenheit) + ") %}" + (String)convertCelsiusToLocalUnit(max_temp, useFahrenheit) + "{% endif %}{% else %}" + (String)convertCelsiusToLocalUnit(22, useFahrenheit) + "{% endif %}";
  haConfig["temp_stat_tpl"] = temp_stat_tpl_str;
  haConfig["curr_temp_t"] = ha_state_topic;
  String curr_temp_tpl_str = F("{{ value_json.roomTemperature if (value_json is defined and value_json.roomTemperature is defined and value_json.roomTemperature|int > ");
  curr_temp_tpl_str += (String)convertCelsiusToLocalUnit(1, useFahrenheit) + ") }}"; // Set default value for fix "Could not parse data for HA"
  haConfig["curr_temp_tpl"] = curr_temp_tpl_str;
  haConfig["min_temp"] = convertCelsiusToLocalUnit(min_temp, useFahrenheit);
  haConfig["max_temp"] = convertCelsiusToLocalUnit(max_temp, useFahrenheit);
  haConfig["temp_step"] = temp_step;
  haConfig["temperature_unit"] = useFahrenheit ? "F" : "C";

  JsonArray haConfigFan_modes = haConfig.createNestedArray("fan_modes");
  haConfigFan_modes.add("auto");  //AUTO
  haConfigFan_modes.add("diffuse"); //QUIET
  haConfigFan_modes.add("low"); //1 native
  haConfigFan_modes.add("medium"); //2 native
  haConfigFan_modes.add("middle"); //3 native
  haConfigFan_modes.add("high"); //4 native

  haConfig["fan_mode_cmd_t"] = ha_fan_set_topic;
  haConfig["fan_mode_stat_t"] = ha_state_topic;
  haConfig["fan_mode_stat_tpl"] = F("{{ value_json.fan if (value_json is defined and value_json.fan is defined and value_json.fan|length) else 'auto' }}"); // Set default value for fix "Could not parse data for HA"

  JsonArray haConfigSwing_modes = haConfig.createNestedArray("swing_modes");
  haConfigSwing_modes.add("AUTO");
  haConfigSwing_modes.add("1");
  haConfigSwing_modes.add("2");
  haConfigSwing_modes.add("3");
  haConfigSwing_modes.add("4");
  haConfigSwing_modes.add("5");
  haConfigSwing_modes.add("SWING");

  haConfig[F("swing_mode_cmd_t")] = ha_vane_set_topic;
  haConfig["swing_mode_stat_t"] = ha_state_topic;
  haConfig["swing_mode_stat_tpl"] = F("{{ value_json.vane if (value_json is defined and value_json.vane is defined and value_json.vane|length) else 'AUTO' }}"); // Set default value for fix "Could not parse data for HA"
  haConfig["action_topic"] = ha_state_topic;
  haConfig["action_template"] = F("{{ value_json.action if (value_json is defined and value_json.action is defined and value_json.action|length) else 'idle' }}"); // Set default value for fix "Could not parse data for HA"

  // add device info
  haConfigureDevice(haConfig);

  String mqttOutput;
  serializeJson(haConfig, mqttOutput);
  mqttClient->publish(ha_config_topic.c_str(), 1, true, mqttOutput.c_str());
}

void sendHaConfig()
{
  // Climate
  haConfigClimate();

  // Button
  haConfigButton(ENT_RESTART_BTN, "restart", "mdi:restart");
  // Temperature sensors
  haConfigSensor(ENT_ROOM_TEMPERATURE, "", "mdi:thermometer");
  // Freq sensor
  haConfigSensor(ENT_COMPR_FRQ, "Hz", "mdi:sine-wave");
  // Up time
  haConfigSensor(ENT_UP_TIME, "", "mdi:clock", true);
  // HVAC connection state
  haConfigSensor(ENT_CONNECTION_STATE, "", "mdi:check-network", true);
  haConfigSensor(ENT_FREE_HEAP, "%", "mdi:memory", true);
  haConfigSensor(ENT_RSSI, "dBm", "mdi:network-strength-1", true);
  haConfigSensor(ENT_BSSI, "", "mdi:router-wireless", true);
}

void mqttConnect()
{
  ESP_LOGD(TAG, "Connecting to MQTT...");
  if (mqttClient != nullptr)
  {
    if (!mqtt_server.isEmpty() && !mqtt_port.isEmpty())
    {
      mqttClient->connect();
    }
  }
}

bool connectWifi()
{
  // WiFi.disconnect(true);
  // delay(1000);
#ifdef ESP32
  WiFi.setHostname(hostname.c_str());
#else
  WiFi.hostname(hostname.c_str());
#endif
  if (WiFi.getMode() != WIFI_STA)
  {
    WiFi.mode(WIFI_STA);
    delay(100);
  }
  bool static_valid = true;
  IPAddress local_ip, gw_ip, subnet_ip, dns_ip;
  if (static_valid && !local_ip.fromString(wifi_static_ip)) {
    static_valid = false;
  }
  if (static_valid && !gw_ip.fromString(wifi_static_gateway_ip)) {
    static_valid = false;
  }
  if (static_valid && !subnet_ip.fromString(wifi_static_subnet)) {
    static_valid = false;
  }
  if (static_valid) {
    if (wifi_static_dns_ip.isEmpty()) {
      dns_ip = gw_ip;
    } else if (!dns_ip.fromString(wifi_static_dns_ip)) {
      static_valid = false;
    }
  }

  bool ok = false;
  if (static_valid) {
    ok = WiFi.config(local_ip, gw_ip, subnet_ip, dns_ip);
  }
  if (!ok || !static_valid) {
    // fallback to DHCP
#ifdef ESP32
    WiFi.config((uint32_t)0, (uint32_t)0, (uint32_t)0);
#else
    WiFi.config(0, 0, 0);
#endif
    }
    WiFi.begin(ap_ssid.c_str(), ap_pwd.c_str());
    ESP_LOGD(TAG, "Connected to %s", ap_ssid.c_str());
    wifi_timeout = millis() + 30000;
    while (WiFi.status() != WL_CONNECTED && millis() < wifi_timeout)
	{
        ESP_LOGD(TAG, ".");
        // Serial.print(WiFi.status());
        //  wait 500ms, flashing the blue LED to indicate WiFi connecting...
        digitalWrite(blueLedPin, LOW);
        delay(250);
        digitalWrite(blueLedPin, HIGH);
        delay(250);
    }
    if (WiFi.status() != WL_CONNECTED)
	{
        ESP_LOGD(TAG, "Failed to connect to wifi");
        return false;
    }
    ESP_LOGD(TAG, "Connected to %s", ap_ssid.c_str());
    ESP_LOGD(TAG, "Ready, IP address: ");
    unsigned long dhcpStartTime = millis();
    while ((WiFi.localIP().toString() == "0.0.0.0" || WiFi.localIP().toString() == "") && millis() - dhcpStartTime < 5000)
	{
        ESP_LOGD(TAG, ".");
        delay(500);
    }
    if (WiFi.localIP().toString() == "0.0.0.0" || WiFi.localIP().toString() == "")
	{
        ESP_LOGD(TAG, "Failed to get IP address");
        return false;
    }
    ESP_LOGD(TAG, "%s", WiFi.localIP().toString().c_str());
    ticker.detach();  // Stop blinking the LED because now we are connected:)
    // keep LED off (For Wemos D1-Mini), Other board check the schematic
    digitalWrite(blueLedPin, HIGH);
    // Auto reconnected
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    return true;
}

// temperature helper functions
float toFahrenheit(float fromCelcius)
{
  return round(1.8 * fromCelcius + 32.0);
}

float toCelsius(float fromFahrenheit)
{
  return (fromFahrenheit - 32.0) / 1.8;
}

float convertCelsiusToLocalUnit(float temperature, bool isFahrenheit)
{
  if (isFahrenheit)
  {
    return toFahrenheit(temperature);
  }
  else
  {
    return temperature;
  }
}

float convertLocalUnitToCelsius(float temperature, bool isFahrenheit)
{
  if (isFahrenheit)
  {
    return toCelsius(temperature);
  }
  else
  {
    return temperature;
  }
}

String getTemperatureScale()
{
  if (useFahrenheit)
  {
    return "F";
  }
  else
  {
    return "C";
  }
}

String getId()
{
#ifdef ESP32
  char chipID[23];
  snprintf(chipID, 23, "%llX", ESP.getEfuseMac());
  return String(chipID);
#else
  String chipID = WiFi.macAddress();
  chipID.replace(":", "");
  return chipID;
#endif
}

// Check if header is present and correct
bool is_authenticated(AsyncWebServerRequest *request)
{
  if (request->hasHeader("Cookie"))
  {
    // Found cookie;
    String cookie = String(request->getHeader("Cookie")->value().c_str());
    if (cookie.indexOf("M2MSESSIONID=1") != -1)
    {
      // Authentication Successful
      return true;
    }
  }
  // Authentication Failed
  return false;
}

void checkLogin(AsyncWebServerRequest *request)
{
  if (!is_authenticated(request) and login_password.length() > 0)
  {
    // use javascript in the case browser disable redirect
    String redirectPage = F("<html lang=\"en\" class=\"\"><head><meta charset='utf-8'>");
    redirectPage += F("<script>");
    redirectPage += F("setTimeout(function () {");
    redirectPage += F("window.location.href= '/login';");
    redirectPage += F("}, 1000);");
    redirectPage += F("</script>");
    redirectPage += F("</body></html>");
    AsyncWebServerResponse *response = request->beginResponse(301, "text/html", redirectPage);
    response->addHeader("Location", "/login");
    response->addHeader("Cache-Control", "no-cache");
    request->send(response);
    return;
  }
}

void loop()
{
#ifdef ARDUINO_OTA
  ArduinoOTA.handle();
#endif
#ifdef WEBSOCKET_ENABLE
  ws.cleanupClients();
#endif
  checkRebootRequest();
  // reset board to attempt to connect to wifi again if in ap mode or wifi dropped out and time limit passed
  bool wifiConnected = WiFi.getMode() == WIFI_STA and WiFi.status() == WL_CONNECTED;
  if (wifiConnected)
  {
    // ESP_LOGD(TAG, "Reset wifi connect timeout");
    wifi_timeout = millis() + WIFI_RETRY_INTERVAL_MS;
  }
  else if (wifi_config and millis() > wifi_timeout)
  {
    ESP_LOGD(TAG, "Wifi connect timeout, restart device now");
    ESP.restart();
  }
  // Sync HVAC UNIT even if mqtt not connected
  if (!captive)
  {
#ifdef ESP8266
    MDNS.update(); // ESP32 working without call this
#endif
    checkHpUpdateRequest();
    checkWifiScanRequest();
    // Sync HVAC UNIT
    if (!hp.isConnected())
    {
      // Use exponential backoff for retries, where each retry is double the length of the previous one.
      unsigned long durationNextSync = (1 << hpConnectionRetries) * HP_RETRY_INTERVAL_MS;
      if (((millis() - lastHpSync > durationNextSync) or lastHpSync == 0))
      {
        lastHpSync = millis();
        // If we've retried more than the max number of tries, keep retrying at that fixed interval, which is several minutes.
        hpConnectionRetries = min(hpConnectionRetries + 1u, HP_MAX_RETRIES);
        hpConnectionTotalRetries++;
        hp.sync();
      }
    }
    else
    {
      hpConnectionRetries = 0;
      hp.sync();
    }
    // check mqtt status and retry
    if (wifiConnected and (!mqtt_connected || !mqttClient->connected()) and millis() > mqtt_reconnect_timeout) // retry to connect mqtt
    {
      mqtt_reconnect_timeout = millis() + MQTT_RECONNECT_INTERVAL_MS; // only retry next 5 seconds to prevent crash
      if (mqttClient != nullptr)
      {
#ifdef ESP32
        xTimerStart(mqttReconnectTimer, 0);
#else
        mqttConnect();
#endif
      }
    }
  }
  else
  {
    dnsServer.processNextRequest(); // for captivate portal
  }
  if (!captive and mqtt_config)
  {
#ifdef ESP8266
    if (mqttClient != nullptr)
    {
      mqttClient->loop();
    }
#endif
    if (wifiConnected && mqtt_connected)
    {
      sendKeepAlive();
    }
  }
  // delay(10);
}

// Reboot in nextSeconds in the future
void sendRebootRequest(unsigned long nextSeconds)
{
  requestReboot = true;
  requestRebootTime = millis() + nextSeconds * 1000L;
  ESP_LOGI(TAG, "Send Reboot Request");
}

void checkRebootRequest()
{
  if (requestReboot and (millis() > requestRebootTime + REBOOT_REQUEST_INTERVAL_MS))
  {
    requestReboot = false;
    requestRebootTime = 0;
    ESP_LOGI(TAG, "Restart device from request");
    ESP.restart();
  }
}

void checkHpUpdateRequest()
{
  if (requestHpUpdate and (millis() > requestHpUpdateTime))
  {
    requestHpUpdate = false;
    requestHpUpdateTime = 0;
    ESP_LOGI(TAG, "Update HP from request");
    hp.update();
  }
}

void checkWifiScanRequest()
{
  if (requestWifiScan and (millis() > requestWifiScanTime))
  {
    requestWifiScan = false;
    requestWifiScanTime = 0;
    WiFi.scanNetworks(true);
    lastWifiScanMillis = millis();
  }
  else
  {
    if (wifi_list.isEmpty() and millis() - lastWifiScanMillis > 2000) // waiting 2 seconds for data available
    {
      getWifiList();
      getWifiOptions(true); // send data over web event
    }
  }
}

#ifdef ESP32
void WiFiEvent(WiFiEvent_t event)
{
  ESP_LOGD(TAG, "[WiFi-event] event: %d\n", event);
  if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP || event == ARDUINO_EVENT_WIFI_STA_GOT_IP6)
  {
    ESP_LOGD(TAG, "WiFi connected, IP address: %s", WiFi.localIP().toString().c_str());
    if (millis() > mqtt_reconnect_timeout)
    {
      mqtt_reconnect_timeout = millis() + MQTT_RECONNECT_INTERVAL_MS; // only retry next 5 seconds to prevent crash
      ticker.detach();                                                // Stop blinking the LED because now we are connected:)
      // keep LED off (For Wemos D1-Mini), Other board check the schematic
      digitalWrite(blueLedPin, HIGH);
      xTimerStart(mqttReconnectTimer, 0); // start timer to connect to MQTT
      // init and get the time
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer.c_str());
    }
  }
  else if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED)
  {
    ESP_LOGD(TAG, "WiFi lost connection");
    if (millis() > wifi_timeout)
    {
      ESP_LOGD(TAG, "Starting AP mode");
      xTimerStop(mqttReconnectTimer, 0);
      // xTimerStop(wifiReconnectTimer, 0);
    }
    else
    { // retry connect
      if (millis() > wifi_reconnect_timeout)
      {
        wifi_reconnect_timeout = millis() + WIFI_RECONNECT_INTERVAL_MS; // only retry next 5 seconds to prevent crash
        xTimerStop(mqttReconnectTimer, 0);                              // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
        // xTimerStart(wifiReconnectTimer, 0);
      }
    }
  }
}
#elif defined(ESP8266)

void onWifiConnect(const WiFiEventStationModeGotIP &event)
{
  ESP_LOGD(TAG, "WiFi connected, IP address: %s", WiFi.localIP().toString().c_str());
  if (millis() > mqtt_reconnect_timeout)
  {
    mqtt_reconnect_timeout = millis() + MQTT_RECONNECT_INTERVAL_MS; // only retry next 5 seconds to prevent crash
    ticker.detach();                                                // Stop blinking the LED because now we are connected:)
    // keep LED off (For Wemos D1-Mini), Other board check the schematic
    digitalWrite(blueLedPin, HIGH);
    mqttConnect();
    // init and get the time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  }
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected &event)
{
  ESP_LOGD(TAG, "Disconnected from Wi-Fi.");
  // TODO crash on esp8266
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  // wifiReconnectTimer.once(2, connectWifi);
}

#endif

void onMqttConnect(bool sessionPresent)
{
  ESP_LOGD(TAG, "Connected to MQTT. Session present: %d", sessionPresent);
  mqtt_connected = true;

  mqttClient->subscribe(ha_system_set_topic.c_str(), 1);
  mqttClient->subscribe(ha_debug_pckts_set_topic.c_str(), 1);
  mqttClient->subscribe(ha_debug_logs_set_topic.c_str(), 1);
  mqttClient->subscribe(ha_mode_set_topic.c_str(), 1);
  mqttClient->subscribe(ha_fan_set_topic.c_str(), 1);
  mqttClient->subscribe(ha_temp_set_topic.c_str(), 1);
  mqttClient->subscribe(ha_vane_set_topic.c_str(), 1);
  mqttClient->subscribe(ha_wide_vane_set_topic.c_str(), 1);
  mqttClient->subscribe(ha_remote_temp_set_topic.c_str(), 1);
  mqttClient->subscribe(ha_custom_packet.c_str(), 1);
  // send online message
  mqttClient->publish(ha_availability_topic.c_str(), 1, false, mqtt_payload_available);
  sendHaConfig();
}

void onMqttDisconnect(espMqttClientTypes::DisconnectReason reason)
{
  mqtt_disconnect_reason = (uint8_t)reason;
  mqtt_connected = false;
  ESP_LOGE(TAG, "Disconnected from MQTT. reason: %d", (uint8_t)reason);
  bool wifiConnected = WiFi.getMode() == WIFI_STA and WiFi.status() == WL_CONNECTED;
  if (wifiConnected)
  {
#ifdef ESP32
    xTimerStart(mqttReconnectTimer, 0);
#else
    mqttConnect();
#endif
  }
}

void onMqttSubscribe(uint16_t packetId, const espMqttClientTypes::SubscribeReturncode *codes, size_t len)
{
  for (size_t i = 0; i < len; ++i)
  {
    ESP_LOGD(TAG, "Subscribe acknowledged. packetId: %d, qos: %d", packetId, static_cast<uint8_t>(codes[i]));
  }
}

void onMqttUnsubscribe(uint16_t packetId)
{
  ESP_LOGD(TAG, "Unsubscribe acknowledged. packetId:  %d", packetId);
}

void onMqttMessage(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, size_t len, size_t index, size_t total)
{
  ESP_LOGD(TAG, "Publish received. topic: %s, qos: %d dup: %d, retain: %d", topic, properties.qos, properties.dup, properties.retain);
  ESP_LOGD(TAG, "Publish received. len: %d, index: %d, total: %d", len, index, total);
  mqttCallback(topic, payload, len);
}

void onMqttPublish(uint16_t packetId)
{
  ESP_LOGD(TAG, "Publish acknowledged. packetId:  %d", packetId);
}

// Handler webserver response
#ifdef WEBSOCKET_ENABLE
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    ESP_LOGD(TAG, "ws[%s][%u] connect\n", server->url(), client->id());
    client->printf("Hello Client %u :)", client->id());
    client->ping();
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    ESP_LOGD(TAG, "ws[%s][%u] disconnect\n", server->url(), client->id());
  }
  else if (type == WS_EVT_ERROR)
  {
    ESP_LOGD(TAG, "ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t *)arg), (char *)data);
  }
  else if (type == WS_EVT_PONG)
  {
    ESP_LOGD(TAG, "ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len) ? (char *)data : "");
  }
  else if (type == WS_EVT_DATA)
  {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    String msg = "";
    if (info->final && info->index == 0 && info->len == len)
    {
      // the whole message is in a single frame and we got all of it's data
      ESP_LOGD(TAG, "ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT) ? "text" : "binary", info->len);

      if (info->opcode == WS_TEXT)
      {
        for (size_t i = 0; i < info->len; i++)
        {
          msg += (char)data[i];
        }
      }
      else
      {
        char buff[3];
        for (size_t i = 0; i < info->len; i++)
        {
          sprintf(buff, "%02x", (uint8_t)data[i]);
          msg += buff;
        }
      }
      ESP_LOGD(TAG, "%s\n", msg.c_str());
      if (info->opcode == WS_TEXT)
      {
        String command = getValueBySeparator(msg, ';', 0);
        if (command == "language")
        {
          String data = getValueBySeparator(msg, ';', 1);
          if (system_language_index != data.toInt())
          {
            client->text("REFRESH"); // refresh web page
            system_language_index = data.toInt();
            ESP_LOGE(TAG, "Set unit language id: %d\n", system_language_index);
          }
        }
        // client->text("I got your text message"); // may crash on Safari
      }
      else
      {
        client->binary("I got your binary message");
      }
    }
    else
    {
      // message is comprised of multiple frames or the frame is split into multiple packets
      if (info->index == 0)
      {
        if (info->num == 0)
          ESP_LOGD(TAG, "ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
        ESP_LOGD(TAG, "ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
      }

      ESP_LOGD(TAG, "ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT) ? "text" : "binary", info->index, info->index + len);

      if (info->opcode == WS_TEXT)
      {
        for (size_t i = 0; i < len; i++)
        {
          msg += (char)data[i];
        }
      }
      else
      {
        char buff[3];
        for (size_t i = 0; i < len; i++)
        {
          sprintf(buff, "%02x", (uint8_t)data[i]);
          msg += buff;
        }
      }
      ESP_LOGD(TAG, "%s\n", msg.c_str());

      if ((info->index + len) == info->len)
      {
        ESP_LOGD(TAG, "ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
        if (info->final)
        {
          ESP_LOGD(TAG, "ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
          // if(info->message_opcode == WS_TEXT)
          //   client->text("I got your text message");
          // else
          //   client->binary("I got your binary message");
        }
      }
    }
  }
}
#endif

String getCurrentTime()
{
  time_t now;
  char strftime_buf[64];
  struct tm timeinfo;

  time(&now);
  // Set timezone to Vietnam Standard Time
  setenv("TZ", timezone.c_str(), 1);
  tzset();

  localtime_r(&now, &timeinfo);
  strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
  return String(strftime_buf);
}

time_t getUpTimeSeconds()
{
  time_t now;

  time(&now);
  // Set timezone to Vietnam Standard Time
  setenv("TZ", timezone.c_str(), 1);
  tzset();

#ifdef ESP32
  int64_t microSecondsSinceBoot = esp_timer_get_time();
  int64_t secondsSinceBoot = microSecondsSinceBoot / 1000000;
#else
  int32_t milliSecondsSinceBoot = millis(); // 2^32-1 only about 49 day before roll over
  int32_t secondsSinceBoot = milliSecondsSinceBoot / 1000;
#endif

  now -= secondsSinceBoot;

  return now;
}

// Time device running without crash or reboot
String getUpTime()
{
  char strftime_buf[64];
  struct tm timeinfo;

  time_t uptime = getUpTimeSeconds();

  localtime_r(&uptime, &timeinfo);
  strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
  return String(strftime_buf);
}

#ifdef ESP32
void initNVS()
{
  // Initialize NVS.
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    // 1.OTA app partition table has a smaller NVS partition size than the non-OTA
    // partition table. This size mismatch may cause NVS initialization to fail.
    // 2.NVS partition contains data in new format and cannot be recognized by this version of code.
    // If this happens, we erase NVS partition and initialize NVS again.
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
}
#endif

String getAppVersion()
{
  if (version.isEmpty())
  {
#ifdef ESP32
    const esp_app_desc_t *app_desc = esp_ota_get_app_description();
    app_name = strdup(app_desc->project_name);
    version = String(app_desc->version);
    version.replace(F("-dirty"), "");
    if (version.startsWith("esp-idf")){
      String data = getValueBySeparator(version, ' ', 2);
      version = String(m2mqtt_version) + "-";
      version += data;
    }
    return version;
#endif
    return m2mqtt_version;
  }
  return version;
}

String getBuildDatetime()
{
  if (build_date_time.isEmpty())
  {
#ifdef ESP32
    const esp_app_desc_t *app_desc = esp_ota_get_app_description();
    build_date_time = String(app_desc->date) + ", " + String(app_desc->time);
    return build_date_time;
#else
    char builDate[64];
    sprintf(builDate, "%s %s", __DATE__, __TIME__);
    build_date_time = String(builDate);
    return build_date_time;
#endif
  }
  return build_date_time;
}

bool isSecureEnable()
{
#ifdef ESP32
  bool flashEncrypt = esp_flash_encryption_enabled();
  bool secureBoot = esp_secure_boot_enabled();
  ESP_LOGW(TAG, "Flash encryption:  %s", flashEncrypt ? "YES" : "NO");
  ESP_LOGW(TAG, "Secure boot:  %s", secureBoot ? "YES" : "NO");
  return flashEncrypt && secureBoot;
#endif
  return false;
}

void getWifiList()
{
  int n = WiFi.scanComplete();
  if (n < 0) return;

  const int k = 5; // top k results

  int top_k_idx[k] = {};
  int32_t top_k_rssi[k];
  for (auto i = 0; i < k; i++) {
    top_k_rssi[i] = INT32_MIN;
  }

  // find top k rssi. k = 5 => n*k => O(n)
  for (int i = 0; i < n; i++) {
    int min_index = 0;
    for (int j = 0; j < k; j++) {
      if (top_k_rssi[j] < top_k_rssi[min_index]) {
        min_index = j;
      }
    }
    int32_t rssi = WiFi.RSSI(i);
    if (rssi > top_k_rssi[min_index]) {
      top_k_rssi[min_index] = rssi;
      top_k_idx[min_index] = i;
    }
  }

  // sort by rssi. k = 5 => k^2 is O(c)
  for (int i = 0; i < k-1; i++) {
    for (int j = i + 1; j < k; j++ ) {
      if (top_k_rssi[i] < top_k_rssi[j]) {
        std::swap(top_k_rssi[i], top_k_rssi[j]);
        std::swap(top_k_idx[i], top_k_idx[j]);
      }
    }
  }

  wifi_list.clear();
  for (int i = 0; i < k; ++i) // only first 5 networkd
  {
    int idx = top_k_idx[i];
    String ssid = WiFi.SSID(idx);
    if (!ssid.isEmpty())
    {
      ESP_LOGI(TAG, "Found %s: ", ssid.c_str());
      if (i == 0)
      {
        wifi_list += ssid;
      }
      else
      {
        wifi_list += ";" + ssid;
      }
    }
  }
  WiFi.scanDelete();
}

String getWifiOptions(bool send)
{
  String wifiOptions = "";
  if (!getValueBySeparator(wifi_list, ';', 0).isEmpty())
  {
    // reset and add fist empty
    wifiOptions = "";
    wifiOptions += "<option value='";
    wifiOptions += "";
    wifiOptions += "'>";
    wifiOptions += "";
    wifiOptions += "</option>";
    for (int i = 0; i < 5; ++i) // only first 5 network
    {
      String ssid = getValueBySeparator(wifi_list, ';', i);
      if (!ssid.isEmpty())
      {
        wifiOptions += "<option value='";
        wifiOptions += ssid;
        wifiOptions += "'>";
        wifiOptions += ssid;
        wifiOptions += "</option>";
      }
    }
    if (send)
    {
      events.send(wifiOptions.c_str(), "wifiOptions", millis(), 20); // send wifi data to browser
    }
  }
  return wifiOptions;
}

// String  var = getValueBySeparator( StringVar, ',', 2); // if  a,4,D,r  would return D
String getValueBySeparator(const String& data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length();

  for (int i = 0; i <= maxIndex && found <= index; i++)
  {
    if (data.charAt(i) == separator || i == maxIndex)
    {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void factoryReset()
{
  SPIFFS.format();
  WiFi.disconnect(true, true);
#ifdef ESP32
  // delete nvs partition because AP mode not working if have other data
  esp_err_t err = nvs_flash_erase();
  if (err == ESP_ERR_NOT_FOUND)
  {
    ESP_LOGE(TAG, "Default nvs partition not found");
  }
  err = nvs_flash_erase_partition("fctry");
  if (err == ESP_ERR_NOT_FOUND)
  {
    ESP_LOGE(TAG, "fctry partition not found");
  }
#endif
}

String getWifiBSSID()
{
  byte* mac = WiFi.BSSID();
  // Wifi BSSID
  char wifi_bssid[18];
  snprintf(wifi_bssid, 18, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(wifi_bssid);
}

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

// Load and save wifi.json, mqtt.json, unit.json, and others.json.

#include "settings_store.h"
#include "system_util.h"


void mountConfigFilesystem()
{
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
  const size_t capacity = JSON_OBJECT_SIZE(10) + 401;
  DynamicJsonDocument doc(capacity);
  deserializeJson(doc, configFile);
  // Legacy key. "fah" enables Fahrenheit; unit.json loaded afterwards does not clear it.
  String unit_tempUnit = doc["unit_tempUnit"].as<String>();
  if (unit_tempUnit == "fah")
    useFahrenheit = true;
  others_haa_topic = doc["haat"].as<String>();
  String haa = doc["haa"].as<String>();
  String debugPckts = doc["debugPckts"].as<String>();
  String debugLogs = doc["debugLogs"].as<String>();
  String webPanel = doc["webPanel"].as<String>();
  if (strcmp(haa.c_str(), "OFF") == 0)
  {
    others_haa = false;
  }
  if (strcmp(debugPckts.c_str(), "ON") == 0)
  {
    debugPacketsEnabled = true;
  }
  if (strcmp(debugLogs.c_str(), "ON") == 0)
  {
    debugLogsEnabled = true;
  }
  if (strcmp(webPanel.c_str(), "OFF") == 0)
  {
    webPanelDisabled = true;
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
    // set default topic if empty
    mqttTopic = default_mqtt_topic;
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

void saveOthers(const String& haa, const String& haat, const String& debugPckts, const String& debugLogs, const String& webPanel, const String& txPin, const String& rxPin, const String& tz, const String &ntp)
{
  // Allocate document capacity.
  const size_t capacity = JSON_OBJECT_SIZE(10) + 401;
  DynamicJsonDocument doc(capacity);
  doc["haa"] = haa;
  doc["haat"] = haat;
  doc["debugPckts"] = debugPckts;
  doc["debugLogs"] = debugLogs;
  doc["webPanel"] = webPanel;
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
  String debugPckts = debugPacketsEnabled ? "ON" : "OFF";
  String debugLogs = debugLogsEnabled ? "ON" : "OFF";
  String webPanel= webPanelDisabled ? "OFF" : "ON";
  saveOthers(haa, others_haa_topic, debugPckts, debugLogs, webPanel, String(HP_TX), String(HP_RX), timezone, ntpServer);
}

void setDefaults()
{
  ap_ssid = "";
  ap_pwd = "";
  others_haa = true;
  others_haa_topic = "homeassistant";
  mqtt_client_id = getId();
  mqtt_client_id.toLowerCase();
}

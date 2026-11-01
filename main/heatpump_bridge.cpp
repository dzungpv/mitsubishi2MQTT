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

// CN105 heat-pump callbacks, Home Assistant mode mapping, and state publishes.

#include "heatpump_bridge.h"
#include "ha_discovery.h"
#include "system_util.h"

static void hpSettingsChanged();
static String getFanModeFromHp(String modeFromHp);
static void hpCheckRemoteTemp();

heatpumpSettings applyWebControlSettings(AsyncWebServerRequest *request, heatpumpSettings settings)
{
  bool update = false;
  if (request->args() == 0)
    return settings;

  if (request->hasArg(F("PWRCHK")))
  {
    settings.power = request->hasArg(F("POWER")) ? "ON" : "OFF";
    update = true;
  }
  if (request->hasArg(F("MODE")))
  {
    //ESP_LOGD(TAG, "Settings Mode before: %s", request->arg("MODE").c_str());
    settings.mode = request->arg(F("MODE")).c_str();
    //ESP_LOGD(TAG, "Settings Mode after: %s", settings.mode);
    update = true;

  }
  if (request->hasArg(F("TEMP")))
  {
    float new_temp = convertLocalUnitToCelsius(request->arg(F("TEMP")).toFloat(), useFahrenheit);
    if (new_temp != settings.temperature)
    {
      settings.temperature = new_temp;
      update = true;
    }
  }
  if (request->hasArg(F("FAN")))
  {
    //ESP_LOGD(TAG, "Settings Fan before: %s", request->arg("FAN").c_str());
    settings.fan = request->arg(F("FAN")).c_str();
    //ESP_LOGD(TAG, "Settings Fan after: %s", settings.fan);
    update = true;
  }
  if (request->hasArg(F("VANE")))
  {
    settings.vane = request->arg(F("VANE")).c_str();
    update = true;
  }
  if (request->hasArg(F("WIDEVANE")))
  {
    settings.wideVane = request->arg(F("WIDEVANE")).c_str();
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

static void hpSettingsChanged()
{
  if (millis() - hp.getLastWanted() < PREVENT_UPDATE_INTERVAL_MS) // prevent HA setting change after send update interval we wait for 1 seconds before udpate data
  {
    return;
  }

  // send room temp, operating info and all information
  hpStatusChanged(hp.getStatus());
}

// Convert mode for home assistant
static String getFanModeFromHp(String modeFromHp)
{
  if (modeFromHp == F("QUIET"))
  {
    return F("diffuse");
  }
  else if (modeFromHp == F("1"))
  {
    return F("low");
  }
  else if (modeFromHp == F("2"))
  {
    return F("medium");
  }
  else if (modeFromHp == F("3"))
  {
    return F("middle");
  }
  else if (modeFromHp == F("4"))
  {
    return F("high");
  }
  else
  { // case "AUTO" or default:
    return F("auto");
  }
}

// Convert mode for heatpump lib
String getFanModeFromHa(String modeFromHa)
{
  if (modeFromHa == F("diffuse"))
  {
    return F("QUIET");
  }
  else if (modeFromHa == F("low"))
  {
    return F("1");
  }
  else if (modeFromHa == F("medium"))
  {
    return F("2");
  }
  else if (modeFromHa == F("middle"))
  {
    return F("3");
  }
  else if (modeFromHa == F("high"))
  {
    return F("4");
  }
  else
  { // case "AUTO" or default:
    return F("AUTO");
  }
}

String hpGetMode(heatpumpSettings hpSettings)
{
  // Map the heat pump state to one of HA's HVAC_MODE_* values.
  // https://github.com/home-assistant/core/blob/master/homeassistant/components/climate/const.py#L3-L23

  String hppower = String(hpSettings.power);
  if (hppower.equalsIgnoreCase("off"))
  {
    return F("off");
  }

  String hpmode = String(hpSettings.mode);
  hpmode.toLowerCase();

  if (hpmode == "fan")
    return F("fan_only");
  else if (hpmode == "auto")
    return F("heat_cool");
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
    return F("off");
  }

  String hpmode = String(hpSettings.mode);
  hpmode.toLowerCase();

  if (hpmode == "fan")
    return F("fan");
  else if (!hpStatus.operating)
    return F("idle");
  else if (hpmode == "auto")
  {
    // If the "operating" flag is true and the heat pump is in "auto" mode,
    // it's either heating or cooling, but its status packets don't explicitly
    // indicate which of those two states it's in. We can infer the state by
    // comparing the room temperature to the set point temperature.
    if (hpStatus.roomTemperature > hpSettings.temperature) // above set point
      return F("cooling");
    else if (hpStatus.roomTemperature < hpSettings.temperature) // below set point
      return F("heating");
    else
      return hpmode; // unknown
  }
  else if (hpmode == "cool")
    return F("cooling");
  else if (hpmode == "heat")
    return F("heating");
  else if (hpmode == "dry")
    return F("drying");
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
  rootInfo[getEntityTag(ENT_ROOM_TEMPERATURE)] = roomTemperature;
  rootInfo["temperature"] = temperature;
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
  rootInfo[getEntityTag(ENT_COMPR_FRQ)] = currentStatus.compressorFrequency;
  if (mqttClient != nullptr && mqttClient->connected())
  {
    String mqttOutput;
    serializeJson(rootInfo, mqttOutput);
    if (!mqttClient->publish(ha_state_topic.c_str(), 1, false, mqttOutput.c_str()))
    {
      if (debugLogsEnabled)
        mqttClient->publish(ha_debug_logs_topic.c_str(), 1, false, (char *)("Failed to publish hp status change"));
    }
  }
}

static void hpCheckRemoteTemp()
{
  if (remoteTempActive && (millis() - lastRemoteTemp > CHECK_REMOTE_TEMP_INTERVAL_MS))
  { // if it's been 5 minutes since last remote_temp message, revert back to HP internal temp sensor
    remoteTempActive = false;
    float temperature = 0;
    hp.setRemoteTemperature(temperature);
    hp.update();
  }
}

void hpPacketDebug(byte *packet, unsigned int length, const char *packetDirection)
{
  if (debugPacketsEnabled)
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
    if (debugPacketsEnabled)
      mqttClient->publish(ha_debug_pckts_topic.c_str(), 1, false, mqttOutput.c_str());
    if (!mqttClient->publish(ha_state_topic.c_str(), 1, false, mqttOutput.c_str()))
    {
      if (debugLogsEnabled)
        mqttClient->publish(ha_debug_logs_topic.c_str(), 1, false, (char *)("Failed to publish dummy hp status change"));
    }
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


void startHeatPump()
{
  lastHpSync = 0;
  hpConnectionRetries = 0;
  hpConnectionTotalRetries = 0;
  hp.setSettingsChangedCallback(hpSettingsChanged);
  hp.setStatusChangedCallback(hpStatusChanged);
  hp.setPacketCallback(hpPacketDebug);
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
  hp.setFastSync(true); // enable fast sync because we are not care about timer and other package
}


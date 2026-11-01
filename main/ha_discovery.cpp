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

// Home Assistant MQTT discovery documents. JSON keys and templates are unchanged.

#include "ha_discovery.h"
#include "system_util.h"
#include "network.h"

static String haGetConfigTopic(String entity_type, String entity_tag = "");
static void haConfigureDevice(DynamicJsonDocument &haConfig);
static void haConfigSensor(byte tag_id, String unit, String icon, bool is_diagnostic = false);
static void haConfigButton(byte tag_id, String payload_press, String icon);
static void haConfigOption(uint8_t tag_id, String icon);
static void haConfigClimate();

// Lookup tables for Tag lookup
static const char* const entityTagLUT[MAX_ENTITY_ID + 1] = {
    /* 0 */ "room_temperature",
    /* 1 */ "connection_state",
    /* 2 */ "up_time",
    /* 3 */ "free_heap",
    /* 4 */ "rssi",
    /* 5 */ "bssi",
    /* 6 */ "compressor_freq",
    /* 7 */ "restart",
    /* 8 */ "webpanel"};

// Lookup tables for Name lookup
static const char* const entityNameLUT[MAX_ENTITY_ID + 1] = {
    /* 0 */ "Room Temperature",
    /* 1 */ "Connection state",
    /* 2 */ "Up Time",
    /* 3 */ "Free Heap",
    /* 4 */ "RSSI",
    /* 5 */ "BSSI",
    /* 6 */ "Compressor Freq",
    /* 7 */ "Restart",
    /* 8 */ "WebPanel"};

//Fast lookup functions for Tag

//Fast lookup functions for Tag
const char* getEntityTag(byte tag_id) {
    if (tag_id <= MAX_ENTITY_ID) {
        return entityTagLUT[tag_id];
    }
    return "unknown";
}

//Fast lookup functions for Name
const char* getEntityName(byte tag_id) {
    if (tag_id <= MAX_ENTITY_ID) {
        return entityNameLUT[tag_id];
    }
    return "Unknown";
}

static String haGetConfigTopic(String entity_type, String entity_tag)
{
  String ha_topic;

  ha_topic = (others_haa ? others_haa_topic : "homeassistant")  + "/" + entity_type + "/";
  if (mqtt_fn.isEmpty()) {
      mqtt_fn = getId();
  }
  ha_topic += mqtt_fn + "/";
  if (!entity_tag.isEmpty()) {
      ha_topic += entity_tag + "/";
  }
  ha_topic += "config";
  return ha_topic;
}

static void haConfigureDevice(DynamicJsonDocument &haConfig)
{
  const size_t capacity = JSON_ARRAY_SIZE(15) + JSON_OBJECT_SIZE(30) + 50;
  DynamicJsonDocument haConnInfo(capacity);  

  String dev_id = getId();

  // device info object
  JsonObject haConfigDevice = haConfig.createNestedObject(F("dev"));
  
  // identifiers array
  haConfigDevice.createNestedArray(F("ids"))[0] = mqtt_fn + "_" + dev_id;

  // connection info (mac)
  haConnInfo.createNestedArray();
  haConnInfo[0] = F("mac");
  haConnInfo[1] = dev_id;
  haConfigDevice.createNestedArray("cns")[0] = haConnInfo;

  // other device infos
  haConfigDevice[F("name")] = mqtt_fn;
  haConfigDevice[F("sw")] = String(appName) + " " + String(getAppVersion());
#ifdef ESP32
  String hardware = String(CONFIG_IDF_TARGET);
  hardware.toUpperCase();
#else
  String hardware = String(ARDUINO_BOARD);
#endif
  haConfigDevice[F("hw")] = hardware;
  haConfigDevice[F("mdl")] = model;
  haConfigDevice[F("mf")] = manufacturer;
  if (!webPanelDisabled)
    haConfigDevice[F("cu")] = "http://" + WiFi.localIP().toString();

  // availability topic
  haConfig[F("avty_t")] = ha_availability_topic;
  haConfig[F("pl_avail")] = mqtt_payload_available;       // MQTT online message payload
  haConfig[F("pl_not_avail")] = mqtt_payload_unavailable; // MQTT offline message payload
}

static void haConfigSensor(byte tag_id, String unit, String icon, bool is_diagnostic)
{
  // send HA config packet for up time
  const size_t capacity = JSON_ARRAY_SIZE(15) + JSON_OBJECT_SIZE(30) + 250;
  DynamicJsonDocument haConfig(capacity);

  haConfig[F("icon")] = icon;
  haConfig[F("name")] = getEntityName(tag_id);
  
  // Set unique ID and value template
  String tag = getEntityTag(tag_id);
  haConfig[F("unique_id")] = getId() + "_" + tag;
  haConfig[F("val_tpl")] = "{{ value_json." + tag + " }}";

  if (tag_id == ENT_ROOM_TEMPERATURE)
  {
    haConfig[F("dev_cla")] = "temperature";
    haConfig[F("unit_of_meas")] = useFahrenheit ? F("°F") : F("°C");
    haConfig[F("stat_t")] = ha_state_topic;
  }
  else if (tag_id == ENT_COMPR_FRQ)
  {
    haConfig[F("dev_cla")] = "frequency";
    haConfig[F("unit_of_meas")] = unit;
    haConfig[F("stat_t")] = ha_state_topic;
  }
  else if (tag_id == ENT_CONNECTION_STATE)
  {
    haConfig[F("dev_cla")] = "connectivity";
    haConfig[F("payload_on")] = "online";
    haConfig[F("payload_off")] = "offline";
    haConfig[F("stat_t")] = ha_system_info_topic;
  }
  else if (tag_id == ENT_UP_TIME)
  {
    haConfig[F("dev_cla")] = "timestamp";
    haConfig[F("val_tpl")] = "{{ as_datetime(value_json." + tag + ") }}";
    // haConfig[F("unit_of_meas")] = unit;
    haConfig[F("stat_t")] = ha_system_info_topic;
  }
  else if (tag_id == ENT_FREE_HEAP)
  {
    haConfig[F("unit_of_meas")] = unit;
    haConfig[F("sug_dsp_prc")] = 0;
    haConfig[F("stat_t")] = ha_system_info_topic;
  }
  else if (tag_id == ENT_RSSI)
  {
    haConfig[F("unit_of_meas")] = unit;
    haConfig[F("stat_t")] = ha_system_info_topic;
  }
  else if (tag_id == ENT_BSSI)
  {
    haConfig[F("stat_t")] = ha_system_info_topic;
  }

  if (is_diagnostic)
    haConfig[F("ent_cat")] = F("diagnostic");

  // add device info
  haConfigureDevice(haConfig);
  
  String mqttOutput;
  serializeJson(haConfig, mqttOutput);

  String ha_entity_type;
  if (tag_id == ENT_CONNECTION_STATE)
  {
    ha_entity_type = F("binary_sensor");
  }
  else
  {
    ha_entity_type = F("sensor");
  }

  String ha_config_topic = haGetConfigTopic(ha_entity_type, tag);
  mqttClient->publish(ha_config_topic.c_str(), 1, true, mqttOutput.c_str());
}

static void haConfigButton(byte tag_id, String payload_press, String icon)
{
  // send HA config packet for button
  const size_t capacity = JSON_ARRAY_SIZE(15) + JSON_OBJECT_SIZE(30) + 250;
  DynamicJsonDocument haConfig(capacity);

  haConfig[F("icon")] = icon;
  haConfig[F("name")] = getEntityName(tag_id);
  
  // Set unique ID and value template
  String tag = getEntityTag(tag_id);
  haConfig[F("unique_id")] = getId() + "_" + tag;

  haConfig[F("dev_cla")] = payload_press;
  haConfig[F("payload_press")] = payload_press; //"restart", "factory", "upgrade" ;
  haConfig[F("command_topic")] = ha_system_set_topic;
  haConfig[F("ent_cat")] = F("config");
  
  // add device info
  haConfigureDevice(haConfig);

  String mqttOutput;
  serializeJson(haConfig, mqttOutput);
  String ha_config_topic = haGetConfigTopic("button", tag);
  mqttClient->publish(ha_config_topic.c_str(), 1, true, mqttOutput.c_str());
}

static void haConfigOption(uint8_t tag_id, String icon) {
    const size_t capacity = JSON_ARRAY_SIZE(15) + JSON_OBJECT_SIZE(30) + 250;
    DynamicJsonDocument haConfig(capacity);

    haConfig[F("icon")] = icon;
    haConfig[F("name")] = getEntityName(tag_id);
    // Set unique ID and value template
    String tag = getEntityTag(tag_id);
    haConfig[F("unique_id")] = getId() + "_" + tag;
    haConfig[F("command_template")] = "{\"options\": {\"" + tag + "\": \"{{ value }}\" } }";
    haConfig[F("command_topic")] = ha_system_setting_request;

    JsonArray haConfigOptions = haConfig[F("options")].to<JsonArray>();
    if (tag_id == ENT_WEB_PANEL) {
        haConfigOptions.add("On");
        haConfigOptions.add("Off");
    }
    haConfig[F("state_topic")] = ha_system_setting_info;
    haConfig[F("value_template")] = "{{ value_json." + tag + " }}";
    haConfig[F("entity_category")] = F("config");

    haConfigureDevice(haConfig);

    String mqttOutput;
    serializeJson(haConfig, mqttOutput);
    String ha_config_topic = haGetConfigTopic("select", tag);
    mqttClient->publish(ha_config_topic.c_str(), 1, true, mqttOutput.c_str());
}

void sendDeviceInfo()
{
  // send HA config packet for device info
  uint32_t freeHeapBytes = getFreeHeapBytes();
  uint32_t totalHeapBytes = getTotalHeapBytes();

  const size_t capacity = JSON_OBJECT_SIZE(10);
  DynamicJsonDocument haConfigInfo(capacity);

  haConfigInfo[getEntityTag(ENT_CONNECTION_STATE)] = hp.isConnected() ? "online" : "offline";
  // get free heap in percent
  // we round to 0.5 (half) to avoid continue changes
  //float percentageHeapFree = 0.5 * round(2.0*(freeHeapBytes * 100.0f / (float)totalHeapBytes));
  // we round to avoid continue changes
  float percentageHeapFree = round(freeHeapBytes * 100.0f / (float)totalHeapBytes);
  String heap(percentageHeapFree);
  haConfigInfo[getEntityTag(ENT_FREE_HEAP)] = heap;
  // get wifi rssi
  haConfigInfo[getEntityTag(ENT_RSSI)] = String(WiFi.RSSI());
  haConfigInfo[getEntityTag(ENT_BSSI)] = getWifiBSSID();
  haConfigInfo[getEntityTag(ENT_UP_TIME)] = getUpTimeSeconds();
  haConfigInfo[getEntityTag(ENT_WEB_PANEL)] = webPanelDisabled ? "Off" : "On";

  String mqttOutput;
  serializeJson(haConfigInfo, mqttOutput);
  mqttClient->publish(ha_system_info_topic.c_str(), 1, false, mqttOutput.c_str());
}

static void haConfigClimate()
{
  // send HA config packet
  // setup HA payload device
  const size_t capacity = JSON_ARRAY_SIZE(5) + 2 * JSON_ARRAY_SIZE(6) + 2 * JSON_ARRAY_SIZE(7) + JSON_OBJECT_SIZE(24) + 2500;
  DynamicJsonDocument haConfig(capacity);

  haConfig[F("name")] = nullptr;
  haConfig[F("unique_id")] = getId();

  JsonArray haConfigModes = haConfig.createNestedArray(F("modes"));
  haConfigModes.add(F("heat_cool")); // native AUTO mode
  haConfigModes.add(F("cool"));
  haConfigModes.add(F("dry"));
  if (supportHeatMode)
  {
    haConfigModes.add(F("heat"));
  }
  haConfigModes.add(F("fan_only")); // native FAN mode
  haConfigModes.add(F("off"));

  haConfig[F("mode_cmd_t")] = ha_mode_set_topic;
  haConfig[F("mode_stat_t")] = ha_state_topic;
  haConfig[F("mode_stat_tpl")] = F("{{ value_json.mode if (value_json is defined and value_json.mode is defined and value_json.mode|length) else 'off' }}"); // Set default value for fix "Could not parse data for HA"
  haConfig[F("temp_cmd_t")] = ha_temp_set_topic;
  haConfig[F("temp_stat_t")] = ha_state_topic;
  haConfig[F("pow_cmd_t")] = ha_power_set_topic;

  // Set default value for fix "Could not parse data for HA"
  String temp_stat_tpl_str = F("{% if (value_json is defined and value_json.temperature is defined) %}{% if (value_json.temperature|int >= ");
  temp_stat_tpl_str += (String)convertCelsiusToLocalUnit(min_temp, useFahrenheit) + " and value_json.temperature|int <= ";
  temp_stat_tpl_str += (String)convertCelsiusToLocalUnit(max_temp, useFahrenheit) + ") %}{{ value_json.temperature }}";
  temp_stat_tpl_str += "{% elif (value_json.temperature|int < " + (String)convertCelsiusToLocalUnit(min_temp, useFahrenheit) + ") %}" + (String)convertCelsiusToLocalUnit(min_temp, useFahrenheit) + "{% elif (value_json.temperature|int > " + (String)convertCelsiusToLocalUnit(max_temp, useFahrenheit) + ") %}" + (String)convertCelsiusToLocalUnit(max_temp, useFahrenheit) + "{% endif %}{% else %}" + (String)convertCelsiusToLocalUnit(22, useFahrenheit) + "{% endif %}";
  haConfig[F("temp_stat_tpl")] = temp_stat_tpl_str;
  haConfig[F("curr_temp_t")] = ha_state_topic;
  String curr_temp_tpl_str = F("{{ value_json.room_temperature if (value_json is defined and value_json.room_temperature is defined and value_json.room_temperature|int > ");
  curr_temp_tpl_str += (String)convertCelsiusToLocalUnit(1, useFahrenheit) + ") }}"; // Set default value for fix "Could not parse data for HA"
  haConfig[F("curr_temp_tpl")] = curr_temp_tpl_str;
  haConfig[F("min_temp")] = convertCelsiusToLocalUnit(min_temp, useFahrenheit);
  haConfig[F("max_temp")] = convertCelsiusToLocalUnit(max_temp, useFahrenheit);
  haConfig[F("temp_step")] = temp_step;
  haConfig[F("temperature_unit")] = useFahrenheit ? F("F") : F("C");

  // fan control
  JsonArray haConfigFan_modes = haConfig.createNestedArray(F("fan_modes"));
  haConfigFan_modes.add(F("auto"));  //AUTO
  if (supportQuietMode)
  {
    haConfigFan_modes.add(F("diffuse")); //QUIET
  }
  haConfigFan_modes.add(F("low")); //1 native
  haConfigFan_modes.add(F("medium")); //2 native
  haConfigFan_modes.add(F("middle")); //3 native
  haConfigFan_modes.add(F("high")); //4 native

  haConfig[F("fan_mode_cmd_t")] = ha_fan_set_topic;
  haConfig[F("fan_mode_stat_t")] = ha_state_topic;
  haConfig[F("fan_mode_stat_tpl")] = F("{{ value_json.fan if (value_json is defined and value_json.fan is defined and value_json.fan|length) else 'auto' }}"); // Set default value for fix "Could not parse data for HA"

  // vertical swing mode control
  JsonArray haConfigSwing_modes = haConfig.createNestedArray(F("swing_modes"));
  haConfigSwing_modes.add(F("AUTO"));
  haConfigSwing_modes.add(F("1"));
  haConfigSwing_modes.add(F("2"));
  haConfigSwing_modes.add(F("3"));
  haConfigSwing_modes.add(F("4"));
  haConfigSwing_modes.add(F("5"));
  haConfigSwing_modes.add(F("SWING"));

  haConfig[F("swing_mode_cmd_t")] = ha_vane_set_topic;
  haConfig[F("swing_mode_stat_t")] = ha_state_topic;
  haConfig[F("swing_mode_stat_tpl")] = F("{{ value_json.vane if (value_json is defined and value_json.vane is defined and value_json.vane|length) else 'AUTO' }}"); // Set default value for fix "Could not parse data for HA"

  // horizontal swing mode control
  JsonArray haConfigSwing_H_modes = haConfig.createNestedArray(F("swing_h_modes"));
  haConfigSwing_H_modes.add(F("<<"));
  haConfigSwing_H_modes.add(F("<"));
  haConfigSwing_H_modes.add(F("|"));
  haConfigSwing_H_modes.add(F(">"));
  haConfigSwing_H_modes.add(F(">>"));
  haConfigSwing_H_modes.add(F("<>"));
  haConfigSwing_H_modes.add(F("SWING"));

  haConfig[F("swing_h_mode_cmd_t")] = ha_wide_vane_set_topic;
  haConfig[F("swing_h_mode_stat_t")] = ha_state_topic;
  haConfig[F("swing_h_mode_stat_tpl")] = F("{{ value_json.wideVane if (value_json is defined and value_json.wideVane is defined and value_json.wideVane|length) else 'SWING' }}"); // Set default value for fix "Could not parse data for HA"

  // action control topic
  haConfig[F("action_topic")] = ha_state_topic;
  haConfig[F("action_template")] = F("{{ value_json.action if (value_json is defined and value_json.action is defined and value_json.action|length) else 'idle' }}"); // Set default value for fix "Could not parse data for HA"

  // add device info
  haConfigureDevice(haConfig);

  String mqttOutput;
  serializeJson(haConfig, mqttOutput);
  String ha_config_topic = haGetConfigTopic("climate");
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
  haConfigOption(ENT_WEB_PANEL, "mdi:cog");
}

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

// MQTT client setup and inbound topic handling. Payloads and topic suffixes are unchanged.

#include "mqtt_client.h"
#include "settings_store.h"
#include "heatpump_bridge.h"
#include "ha_discovery.h"
#include "system_util.h"

static void mqttCallback(const char *topic, const uint8_t *payload, const unsigned int length);
static void onMqttConnect(bool sessionPresent);
static void onMqttDisconnect(espMqttClientTypes::DisconnectReason reason);
static void onMqttSubscribe(uint16_t packetId, const espMqttClientTypes::SubscribeReturncode *codes, size_t len);
static void onMqttUnsubscribe(uint16_t packetId);
static void onMqttMessage(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, size_t len, size_t index, size_t total);
static void onMqttPublish(uint16_t packetId);

#ifdef ESP32
// Let Encrypt isrgrootx1.pem
static const char rootCA_LE[] = R"====(
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


void buildMqttTopics()
{
  String main_topic = mqtt_topic + F("/") + mqtt_fn;
  ha_power_set_topic = main_topic + F("/power/set");
  ha_mode_set_topic = main_topic + F("/mode/set");
  ha_temp_set_topic = main_topic + F("/temp/set");
  ha_remote_temp_set_topic = main_topic + F("/remote_temp/set");
  ha_fan_set_topic = main_topic + F("/fan/set");
  ha_vane_set_topic = main_topic + F("/vane/set");
  ha_wide_vane_set_topic = main_topic + F("/wide-vane/set");
  ha_debug_pckts_topic = main_topic + F("/debug/packets");
  ha_debug_pckts_set_topic = main_topic + F("/debug/packets/set");
  ha_debug_logs_topic = main_topic + F("/debug/logs");
  ha_debug_logs_set_topic = main_topic + F("/debug/logs/set");
  ha_state_topic = main_topic + F("/state");
  // Device info and the web-panel select both publish on /system/info.
  ha_system_info_topic = main_topic + F("/system/info");
  ha_system_set_topic = main_topic + F("/system/set");
  ha_system_setting_info = main_topic + F("/system/info");
  ha_system_setting_request = main_topic + F("/system/opt/rqt");
  ha_system_setting_respond = main_topic + F("/system/opt/rps");
  ha_custom_packet = main_topic + F("/custom/send");
  ha_availability_topic = main_topic + F("/availability");
  ha_birth_topic = (others_haa ? others_haa_topic : F("homeassistant")) + F("/status");
}

void initMqtt()
{
  ESP_LOGD(TAG, "Setup Async Mqtt...");
  if (mqttClient != nullptr)
  {
    delete mqttClient;
    mqttClient = nullptr;
  }
  // Port 8883 selects TLS. ESP32 builds a secure client; other targets leave the client unset.
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

void sendKeepAlive(bool force)
{
  if ((millis() - lastAliveMsgSend < SEND_ALIVE_MSG_INTERVAL_MS) && !force)
  {
    return;
  }
  lastAliveMsgSend = millis();

  // send keep alive message
  if (mqttClient != nullptr && mqttClient->connected())
  {
    if (!mqttClient->publish(ha_availability_topic.c_str(), 1, false, mqtt_payload_available))
    {
      if (debugLogsEnabled)
        mqttClient->publish(ha_debug_logs_topic.c_str(), 1, false, (char *)"Failed to publish avialable status");
    }
    sendDeviceInfo();
    if (hp.isConnected())
    {
        hpStatusChanged(hp.getStatus());
    }
  }
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

static void mqttCallback(const char *topic, const uint8_t *payload, const unsigned int length)
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
    if (modeUpper == "OFF") {
        hp.setPowerSetting(modeUpper.c_str());
        update = true;
    } else if (modeUpper == "ON") {
        // Set temp and mode
        heatpumpSettings currentSettings = hp.getSettings();
        hp.setModeSetting(currentSettings.mode);
        rootInfo["mode"] = hpGetMode(currentSettings);
        //
        float temperature_c = convertLocalUnitToCelsius(currentSettings.temperature, useFahrenheit);
        if (temperature_c < min_temp || temperature_c > max_temp) {
            temperature_c = 23;
            rootInfo["temperature"] = convertCelsiusToLocalUnit(temperature_c, useFahrenheit);
        } else {
            rootInfo["temperature"] = temperature_c;
        }
        hp.setTemperature(temperature_c);
        hp.setPowerSetting(modeUpper.c_str());
        hpSendLocalState();
        update = true;
    }
  }
  else if (strcmp(topic, ha_mode_set_topic.c_str()) == 0)
  {
    String modeUpper = message;
    modeUpper.toUpperCase();
    if (modeUpper == "OFF")
    {
      rootInfo["mode"] = F("off");
      rootInfo["action"] = F("off");
      hpSendLocalState();
      hp.setPowerSetting("OFF");
      update = true;
    }
    else
    {
      if (modeUpper == "HEAT_COOL")
      {
        rootInfo["mode"] = F("heat_cool");
        rootInfo["action"] = F("idle");
        modeUpper = F("AUTO");
      }
      else if (modeUpper == "HEAT")
      {
        rootInfo["mode"] = F("heat");
        rootInfo["action"] = F("heating");
      }
      else if (modeUpper == "COOL")
      {
        rootInfo["mode"] = F("cool");
        rootInfo["action"] = F("cooling");
      }
      else if (modeUpper == "DRY")
      {
        rootInfo["mode"] = F("dry");
        rootInfo["action"] = F("drying");
      }
      else if (modeUpper == "FAN_ONLY")
      {
        rootInfo["mode"] = F("fan_only");
        rootInfo["action"] = F("fan");
        modeUpper = F("FAN");
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
      debugPacketsEnabled = true;
      saveCurrentOthers();
      mqttClient->publish(ha_debug_pckts_topic.c_str(), 1, false, (char *)("Debug packets mode enabled"));
    }
    else if (strcmp(message, "off") == 0)
    {
      debugPacketsEnabled = false;
      saveCurrentOthers();
      mqttClient->publish(ha_debug_pckts_topic.c_str(), 1, false, (char *)("Debug packets mode disabled"));
    }
  }
  else if (strcmp(topic, ha_debug_logs_set_topic.c_str()) == 0)
  { // if the incoming message is on the heatpump_debug_set_topic topic...
    if (strcmp(message, "on") == 0)
    {
      debugLogsEnabled = true;
      saveCurrentOthers();
      mqttClient->publish(ha_debug_logs_topic.c_str(), 1, false, (char *)"Debug mode enabled");
    }
    else if (strcmp(message, "off") == 0)
    {
      debugLogsEnabled = false;
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
  else if (strcmp(topic, ha_birth_topic.c_str()) == 0)
  { // We receive birth topic from ha
    if (strcmp(message, mqtt_payload_available) == 0)
      sendKeepAlive(true);
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
      // Later tokens split on three spaces. That separator is part of the historical parser.
      nextByte = strtok(NULL, "   ");
      byteCount++;
    }

    // dump the packet so we can see what it is. handy because you can run the code without connecting the ESP to the heatpump, and test sending custom packets
    hpPacketDebug(bytes, byteCount, "customPacket");

    hp.sendCustomPacket(bytes, byteCount);
  }
  else if (strcmp(topic, ha_system_setting_request.c_str()) == 0) // We receive command for board
  {
      // Allocate document capacity.
      const size_t capacity = JSON_OBJECT_SIZE(3) + 121;
      DynamicJsonDocument doc(capacity);
      // Deserialize the JSON document
      DeserializationError error = deserializeJson(doc, message);
      // Test if parsing succeeds.
      if (!error) {
          if (doc.containsKey("options")) {
              JsonObject options = doc["options"];
              if (options.containsKey("webpanel")) {
                  String webPanel = doc["options"]["webpanel"];
                  ESP_LOGI(TAG, "Web panel option: %s", webPanel.c_str());
                  if (webPanel == "On" || webPanel == "Off") {
                      bool new_web_panel_disable = false;
                      if (webPanel == "On") {
                          new_web_panel_disable = false;
                      }
                      if (webPanel == "Off") {
                          new_web_panel_disable = true;
                      }
                      if (webPanelDisabled != new_web_panel_disable) {
                          ESP_LOGI(TAG, "Set Webpanel option and reboot");
                          webPanelDisabled = new_web_panel_disable;
                          saveCurrentOthers();
                          sendRebootRequest(5);
                          mqttClient->publish(ha_system_setting_respond.c_str(), 1, false, message);
                      } else {
                          ESP_LOGE(TAG, "Set Web panel option do nothing");
                      }
                  } else {
                      ESP_LOGE(TAG, "Web panel Invalid option");
                  }
              }
          }
      } else {
          ESP_LOGE(TAG, "Error decode json data");
      }
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

static void onMqttConnect(bool sessionPresent)
{
  ESP_LOGD(TAG, "Connected to MQTT. Session present: %d", sessionPresent);
  mqtt_connected = true;

  mqttClient->subscribe(ha_system_set_topic.c_str(), 1);
  mqttClient->subscribe(ha_system_setting_request.c_str(), 1);
  mqttClient->subscribe(ha_debug_pckts_set_topic.c_str(), 1);
  mqttClient->subscribe(ha_debug_logs_set_topic.c_str(), 1);
  mqttClient->subscribe(ha_mode_set_topic.c_str(), 1);
  mqttClient->subscribe(ha_fan_set_topic.c_str(), 1);
  mqttClient->subscribe(ha_temp_set_topic.c_str(), 1);
  mqttClient->subscribe(ha_vane_set_topic.c_str(), 1);
  mqttClient->subscribe(ha_wide_vane_set_topic.c_str(), 1);
  mqttClient->subscribe(ha_remote_temp_set_topic.c_str(), 1);
  mqttClient->subscribe(ha_custom_packet.c_str(), 1);
  mqttClient->subscribe(ha_birth_topic.c_str(), 1);
  // send online message
  mqttClient->publish(ha_availability_topic.c_str(), 1, false, mqtt_payload_available);
  sendHaConfig();
}

static void onMqttDisconnect(espMqttClientTypes::DisconnectReason reason)
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

static void onMqttSubscribe(uint16_t packetId, const espMqttClientTypes::SubscribeReturncode *codes, size_t len)
{
  for (size_t i = 0; i < len; ++i)
  {
    ESP_LOGD(TAG, "Subscribe acknowledged. packetId: %d, qos: %d", packetId, static_cast<uint8_t>(codes[i]));
  }
}

static void onMqttUnsubscribe(uint16_t packetId)
{
  ESP_LOGD(TAG, "Unsubscribe acknowledged. packetId:  %d", packetId);
}

static void onMqttMessage(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, size_t len, size_t index, size_t total)
{
  ESP_LOGD(TAG, "Publish received. topic: %s, qos: %d dup: %d, retain: %d", topic, properties.qos, properties.dup, properties.retain);
  ESP_LOGD(TAG, "Publish received. len: %d, index: %d, total: %d", len, index, total);
  mqttCallback(topic, payload, len);
}

static void onMqttPublish(uint16_t packetId)
{
  ESP_LOGD(TAG, "Publish acknowledged. packetId:  %d", packetId);
}

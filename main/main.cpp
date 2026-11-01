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

// Boot and the main loop. Feature work lives in the modules included below.

#include "app_state.h"
#include "settings_store.h"
#include "network.h"
#include "mqtt_client.h"
#include "web_server.h"
#include "heatpump_bridge.h"
#include "ota_util.h"
#include "system_util.h"


void setup()
{
  Serial.begin(115200);
  wifi_list.reserve(200);
  unique_id.reserve(15);
#ifdef ESP32
  Serial.setDebugOutput(true);
  initNVS();
#endif
  ESP_LOGD(TAG, "Starting  %s", appName);
  mountConfigFilesystem();
  pinMode(blueLedPin, OUTPUT);
  ticker.attach(1, tick);
  setDefaults();
  wifi_config = loadWifi();
  if (hostname.isEmpty())
  {
    hostname += hostnamePrefix;
    hostname += getId();
  }
  loadOthers();
  loadUnit();
  applyHostname();
  wifi_reconnect_timeout = 0;
  mqtt_reconnect_timeout = 0;
  if (loadMqtt())
  {
    buildMqttTopics();
    initMqtt();
  }
  if (initWifi())
  {
    if (SPIFFS.exists(console_file))
    {
      SPIFFS.remove(console_file);
    }
    MDNS.begin(hostname);
    if (!webPanelDisabled)
    {
      startStationWebServer();
    }
    ESP_LOGD(TAG, "Connection to HVAC. Stop serial log.");
    startHeatPump();
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
        hpConnectionRetries = min((uint32_t)(hpConnectionRetries + 1u), HP_MAX_RETRIES);
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

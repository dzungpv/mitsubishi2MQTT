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

// Wi-Fi station setup, AP fallback, captive-portal DNS, and scan results.

#include "network.h"
#include "mqtt_client.h"
#include "system_util.h"
#include <climits>
#include <utility>

#ifdef ESP32
static void WiFiEvent(WiFiEvent_t event);
#elif defined(ESP8266)
static void onWifiConnect(const WiFiEventStationModeGotIP &event);
static void onWifiDisconnect(const WiFiEventStationModeDisconnected &event);
#endif


void applyHostname()
{
#ifdef ESP32
  WiFi.setHostname(hostname.c_str());
#else
  WiFi.hostname(hostname.c_str());
#endif
}

boolean initWifi()
{
  bool connectWifiSuccess = true;
  if (!ap_ssid.isEmpty())
  {
    // wifi connection handle
#ifdef ESP32
    mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(mqttConnect));
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

bool connectWifi()
{
  // WiFi.disconnect(true);
  // delay(1000);
  applyHostname();
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
    // keep LED off
    digitalWrite(blueLedPin, blueLedDisabled);
    // Auto reconnected
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    return true;
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
static void WiFiEvent(WiFiEvent_t event)
{
  ESP_LOGD(TAG, "[WiFi-event] event: %d\n", event);
  if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP || event == ARDUINO_EVENT_WIFI_STA_GOT_IP6)
  {
    ESP_LOGD(TAG, "WiFi connected, IP address: %s", WiFi.localIP().toString().c_str());
    if (millis() > mqtt_reconnect_timeout)
    {
      mqtt_reconnect_timeout = millis() + MQTT_RECONNECT_INTERVAL_MS; // only retry next 5 seconds to prevent crash
      ticker.detach();                                                // Stop blinking the LED because now we are connected:)
      // keep LED off
      digitalWrite(blueLedPin, blueLedDisabled);
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
    }
    else
    { // retry connect
      if (millis() > wifi_reconnect_timeout)
      {
        wifi_reconnect_timeout = millis() + WIFI_RECONNECT_INTERVAL_MS; // only retry next 5 seconds to prevent crash
        xTimerStop(mqttReconnectTimer, 0);                              // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
      }
    }
  }
}
#elif defined(ESP8266)

static void onWifiConnect(const WiFiEventStationModeGotIP &event)
{
  ESP_LOGD(TAG, "WiFi connected, IP address: %s", WiFi.localIP().toString().c_str());
  if (millis() > mqtt_reconnect_timeout)
  {
    mqtt_reconnect_timeout = millis() + MQTT_RECONNECT_INTERVAL_MS; // only retry next 5 seconds to prevent crash
    ticker.detach();                                                // Stop blinking the LED because now we are connected:)
    // keep LED off
    digitalWrite(blueLedPin, blueLedDisabled);
    mqttConnect();
    // init and get the time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  }
}

static void onWifiDisconnect(const WiFiEventStationModeDisconnected &event)
{
  ESP_LOGD(TAG, "Disconnected from Wi-Fi.");
  // TODO crash on esp8266
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
}

#endif

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
      // check for ssid with same name in the list
      String bssi = WiFi.SSID(i);
      bool found = false;
      for (int j = 0; j < k; j++) {
        if (top_k_rssi[j] == INT32_MIN)
          break;
        // replace exixting if better rssi
        if (bssi == WiFi.SSID(top_k_idx[j])) {
          if (rssi > top_k_rssi[j]) {
            top_k_rssi[j] = rssi;
            top_k_idx[j] = i;
          }
          found = true;
          break;
        }
      }
      if (!found)
      {
        top_k_rssi[min_index] = rssi;
        top_k_idx[min_index] = i;
      }
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
    if (top_k_rssi[i] == INT32_MIN)
      break;
    int idx = top_k_idx[i];
    String ssid = WiFi.SSID(idx);
    if (!ssid.isEmpty())
    {
      ESP_LOGI(TAG, "Found %s: ", ssid.c_str());
      if (!wifi_list.isEmpty()) {
        wifi_list += ";";
      }
      wifi_list += ssid;
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

String getWifiBSSID()
{
  byte* mac = WiFi.BSSID();
  // Wifi BSSID
  char wifi_bssid[18];
  snprintf(wifi_bssid, 18, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(wifi_bssid);
}

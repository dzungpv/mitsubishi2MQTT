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

// Feature switches. Leave commented unless you intend to rebuild with that feature.
// #define ARDUINO_OTA 1
// #define METRICS 1
// #define MITSU2MQTT_EN_ONLY 1
// #define MITSU2MQTT_VI_ONLY 1
// #define MITSU2MQTT_TEST 1

#include "FS.h"
#ifdef ESP32
#include <WiFi.h>
#include "esp_mac.h"
#include <WiFiUdp.h>
#include <ESPmDNS.h>
#include <AsyncTCP.h>
#include "SPIFFS.h"
#define U_PART U_SPIFFS
extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
}
#include "esp_ota_ops.h"
#include "esp_app_format.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_flash_encrypt.h"
#include "esp_secure_boot.h"
#else
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncTCP.h>
#define U_PART U_FS
#endif
#include <espMqttClient.h>
#include <ESPAsyncWebServer.h>
#define WEBSOCKET_ENABLE 1
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <math.h>
#include <ArduinoOTA.h>
#include <HeatPump.h>
#include <Ticker.h>
#include "time.h"

#ifdef ESP8266
#define ESP_LOGE(tag, format, ...)
#define ESP_LOGW(tag, format, ...)
#define ESP_LOGI(tag, format, ...)
#define ESP_LOGD(tag, format, ...)
#define ESP_LOGV(tag, format, ...)
#endif

static const char *const TAG = "mitsu_cn105";

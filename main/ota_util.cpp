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

// Web firmware upload, ArduinoOTA hook, and secure-boot gate.

#include "ota_util.h"

int uploaderror = 0;
static size_t ota_content_len = 0;


void otaUpdateProgress(size_t prg, size_t sz)
{
  ESP_LOGD(TAG, "Progress: %d%%\n", (prg * 100) / ota_content_len);
}

// Enable OTA only when connected as a client.
#ifdef ARDUINO_OTA
void initOTA()
{
  ArduinoOTA.setHostname(hostname.c_str());
  if (ota_pwd.length() > 0)
  {
    ArduinoOTA.setPassword(ota_pwd.c_str());
  }
  ArduinoOTA.onStart([]()
                     {
                     });
  ArduinoOTA.onEnd([]()
                   {
                   });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        {
                        });
  ArduinoOTA.onError([](ota_error_t error)
                     {
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

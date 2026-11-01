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

// Identity, clock, heap, temperature conversion, factory reset, and reboot requests.

#include "system_util.h"

static float toFahrenheit(float fromCelcius);
static float toCelsius(float fromFahrenheit);

void tick()
{
  // toggle state
  int state = digitalRead(blueLedPin); // get the current state of GPIO2 pin
  digitalWrite(blueLedPin, !state);    // set pin to the opposite state
}

// temperature helper functions
static float toFahrenheit(float fromCelcius)
{
  return round(1.8 * fromCelcius + 32.0);
}

static float toCelsius(float fromFahrenheit)
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

String getMacAddr(bool keepSeparator) {
#ifdef ESP32
    unsigned char mac_base[6] = {0};
    esp_read_mac(mac_base, ESP_MAC_WIFI_STA);
    if (keepSeparator)
    {
      char chipID[19];
      snprintf(chipID, 18, "%02X:%02X:%02X:%02X:%02X:%02X", mac_base[0], mac_base[1], mac_base[2], mac_base[3], mac_base[4], mac_base[5]);
      return String(chipID);
    }

    char chipID[14];
    snprintf(chipID, 13, "%02X%02X%02X%02X%02X%02X", mac_base[0], mac_base[1], mac_base[2], mac_base[3], mac_base[4], mac_base[5]);
    return String(chipID);
#else
    String chipID = WiFi.macAddress();
    if (!keepSeparator)
      chipID.replace(":", "");
    return chipID;
#endif
}

const String& getId() {
  if (unique_id.isEmpty())
    unique_id = getMacAddr(false);
  return unique_id;
}

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
  strftime(strftime_buf, sizeof(strftime_buf), "%b %e %Y %H:%M:%S", &timeinfo);
  return String(strftime_buf);
}

time_t getUpTimeSeconds()
{
  time_t now;

  if (device_boot_time > 0)
    return device_boot_time;

  time(&now);
  // if system time still not set via NTP, we return 0
  if (now < min_valid_date)
    return 0;

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

  device_boot_time = now - secondsSinceBoot;

  return device_boot_time;
}

// Time device running without crash or reboot
String getUpTime()
{
  char strftime_buf[64];
  struct tm timeinfo;

  time_t uptime = getUpTimeSeconds();

  localtime_r(&uptime, &timeinfo);
  strftime(strftime_buf, sizeof(strftime_buf), "%b %e %Y %H:%M:%S", &timeinfo);
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
    char builDate[64];
    sprintf(builDate, "%s %s", __DATE__, __TIME__);
    build_date_time = String(builDate);
    return build_date_time;
  }
  return build_date_time;
}

uint32_t getFreeHeapBytes()
{
#ifdef ESP32
  uint32_t freeHeapBytes = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
#else
  uint32_t freeHeapBytes = ESP.getFreeHeap();
#endif

return freeHeapBytes;
}

uint32_t getTotalHeapBytes()
{
#ifdef ESP32
  uint32_t totalHeapBytes = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
#else
  uint32_t totalHeapBytes = 64000;
#endif

return totalHeapBytes;
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

// Reboot in nextSeconds in the future
void sendRebootRequest(unsigned long nextSeconds)
{
  requestReboot = true;
  requestRebootTime = millis() + nextSeconds * 1000L;
  ESP_LOGI(TAG, "Send Reboot Request");
}

void checkRebootRequest()
{
  // Wait one extra REBOOT_REQUEST_INTERVAL_MS past the requested deadline.
  if (requestReboot and (millis() > requestRebootTime + REBOOT_REQUEST_INTERVAL_MS))
  {
    requestReboot = false;
    requestRebootTime = 0;
    ESP_LOGI(TAG, "Restart device from request");
    ESP.restart();
  }
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

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

// HTTP routes, page rendering, login cookie, and captive-portal handlers.

#include "web_server.h"
#include "settings_store.h"
#include "network.h"
#include "mqtt_client.h"
#include "heatpump_bridge.h"
#include "ota_util.h"
#include "system_util.h"
#include "language_util.h"
#include "htmls/html_common.h"
#include "htmls/javascript_common.h"
#include "htmls/html_init.h"
#include "htmls/html_menu.h"
#include "htmls/html_pages.h"
#ifdef METRICS
#include "htmls/html_metrics.h"
#endif

// Languages supported. Note: the order is important and must match locale_translations.h
#if defined(MITSU2MQTT_TEST) || defined(MITSU2MQTT_EN_ONLY)
// in Debug mode use one language (en) to save flash memory needed for the tests
const char *const languages[] = {MITSU2MQTT_LOCALE_EN};
const char *const language_names[] = {"English"};
#elif defined(MITSU2MQTT_VI_ONLY)
const char *const languages[] = {MITSU2MQTT_LOCALE_VI};
const char *const language_names[] = {"Tiếng Việt"};
#else
const char *const languages[] = {
    MITSU2MQTT_LOCALE_EN,
    MITSU2MQTT_LOCALE_VI,
    MITSU2MQTT_LOCALE_DA,
    MITSU2MQTT_LOCALE_DE,
    MITSU2MQTT_LOCALE_ES,
    MITSU2MQTT_LOCALE_FR,
    MITSU2MQTT_LOCALE_IT,
    MITSU2MQTT_LOCALE_JA,
    MITSU2MQTT_LOCALE_ZH,
    MITSU2MQTT_LOCALE_CA
};
const char *const language_names[] = {
    "English",
    "Tiếng Việt",
    "Dansk",
    "Deutsch",
    "Español",
    "Français",
    "Italiano",
    "日本語",
    "中文",
    "Català"
};
#endif

static constexpr uint8_t NUM_LANGUAGES = sizeof(languages) / sizeof(const char *);

static char *html_response = NULL;
static unsigned int html_resp_length = 0;

// Match every HTTP method on /upload.
// PlatformIO pulls ESPAsyncWebServer 3.9+, which names the matcher
// AsyncWebRequestMethod::HTTP_ANY (HTTP_ALL when the IDF HTTP_ANY macro is
// already defined). The ESP-IDF submodule is 3.8.x and still uses
// WebRequestMethod::HTTP_ANY.
#ifdef ASYNCWEBSERVER_WIFI_SUPPORTED
#ifdef HTTP_ANY
#define ANY_HTTP_METHOD AsyncWebRequestMethod::HTTP_ALL
#else
#define ANY_HTTP_METHOD AsyncWebRequestMethod::HTTP_ANY
#endif
#else
#define ANY_HTTP_METHOD WebRequestMethod::HTTP_ANY
#endif

static String buildLanguageOptions()
{
  String language_list;
  for (uint8_t i = 0; i < NUM_LANGUAGES; i++)
  {
    language_list += "<option value='";
    language_list += i;
    language_list += "'";
    if (i == system_language_index)
    {
      language_list += F("selected");
    }
    language_list += ">";
    language_list += language_names[i];
    language_list += "</option>";
  }
  return language_list;
}


static void sendWrappedHTML(AsyncWebServerRequest *request, const String &content);
static void handleNotFound(AsyncWebServerRequest *request);
static void handleSaveWifiAndMqtt(AsyncWebServerRequest *request);
static void handleReboot(AsyncWebServerRequest *request);
static void handleRoot(AsyncWebServerRequest *request);
static void handleInitSetup(AsyncWebServerRequest *request);
static void handleSetup(AsyncWebServerRequest *request);
static void handleOthers(AsyncWebServerRequest *request);
static void handleMqtt(AsyncWebServerRequest *request);
static void handleUnit(AsyncWebServerRequest *request);
static void handleWifi(AsyncWebServerRequest *request);
static void handleStatus(AsyncWebServerRequest *request);
static String getSelectStatus(const String &curr_status, const String &status);
static String getModeSelect(const String &curr_status);
static String getFanSelect(const String &curr_status);
static String getVaneSelect(const String &curr_status);
static String getWideVaneSelect(const String &curr_status);
static void handleControl(AsyncWebServerRequest *request);
#ifdef METRICS
static void handleMetrics(AsyncWebServerRequest *request);
#endif
static void handleLogin(AsyncWebServerRequest *request);
static void handleUpgrade(AsyncWebServerRequest *request);
static void handleUploadDone(AsyncWebServerRequest *request);
static bool is_authenticated(AsyncWebServerRequest *request);
static void redirectLoginPage(AsyncWebServerRequest *request);
static bool checkLogin(AsyncWebServerRequest *request);
#ifdef WEBSOCKET_ENABLE
static void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
#endif

// Check if header is present and correct
static bool is_authenticated(AsyncWebServerRequest *request)
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

static void redirectLoginPage(AsyncWebServerRequest *request)
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
}

static bool checkLogin(AsyncWebServerRequest *request)
{
  if (!is_authenticated(request) && login_password.length() > 0)
  {
    redirectLoginPage(request);
    return false;
  }
  return true;
}

// Pages are wrapped with the shared header and footer, then sent as one HTML document.
static void sendWrappedHTML(AsyncWebServerRequest *request, const String &content)
{
  if (content.isEmpty())
    return;

  String footer = FPSTR(html_common_footer);
  footer.replace(F("_APP_NAME_"), appName);
  footer.replace(F("_UNIT_NAME_"), hostname);
  #ifdef ESP32
  String hardware = String(CONFIG_IDF_TARGET);
  hardware.toUpperCase();
#else
  String hardware = String(ARDUINO_BOARD);
#endif
  footer.replace(F("_VERSION_"), getAppVersion() + F(" (") + hardware + F(")"));

  String response = FPSTR(html_common_header);
  response.replace(F("_APP_NAME_"), appName);
  response.replace(F("_UNIT_NAME_"), hostname);

#ifdef ESP32
  response += content;
  response += footer;
  request->send(200, "text/html", response);
#else
  // ESP8266 send_P does not copy. Keep the bytes until the next page replaces them.
  if (html_response != NULL)
  {
    delete[] html_response; // cleanup memory when send completed
    html_response = NULL;
  }
  html_resp_length = response.length() + content.length() + footer.length();
  html_response = new char[html_resp_length + 1];
  memcpy(html_response, response.c_str(), response.length());
  u_int16_t index = response.length();
  memcpy(html_response + index, content.c_str(), content.length());
  index += content.length();
  memcpy(html_response + index, footer.c_str(), footer.length());
  html_response[html_resp_length] = '\0';
  request->send_P(200, "text/html", html_response);
#endif
}

static String getSelectStatus(const String &curr_status, const String &status)
{
  if (curr_status.isEmpty())
    return F("");
  if (curr_status == status)
    return F("selected");
  return F("");
}

static String getModeSelect(const String &curr_status)
{
  String modeSelect = FPSTR(html_page_control_mode);
  modeSelect.replace(F("_TXT_CTRL_MODE_"), translatedWord(FL_(txt_ctrl_mode)));
  modeSelect.replace(F("_TXT_F_AUTO_"), translatedWord(FL_(txt_f_auto)));
  modeSelect.replace(F("_TXT_F_HEAT_"), translatedWord(FL_(txt_f_heat)));
  modeSelect.replace(F("_TXT_F_DRY_"), translatedWord(FL_(txt_f_dry)));
  modeSelect.replace(F("_TXT_F_COOL_"), translatedWord(FL_(txt_f_cool)));
  modeSelect.replace(F("_TXT_F_FAN_"), translatedWord(FL_(txt_f_fan)));

  modeSelect.replace(F("_MODE_H_"), getSelectStatus(curr_status, F("HEAT")));
  modeSelect.replace(F("_MODE_D_"), getSelectStatus(curr_status, F("DRY")));
  modeSelect.replace(F("_MODE_C_"), getSelectStatus(curr_status, F("COOL")));
  modeSelect.replace(F("_MODE_F_"), getSelectStatus(curr_status, F("FAN")));
  modeSelect.replace(F("_MODE_A_"), getSelectStatus(curr_status, F("AUTO")));

  modeSelect.replace(F("_HEAT_HIDDEN_"), !supportHeatMode ? F("'hidden' style='display: none;' disabled"): F(""));

  return modeSelect;
}

static String getFanSelect(const String &curr_status)
{
  String fanSelect = FPSTR(html_page_control_fan);
  fanSelect.replace(F("_TXT_CTRL_FAN_"), translatedWord(FL_(txt_ctrl_fan)));
  fanSelect.replace(F("_TXT_F_AUTO_"), translatedWord(FL_(txt_f_auto)));
  fanSelect.replace(F("_TXT_F_QUIET_"), translatedWord(FL_(txt_f_quiet)));
  fanSelect.replace(F("_TXT_F_LOW_"), translatedWord(FL_(txt_f_low)));
  fanSelect.replace(F("_TXT_F_MEDIUM_"), translatedWord(FL_(txt_f_medium)));
  fanSelect.replace(F("_TXT_F_MIDDLE_"), translatedWord(FL_(txt_f_middle)));
  fanSelect.replace(F("_TXT_F_HIGH_"), translatedWord(FL_(txt_f_high)));

  fanSelect.replace(F("_FAN_A_"), getSelectStatus(curr_status, F("AUTO")));
  fanSelect.replace(F("_FAN_Q_"), getSelectStatus(curr_status, F("QUIET")));
  fanSelect.replace(F("_FAN_1_"), getSelectStatus(curr_status, F("1")));
  fanSelect.replace(F("_FAN_2_"), getSelectStatus(curr_status, F("2")));
  fanSelect.replace(F("_FAN_3_"), getSelectStatus(curr_status, F("3")));
  fanSelect.replace(F("_FAN_4_"), getSelectStatus(curr_status, F("4")));

  fanSelect.replace(F("_QUIET_HIDDEN_"), !supportQuietMode ? F("'hidden' style='display: none;' disabled"): F(""));

  return fanSelect;
}

static String getVaneSelect(const String &curr_status)
{
  String vaneSelect = FPSTR(html_page_control_vane);
  vaneSelect.replace(F("_TXT_CTRL_VANE_"), translatedWord(FL_(txt_ctrl_vane)));
  vaneSelect.replace(F("_TXT_F_AUTO_"), translatedWord(FL_(txt_f_auto)));
  vaneSelect.replace(F("_TXT_F_SWING_"), translatedWord(FL_(txt_f_swing)));
  vaneSelect.replace(F("_TXT_F_POS_"), translatedWord(FL_(txt_f_pos)));

  vaneSelect.replace(F("_VANE_A_"), getSelectStatus(curr_status, F("AUTO")));
  vaneSelect.replace(F("_VANE_1_"), getSelectStatus(curr_status, F("1")));
  vaneSelect.replace(F("_VANE_2_"), getSelectStatus(curr_status, F("2")));
  vaneSelect.replace(F("_VANE_3_"), getSelectStatus(curr_status, F("3")));
  vaneSelect.replace(F("_VANE_4_"), getSelectStatus(curr_status, F("4")));
  vaneSelect.replace(F("_VANE_5_"), getSelectStatus(curr_status, F("5")));
  vaneSelect.replace(F("_VANE_S_"), getSelectStatus(curr_status, F("SWING")));

  return vaneSelect;
}

static String getWideVaneSelect(const String &curr_status)
{
  String wideVaneSelect = FPSTR(html_page_control_widevane);
  wideVaneSelect.replace(F("_TXT_CTRL_WVANE_"), translatedWord(FL_(txt_ctrl_wvane)));
  wideVaneSelect.replace(F("_TXT_F_SWING_"), translatedWord(FL_(txt_f_swing)));
  wideVaneSelect.replace(F("_TXT_F_POS_"), translatedWord(FL_(txt_f_pos)));

  wideVaneSelect.replace(F("_WVANE_1_"), getSelectStatus(curr_status, F("<<")));
  wideVaneSelect.replace(F("_WVANE_2_"), getSelectStatus(curr_status, F("<")));
  wideVaneSelect.replace(F("_WVANE_3_"), getSelectStatus(curr_status, F("|")));
  wideVaneSelect.replace(F("_WVANE_4_"), getSelectStatus(curr_status, F(">")));
  wideVaneSelect.replace(F("_WVANE_5_"), getSelectStatus(curr_status, F(">>")));
  // The "<>" option repeats the _WVANE_5_ token, which the line above already replaced.
  wideVaneSelect.replace(F("_WVANE_5_"), getSelectStatus(curr_status, F("<>")));
  wideVaneSelect.replace(F("_WVANE_S_"), getSelectStatus(curr_status, F("SWING")));

  return wideVaneSelect;
}

static void handleNotFound(AsyncWebServerRequest *request)
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

static void handleSaveWifiAndMqtt(AsyncWebServerRequest *request)
{
  if (!checkLogin(request)) {
      return;
  }
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
  initSavePage.replace(F("_TXT_INIT_REBOOT_MES_1_"), translatedWord(FL_(txt_init_reboot_mes_1)));
  initSavePage.replace(F("_TXT_INIT_REBOOT_MES_"), translatedWord(FL_(txt_init_reboot_mes)));
  sendWrappedHTML(request, initSavePage + countDown);
  sendRebootRequest(2); // Reboot after 1 seconds
}

static void handleReboot(AsyncWebServerRequest *request)
{
  if (!checkLogin(request)) {
      return;
  }
  ESP_LOGD(TAG, "Rebooting");
  String initRebootPage = FPSTR(html_init_reboot);
  // localize
  initRebootPage.replace(F("_TXT_INIT_REBOOT_"), translatedWord(FL_(txt_init_reboot)));
  sendWrappedHTML(request, initRebootPage);
  sendRebootRequest(3); // Reboot after 3 seconds
}

static void handleRoot(AsyncWebServerRequest *request)
{
  if (!checkLogin(request)) {
      return;
  }
  if (request->hasArg("REBOOT"))
  {
    String rebootPage = FPSTR(html_page_reboot);
    String countDown = FPSTR(count_down_script);
    // localize
    rebootPage.replace(F("_TXT_M_REBOOT_"), translatedWord(FL_(txt_m_reboot)));
    sendWrappedHTML(request, rebootPage + countDown);
    sendRebootRequest(3); // Reboot after 3 seconds
  }
  else
  {
    String menuRootPage = FPSTR(html_menu_root);
    // localize
    menuRootPage.replace(F("_TXT_HOME_PAGE_"), translatedWord(FL_(txt_home_page)));
    menuRootPage.replace(F("_TXT_CONTROL_"), translatedWord(FL_(txt_control)));
    menuRootPage.replace(F("_TXT_SETUP_"), translatedWord(FL_(txt_setup)));
    menuRootPage.replace(F("_TXT_STATUS_"), translatedWord(FL_(txt_status)));
    menuRootPage.replace(F("_TXT_FW_UPGRADE_"), translatedWord(FL_(txt_firmware_upgrade)));
    menuRootPage.replace(F("_TXT_REBOOT_"), translatedWord(FL_(txt_reboot)));
    menuRootPage.replace(F("_TXT_LOGOUT_"), translatedWord(FL_(txt_logout)));
    // set data
    menuRootPage.replace(F("_SHOW_LOGOUT_"), (String)(login_password.length() > 0));
    // not show control button if hp not connected
    menuRootPage.replace(F("_SHOW_CONTROL_"), (String)(hp.isConnected()));
    sendWrappedHTML(request, menuRootPage);
  }
}

static void handleInitSetup(AsyncWebServerRequest *request)
{
  getWifiList();
  String initSetupPage = FPSTR(html_init_setup);
  String unitScriptWs = FPSTR(unit_script_ws);
  // localize
  initSetupPage.replace(F("_TXT_INIT_TITLE_"), translatedWord(FL_(txt_init_title)));
  initSetupPage.replace(F("_TXT_WIFI_TITLE_"), translatedWord(FL_(txt_wifi_title)));
  initSetupPage.replace(F("_TXT_UNIT_LANGUAGE_"), translatedWord(FL_(txt_unit_language)));
  initSetupPage.replace(F("_TXT_WIFI_SSID_ENTER_"), translatedWord(FL_(txt_wifi_ssid_enter)));
  initSetupPage.replace(F("_TXT_WIFI_SSID_SELECT_"), translatedWord(FL_(txt_wifi_ssid_select)));
  initSetupPage.replace(F("_TXT_WIFI_SSID_"), translatedWord(FL_(txt_wifi_ssid)));
  initSetupPage.replace(F("_TXT_WIFI_PSK_"), translatedWord(FL_(txt_wifi_psk)));
  initSetupPage.replace(F("_TXT_WIFI_STATIC_IP_"), translatedWord(FL_(txt_wifi_static_ip)));
  initSetupPage.replace(F("_TXT_WIFI_STATIC_GW_"), translatedWord(FL_(txt_wifi_static_gw)));
  initSetupPage.replace(F("_TXT_WIFI_STATIC_MASK_"), translatedWord(FL_(txt_wifi_static_mask)));
  initSetupPage.replace(F("_TXT_WIFI_STATIC_DNS_"), translatedWord(FL_(txt_wifi_static_dns)));
  initSetupPage.replace(F("_TXT_MQTT_TITLE_"), translatedWord(FL_(txt_mqtt_title)));
  initSetupPage.replace(F("_TXT_MQTT_PH_USER_"), translatedWord(FL_(txt_mqtt_ph_user)));
  initSetupPage.replace(F("_TXT_MQTT_PH_PWD_"), translatedWord(FL_(txt_mqtt_ph_pwd)));
  initSetupPage.replace(F("_TXT_MQTT_HOST_"), translatedWord(FL_(txt_mqtt_host)));
  initSetupPage.replace(F("_TXT_MQTT_PORT_DESC"), translatedWord(FL_(txt_mqtt_port_desc)));
  initSetupPage.replace(F("_TXT_MQTT_PORT_"), translatedWord(FL_(txt_mqtt_port)));
  initSetupPage.replace(F("_TXT_MQTT_USER_"), translatedWord(FL_(txt_mqtt_user)));
  initSetupPage.replace(F("_TXT_MQTT_PASSWORD_"), translatedWord(FL_(txt_mqtt_password)));
  initSetupPage.replace(F("_TXT_SAVE_"), translatedWord(FL_(txt_save)));
  initSetupPage.replace(F("_TXT_FIRMWARE_UPGRADE_"), translatedWord(FL_(txt_firmware_upgrade)));
  // set the data
  // language
  initSetupPage.replace(F("_LANGUAGE_OPTIONS_"), buildLanguageOptions());
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

static void handleSetup(AsyncWebServerRequest *request)
{
  if (!checkLogin(request)) {
      return;
  }
  if (request->hasArg("RESET"))
  {
    String resetPage = FPSTR(html_page_reset);
    // localize
    resetPage.replace(F("_TXT_M_RESET_1_"), translatedWord(FL_(txt_m_reset_1)));
    resetPage.replace(F("_TXT_M_RESET_"), translatedWord(FL_(txt_m_reset)));
    String countDown = FPSTR(count_down_script);
    sendWrappedHTML(request, resetPage + countDown);
    factoryReset();
    sendRebootRequest(5); // Reboot after 5 seconds
  }
  else
  {
    String menuSetupPage = FPSTR(html_menu_setup);
    // localize
    menuSetupPage.replace(F("_TXT_SETUP_PAGE_"), translatedWord(FL_(txt_setup_page)));
    menuSetupPage.replace(F("_TXT_MQTT_"), translatedWord(FL_(txt_mqtt)));
    menuSetupPage.replace(F("_TXT_WIFI_"), translatedWord(FL_(txt_wifi)));
    menuSetupPage.replace(F("_TXT_UNIT_"), translatedWord(FL_(txt_unit)));
    menuSetupPage.replace(F("_TXT_OTHERS_"), translatedWord(FL_(txt_others)));
    menuSetupPage.replace(F("_TXT_RESET_CONFIRM_"), translatedWord(FL_(txt_reset_confirm)));
    menuSetupPage.replace(F("_TXT_RESET_"), translatedWord(FL_(txt_reset)));
    menuSetupPage.replace(F("_TXT_BACK_"), translatedWord(FL_(txt_back)));
    sendWrappedHTML(request, menuSetupPage);
  }
}

static void handleOthers(AsyncWebServerRequest *request)
{
  if (!checkLogin(request)) {
      return;
  }
  if (request->hasArg("save"))
  {
    saveOthers(request->arg("HAA"), request->arg("haat"), request->arg("DebugPckts"), request->arg("DebugLogs"), request->arg("web_p"), request->arg("tx_pin"), request->arg("rx_pin"), request->arg("tz"), request->arg("ntp"));
    String saveRebootPage = FPSTR(html_page_save_reboot);
    // localize
    saveRebootPage.replace(F("_TXT_M_SAVE_"), translatedWord(FL_(txt_m_save)));
    String countDown = FPSTR(count_down_script);
    sendWrappedHTML(request, saveRebootPage + countDown);
    sendRebootRequest(5); // Reboot after 5 seconds
  }
  else
  {
    String othersPage = FPSTR(html_page_others);
    // localize
    othersPage.replace(F("_TXT_OTHERS_TITLE_"), translatedWord(FL_(txt_others_title)));
    othersPage.replace(F("_TXT_OTHERS_HAAUTO_"), translatedWord(FL_(txt_others_haauto)));
    othersPage.replace(F("_TXT_OTHERS_HATOPIC_"), translatedWord(FL_(txt_others_hatopic)));
    othersPage.replace(F("_TXT_OTHERS_DEBUG_PCKTS_"), translatedWord(FL_(txt_others_debug_packets)));
    othersPage.replace(F("_TXT_OTHERS_DEBUG_LOGS_"), translatedWord(FL_(txt_others_debug_log)));
    othersPage.replace(F("_TXT_OTHERS_WEB_PANEL_"), translatedWord(FL_(txt_others_web_panel)));
    othersPage.replace(F("_TXT_OTHERS_TIME_ZONE_"), translatedWord(FL_(txt_others_tz)));
    othersPage.replace(F("_TXT_OTHER_NTP_SERVER_"), translatedWord(FL_(txt_others_ntp_server)));
    othersPage.replace(F("_SEE_TZ_LIST"), translatedWord(FL_(txt_others_tz_list)));
    othersPage.replace(F("_TXT_TX_PIN_"), translatedWord(FL_(txt_others_tx_pin)));
    othersPage.replace(F("_TXT_RX_PIN_"), translatedWord(FL_(txt_others_rx_pin)));
    othersPage.replace(F("_TXT_F_ON_"), translatedWord(FL_(txt_f_on)));
    othersPage.replace(F("_TXT_F_OFF_"), translatedWord(FL_(txt_f_off)));
    othersPage.replace(F("_TXT_SAVE_"), translatedWord(FL_(txt_save)));
    othersPage.replace(F("_TXT_BACK_"), translatedWord(FL_(txt_back)));
    // disable web panel if MQTT not set or not connected to prevent accident disable
    if ((!mqtt_config || !mqtt_connected) && !captive) {
        othersPage.replace(F("_WEB_PN_EN_"), F("disabled"));
    } else {
        othersPage.replace(F("_WEB_PN_EN_"), F(""));
    }
    // set data
    othersPage.replace(F("_HAA_TOPIC_"), others_haa_topic);
    othersPage.replace(F("_TX_PIN_"), String(HP_TX));
    othersPage.replace(F("_RX_PIN_"), String(HP_RX));
    othersPage.replace(F("_TIME_ZONE_"), timezone);
    othersPage.replace(F("_NTP_SERVER_"), String(ntpServer));
    if (others_haa)
    {
      othersPage.replace(F("_HAA_ON_"), F("selected"));
    }
    else
    {
      othersPage.replace(F("_HAA_OFF_"), F("selected"));
    }
    if (debugPacketsEnabled)
    {
      othersPage.replace(F("_DEBUG_PCKTS_ON_"), F("selected"));
    }
    else
    {
      othersPage.replace(F("_DEBUG_PCKTS_OFF_"), F("selected"));
    }
    if (debugLogsEnabled)
    {
      othersPage.replace(F("_DEBUG_LOGS_ON_"), F("selected"));
    }
    else
    {
      othersPage.replace(F("_DEBUG_LOGS_OFF_"), F("selected"));
    }
    if (webPanelDisabled)
    {
      othersPage.replace(F("_WEB_OFF_"), F("selected"));
    }
    else
    {
      othersPage.replace(F("_WEB_ON_"), F("selected"));
    }
    sendWrappedHTML(request, othersPage);
  }
}

static void handleMqtt(AsyncWebServerRequest *request)
{
  if (!checkLogin(request)) {
      return;
  }
  if (request->hasArg("save"))
  {
    saveMqtt(request->arg("fn"), request->arg("mh"), request->arg("ml"), request->arg("mu"), request->arg("mp"), request->arg("mt"), request->arg("mrcc"));
    String saveRebootPage = FPSTR(html_page_save_reboot);
    // localize
    saveRebootPage.replace(F("_TXT_M_SAVE_"), translatedWord(FL_(txt_m_save)));
    String countDown = FPSTR(count_down_script);
    sendWrappedHTML(request, saveRebootPage + countDown);
    sendRebootRequest(5); // Reboot after 5 seconds
  }
  else
  {
    String mqttPage = FPSTR(html_page_mqtt);
    // localize
    mqttPage.replace(F("_TXT_MQTT_TITLE_"), translatedWord(FL_(txt_mqtt_title)));
    mqttPage.replace(F("_TXT_MQTT_FN_DESC_"), translatedWord(FL_(txt_mqtt_fn_desc)));
    mqttPage.replace(F("_TXT_MQTT_FN_"), translatedWord(FL_(txt_mqtt_fn)));
    mqttPage.replace(F("_TXT_MQTT_PH_USER_"), translatedWord(FL_(txt_mqtt_ph_user)));
    mqttPage.replace(F("_TXT_MQTT_PH_PWD_"), translatedWord(FL_(txt_mqtt_ph_pwd)));
    mqttPage.replace(F("_TXT_MQTT_PH_TOPIC_"), translatedWord(FL_(txt_mqtt_ph_topic)));
    mqttPage.replace(F("_TXT_MQTT_HOST_"), translatedWord(FL_(txt_mqtt_host)));
    mqttPage.replace(F("_TXT_MQTT_PORT_DESC_"), translatedWord(FL_(txt_mqtt_port_desc)));
    mqttPage.replace(F("_TXT_MQTT_PORT_"), translatedWord(FL_(txt_mqtt_port)));
    mqttPage.replace(F("_TXT_MQTT_USER_"), translatedWord(FL_(txt_mqtt_user)));
    mqttPage.replace(F("_TXT_MQTT_PASSWORD_"), translatedWord(FL_(txt_mqtt_password)));
    mqttPage.replace(F("_TXT_MQTT_TOPIC_"), translatedWord(FL_(txt_mqtt_topic)));
    mqttPage.replace(F("_TXT_MQTT_ROOT_CA_CERT_"), translatedWord(FL_(txt_mqtt_root_ca_cert)));
    mqttPage.replace(F("_TXT_SAVE_"), translatedWord(FL_(txt_save)));
    mqttPage.replace(F("_TXT_BACK_"), translatedWord(FL_(txt_back)));
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

static void handleUnit(AsyncWebServerRequest *request)
{
  if (!checkLogin(request)) {
      return;
  }
  if (request->hasArg("save"))
  {
    String loginPassword = request->arg("lpw");
    String confirmLoginPassword = request->arg("lpwc");
    if (loginPassword == confirmLoginPassword)
    {
      saveUnit(request->arg("tu"), request->arg("md"), request->arg("mdf"), loginPassword, request->arg("temp_step"), request->arg("language"));
      String saveRebootPage = FPSTR(html_page_save_reboot);
      // localize
      saveRebootPage.replace(F("_TXT_M_SAVE_"), translatedWord(FL_(txt_m_save)));
      String countDown = FPSTR(count_down_script);
      sendWrappedHTML(request, saveRebootPage + countDown);
      sendRebootRequest(5); // Reboot after 5 seconds
    }
    else
    {
      String saveRebootPage = FPSTR(html_page_save_reboot);
      // localize
      saveRebootPage.replace(F("_TXT_M_SAVE_"), translatedWord(FL_(txt_unit_password_not_match)));
      String countDown = FPSTR(count_down_script);
      sendWrappedHTML(request, saveRebootPage + countDown);
    }
  }
  else
  {
    String unitPage = FPSTR(unit_script_ws);
    unitPage.replace(F("_TXT_UNIT_PASSWORD_NOT_MATCH_"), translatedWord(FL_(txt_unit_password_not_match)));

    unitPage += FPSTR(html_page_unit);
    // localize
    unitPage.replace(F("_TXT_UNIT_TITLE_"), translatedWord(FL_(txt_unit_title)));
    unitPage.replace(F("_TXT_UNIT_LANGUAGE_"), translatedWord(FL_(txt_unit_language)));
    unitPage.replace(F("_TXT_UNIT_TEMP_"), translatedWord(FL_(txt_unit_temp)));
    unitPage.replace(F("_TXT_UNIT_STEPTEMP_"), translatedWord(FL_(txt_unit_steptemp)));
    unitPage.replace(F("_TXT_UNIT_FAN_MODES_"), translatedWord(FL_(txt_unit_fan_modes)));
    unitPage.replace(F("_TXT_UNIT_MODES_"), translatedWord(FL_(txt_unit_modes)));
    unitPage.replace(F("_TXT_UNIT_LOGIN_USERNAME_"), translatedWord(FL_(txt_unit_login_username)));
    unitPage.replace(F("_TXT_UNIT_PASSWORD_CONFIRM_"), translatedWord(FL_(txt_unit_password_confirm)));
    unitPage.replace(F("_TXT_UNIT_PASSWORD_"), translatedWord(FL_(txt_unit_password)));
    unitPage.replace(F("_TXT_F_CELSIUS_"), translatedWord(FL_(txt_f_celsius)));
    unitPage.replace(F("_TXT_F_FH_"), translatedWord(FL_(txt_f_fh)));
    unitPage.replace(F("_TXT_F_ALLMODES_"), translatedWord(FL_(txt_f_allmodes)));
    unitPage.replace(F("_TXT_F_NOHEAT_"), translatedWord(FL_(txt_f_noheat)));
    unitPage.replace(F("_TXT_F_NOQUIET_"), translatedWord(FL_(txt_f_noquiet)));
    unitPage.replace(F("_TXT_SAVE_"), translatedWord(FL_(txt_save)));
    unitPage.replace(F("_TXT_BACK_"), translatedWord(FL_(txt_back)));
    // set data
    // language
    unitPage.replace(F("_LANGUAGE_OPTIONS_"), buildLanguageOptions());
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
    sendWrappedHTML(request, unitPage);
  }
}

static void handleWifi(AsyncWebServerRequest *request)
{
  if (!checkLogin(request)) {
      return;
  }
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
    saveRebootPage.replace(F("_TXT_M_SAVE_"), translatedWord(FL_(txt_m_save)));
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

    String wifiPageHtml = FPSTR(html_page_wifi);
    // localize
    wifiPageHtml.replace(F("_TXT_WIFI_TITLE_"), translatedWord(FL_(txt_wifi_title)));
    wifiPageHtml.replace(F("_TXT_WIFI_HOST_DESC_"), translatedWord(FL_(txt_wifi_hostname_desc)));
    wifiPageHtml.replace(F("_TXT_WIFI_HOST_"), translatedWord(FL_(txt_wifi_hostname)));
    wifiPageHtml.replace(F("_TXT_WIFI_SSID_ENTER_"), translatedWord(FL_(txt_wifi_ssid_enter)));
    wifiPageHtml.replace(F("_TXT_WIFI_SSID_SELECT_"), translatedWord(FL_(txt_wifi_ssid_select)));
    wifiPageHtml.replace(F("_TXT_WIFI_SSID_"), translatedWord(FL_(txt_wifi_ssid)));
    wifiPageHtml.replace(F("_TXT_WIFI_PSK_"), translatedWord(FL_(txt_wifi_psk)));
    wifiPageHtml.replace(F("_TXT_WIFI_OTAP_"), translatedWord(FL_(txt_wifi_otap)));
    wifiPageHtml.replace(F("_TXT_WIFI_STATIC_IP_"), translatedWord(FL_(txt_wifi_static_ip)));
    wifiPageHtml.replace(F("_TXT_WIFI_STATIC_GW_"), translatedWord(FL_(txt_wifi_static_gw)));
    wifiPageHtml.replace(F("_TXT_WIFI_STATIC_MASK_"), translatedWord(FL_(txt_wifi_static_mask)));
    wifiPageHtml.replace(F("_TXT_WIFI_STATIC_DNS_"), translatedWord(FL_(txt_wifi_static_dns)));
    wifiPageHtml.replace(F("_TXT_SAVE_"), translatedWord(FL_(txt_save)));
    wifiPageHtml.replace(F("_TXT_BACK_"), translatedWord(FL_(txt_back)));
    // set data
    String str_ap_ssid = ap_ssid;
    String str_ap_pwd = ap_pwd;
    String str_ota_pwd = ota_pwd;
    str_ap_ssid.replace("'", F("&apos;")); // fix single quote in password and ssid
    str_ap_pwd.replace("'", F("&apos;"));
    str_ota_pwd.replace("'", F("&apos;"));
    // display wifi list
    wifiPageHtml.replace(F("_UNIT_NAME_"), hostname);
    String wifiOptions = getWifiOptions(false);
    if (!wifiOptions.isEmpty())
    {
      wifiPageHtml.replace(F("_WIFI_OPTIONS_"), wifiOptions);
    }
    wifiPageHtml.replace(F("_SSID_"), str_ap_ssid);
    wifiPageHtml.replace(F("_PSK_"), str_ap_pwd);
    wifiPageHtml.replace(F("_OTA_PWD_"), str_ota_pwd);
    wifiPageHtml.replace(F("_WIFI_STATIC_IP_"), wifi_static_ip);
    wifiPageHtml.replace(F("_WIFI_STATIC_GW_"), wifi_static_gateway_ip);
    wifiPageHtml.replace(F("_WIFI_STATIC_MASK_"), wifi_static_subnet);
    wifiPageHtml.replace(F("_WIFI_STATIC_DNS_"), wifi_static_dns_ip);

    String wifiPage = FPSTR(fw_check_script_events);
    wifiPage += wifiPageHtml;
    wifiPageHtml = "";

    sendWrappedHTML(request, wifiPage);
  }
}

static void handleStatus(AsyncWebServerRequest *request)
{
  uint32_t freeHeapBytes = getFreeHeapBytes();
  uint32_t totalHeapBytes = getTotalHeapBytes();
  String statusPage = FPSTR(html_page_status);
  
  // localize
  statusPage.replace(F("_TXT_STATUS_TITLE_"), translatedWord(FL_(txt_status_title)));
  statusPage.replace(F("_TXT_STATUS_HVAC_"), translatedWord(FL_(txt_status_hvac)));
  statusPage.replace(F("_TXT_RETRIES_HVAC_"), translatedWord(FL_(txt_retries_hvac)));
  statusPage.replace(F("_TXT_STATUS_MQTT_"), translatedWord(FL_(txt_status_mqtt)));
  statusPage.replace(F("_TXT_STATUS_WIFI_IP_"), translatedWord(FL_(txt_status_wifi_ip)));
  statusPage.replace(F("_TXT_STATUS_WIFI_"), translatedWord(FL_(txt_status_wifi)));
  statusPage.replace(F("_TXT_BUILD_VERSION_"), translatedWord(FL_(txt_build_version)));
  statusPage.replace(F("_TXT_BUILD_DATE_"), translatedWord(FL_(txt_build_date)));
  statusPage.replace(F("_TXT_STATUS_FREEHEAP_"), translatedWord(FL_(txt_status_freeheap)));
  statusPage.replace(F("_TXT_CURRENT_TIME_"), translatedWord(FL_(txt_current_time)));
  statusPage.replace(F("_TXT_BOOT_TIME"), translatedWord(FL_(txt_boot_time)));
  statusPage.replace(F("_TXT_BACK_"), translatedWord(FL_(txt_back)));
  // set data
  if (request->hasArg("mrconn"))
    mqttConnect();
  String connected = F("<font color='green'><b>");
  connected += translatedWord(FL_(txt_status_connect));
  connected += F("</b></font>");
  String disconnected = F("<font color='red'><b>");
  disconnected += translatedWord(FL_(txt_status_disconnect));
  disconnected += F("</b>(_MQTT_REASON_)</font>");
  disconnected.replace(F("_MQTT_REASON_"), String(mqtt_disconnect_reason));
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
    statusPage.replace(F("_WIFI_IP_"), F("<font color='blue'><b>") + WiFi.localIP().toString() + F("</b></font>"));
  }
  if (mqttClient != nullptr && mqttClient->connected())
    statusPage.replace(F("_MQTT_STATUS_"), connected);
  else
    statusPage.replace(F("_MQTT_STATUS_"), disconnected);
  statusPage.replace(F("_WIFI_STATUS_"), String(WiFi.RSSI()));
  statusPage.replace(F("_WIFI_BSSID_"), getWifiBSSID());
  statusPage.replace(F("_WIFI_MAC_"), getMacAddr());
  statusPage.replace(F("_BUILD_VERSION_"), getAppVersion());
  statusPage.replace(F("_BUILD_DATE_"), getBuildDatetime());

  // calculate free heap and percent
  float percentageHeapFree = freeHeapBytes * 100.0f / (float)totalHeapBytes;
  String heap(freeHeapBytes);
  heap += " (";
  heap += String(percentageHeapFree);
  heap += "% )";
  statusPage.replace(F("_FREE_HEAP_"), heap);
  statusPage.replace(F("_CURRENT_TIME_"), F("<font color='blue'><b>") + getCurrentTime() + F("</b></font>"));
  statusPage.replace(F("_BOOT_TIME_"), F("<font color='orange'><b>") + getUpTime() + F("</b></font>"));
  sendWrappedHTML(request, statusPage);
}

static void handleControl(AsyncWebServerRequest *request)
{
  if (!checkLogin(request)) {
      return;
  }
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
  settings = applyWebControlSettings(request, settings);

  String controlPage = FPSTR(control_script_events);
  controlPage.replace(F("_MIN_TEMP_"), String(convertCelsiusToLocalUnit(min_temp, useFahrenheit)));
  controlPage.replace(F("_MAX_TEMP_"), String(convertCelsiusToLocalUnit(max_temp, useFahrenheit)));
  controlPage.replace(F("_TEMP_STEP_"), String(temp_step));
  controlPage.replace(F("_HEAT_MODE_SUPPORT_"), (String)supportHeatMode);
  controlPage.replace(F("_QUIET_MODE_SUPPORT_"), (String)supportQuietMode);

  String htmlControlPage = FPSTR(html_page_control);
  // localize
  htmlControlPage.replace(F("_TXT_CTRL_CTEMP_"), translatedWord(FL_(txt_ctrl_ctemp)));
  htmlControlPage.replace(F("_TXT_CTRL_TEMP_"), translatedWord(FL_(txt_ctrl_temp)));
  htmlControlPage.replace(F("_TXT_CTRL_TITLE_"), translatedWord(FL_(txt_ctrl_title)));
  htmlControlPage.replace(F("_TXT_CTRL_POWER_"), translatedWord(FL_(txt_ctrl_power)));

  htmlControlPage.replace(F("_TXT_F_ON_"), translatedWord(FL_(txt_f_on)));
  htmlControlPage.replace(F("_TXT_F_OFF_"), translatedWord(FL_(txt_f_off)));
  // set data
  htmlControlPage.replace(F("_ROOMTEMP_"), String(convertCelsiusToLocalUnit(hp.getRoomTemperature(), useFahrenheit)));
  htmlControlPage.replace(F("_TEMP_SCALE_"), getTemperatureScale());
  htmlControlPage.replace(F("_TEMP_"), String(convertCelsiusToLocalUnit(hp.getTemperature(), useFahrenheit)));

  if (!(String(settings.power).isEmpty())) // null may crash with multitask
  {
    htmlControlPage.replace(F("_POWER_"), strcmp(settings.power, "ON") == 0 ? F("checked") : F(""));
  }

  controlPage += htmlControlPage;
  htmlControlPage = "";

  controlPage += getModeSelect(String(settings.mode));
  controlPage += getFanSelect(String(settings.fan));
  controlPage += getVaneSelect(String(settings.vane));
  controlPage += getWideVaneSelect(String(settings.wideVane));
  
  htmlControlPage = FPSTR(html_page_control_footer); 
  htmlControlPage.replace(F("_TXT_BACK_"), translatedWord(FL_(txt_back)));
  controlPage += htmlControlPage;
  htmlControlPage = "";

  sendWrappedHTML(request, controlPage);
  // We need to send the page content in chunks to overcome
  // a limitation on the maximum size we can send at one
  // time (approx 6k).
}

#ifdef METRICS
static void handleMetrics(AsyncWebServerRequest *request)
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

  metrics.replace(F("_UNIT_NAME_"), hostname);
  metrics.replace(F("_VERSION_"), m2mqtt_version);
  metrics.replace(F("_POWER_"), hppower);
  metrics.replace(F("_ROOMTEMP_"), (String)currentStatus.roomTemperature);
  metrics.replace(F("_TEMP_"), (String)currentSettings.temperature);
  metrics.replace(F("_FAN_"), hpfan);
  metrics.replace(F("_VANE_"), hpvane);
  metrics.replace(F("_WIDEVANE_"), hpwidevane);
  metrics.replace(F("_MODE_"), hpmode);
  metrics.replace(F("_OPER_"), (String)currentStatus.operating);
  metrics.replace(F("_COMPFREQ_"), (String)currentStatus.compressorFrequency);
  sendWrappedHTML(request, metrics);
}
#endif

// login page, also called for logout
static void handleLogin(AsyncWebServerRequest *request)
{
  bool loginSuccess = false;
  String msg;
  String loginPage = FPSTR(html_page_login);
  // localize
  loginPage.replace(F("_TXT_LOGIN_TITLE_"), translatedWord(FL_(txt_login_title)));
  loginPage.replace(F("_TXT_LOGIN_PH_USER_"), translatedWord(FL_(txt_login_ph_user)));
  loginPage.replace(F("_TXT_LOGIN_PH_PWD_"), translatedWord(FL_(txt_login_ph_pwd)));
  loginPage.replace(F("_TXT_LOGIN_USERNAME_"), translatedWord(FL_(txt_login_username)));
  loginPage.replace(F("_TXT_LOGIN_PASSWORD_"), translatedWord(FL_(txt_login_password)));
  loginPage.replace(F("_TXT_LOGIN_OPEN_STATUS_"), translatedWord(FL_(txt_login_open_status)));
  loginPage.replace(F("_TXT_LOGIN_"), translatedWord(FL_(txt_login)));
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

static void handleUpgrade(AsyncWebServerRequest *request)
{
  if (!checkLogin(request)) {
      return;
  }
  uploaderror = 0;
  String upgradePage = FPSTR(html_page_upgrade);
  // localize
  upgradePage.replace(F("_TXT_FW_UPDATE_PAGE_"), translatedWord(FL_(txt_fw_update_page)));
  upgradePage.replace(F("_TXT_B_UPGRADE_"), translatedWord(FL_(txt_upgrade)));
  upgradePage.replace(F("_TXT_BACK_"), translatedWord(FL_(txt_back)));
  upgradePage.replace(F("_TXT_UPGRADE_TITLE_"), translatedWord(FL_(txt_upgrade_title)));
  upgradePage.replace(F("_TXT_UPGRADE_INFO_"), translatedWord(FL_(txt_upgrade_info)));
  upgradePage.replace(F("_TXT_UPGRADE_START_"), translatedWord(FL_(txt_upgrade_start)));

  sendWrappedHTML(request, upgradePage);
}

static void handleUploadDone(AsyncWebServerRequest *request)
{
  ESP_LOGD(TAG, "HTTP: Firmware upload done");
  bool restartflag = false;
  String uploadDonePage = FPSTR(html_page_upload);
  // localize
  uploadDonePage.replace(F("_TXT_UPLOAD_FW_PAGE_"), translatedWord(FL_(txt_upload_fw_page)));
  uploadDonePage.replace(F("_TXT_BACK_"), translatedWord(FL_(txt_back)));

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
  uploadDonePage.replace(F("_UPLOAD_MSG_"), content);
  uploadDonePage.replace(F("_TXT_BACK_"), translatedWord(FL_(txt_back)));
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

// Handler webserver response
#ifdef WEBSOCKET_ENABLE
static void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    ESP_LOGD(TAG, "ws[%s][%" PRIu32 "] connect\n", server->url(), client->id());
    client->printf("Hello Client %" PRIu32 " :)", client->id());
    client->ping();
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    ESP_LOGD(TAG, "ws[%s][%" PRIu32 "] disconnect\n", server->url(), client->id());
  }
  else if (type == WS_EVT_ERROR)
  {
    ESP_LOGD(TAG, "ws[%s][%" PRIu32 "] error(%u): %s\n", server->url(), client->id(), *((uint16_t *)arg), (char *)data);
  }
  else if (type == WS_EVT_PONG)
  {
    ESP_LOGD(TAG, "ws[%s][%" PRIu32 "] pong[%zu]: %s\n", server->url(), client->id(), len, (len) ? (char *)data : "");
  }
  else if (type == WS_EVT_DATA)
  {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    String msg = "";
    if (info->final && info->index == 0 && info->len == len)
    {
      // the whole message is in a single frame and we got all of it's data
      ESP_LOGD(TAG, "ws[%s][%" PRIu32 "] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT) ? "text" : "binary", info->len);

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
          ESP_LOGD(TAG, "ws[%s][%" PRIu32 "] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
        ESP_LOGD(TAG, "ws[%s][%" PRIu32 "] frame[%lu] start[%llu]\n", server->url(), client->id(), info->num, info->len);
      }

      ESP_LOGD(TAG, "ws[%s][%" PRIu32 "] frame[%lu] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT) ? "text" : "binary", info->index, info->index + len);

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
        ESP_LOGD(TAG, "ws[%s][%" PRIu32 "] frame[%lu] end[%llu]\n", server->url(), client->id(), info->num, info->len);
        if (info->final)
        {
          ESP_LOGD(TAG, "ws[%s][%" PRIu32 "] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
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
  server.on("/others", handleOthers);
  server.on("/unit", handleUnit);
  server.on("/control", handleControl);
  server.on("/status", handleStatus);
  if (!isSecureEnable())
  {
    server.on("/upgrade", handleUpgrade);
    server.on("/upload", ANY_HTTP_METHOD, handleUploadDone, handleUploadLoop);
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


void startStationWebServer()
{
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
    server.on("/upload", ANY_HTTP_METHOD, handleUploadDone, handleUploadLoop);
#ifdef ESP32
    Update.onProgress(otaUpdateProgress);
#endif
  }
#ifdef WEBSOCKET_ENABLE
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
#endif
  events.onConnect([](AsyncEventSourceClient *client) { client->send("hello!", NULL, millis(), 1000); });
  server.addHandler(&events);
  server.begin();
}


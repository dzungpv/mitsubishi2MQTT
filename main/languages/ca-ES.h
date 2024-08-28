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
namespace ca
{
  // Breadcum
  const char txt_setup_page[] PROGMEM = "Pàgina de configuració";
  const char txt_upload_fw_page[] PROGMEM = "Càrrega del microprogramari";
  const char txt_fw_update_page[] PROGMEM = "Actualització del microprogramari";
  const char txt_check_fw_page[] PROGMEM = "Comproveu el nou microprogramari";
  const char txt_home_page[] PROGMEM = "Pàgina d'inici";

  // Main Menu
  const char txt_control[] PROGMEM = "Control";
  const char txt_setup[] PROGMEM = "Configuració";
  const char txt_status[] PROGMEM = "Estat";
  const char txt_firmware_upgrade[] PROGMEM = "Actualització de microprogramari";
  const char txt_reboot[] PROGMEM = "Reinicia";

  // Setup Menu
  const char txt_mqtt[] PROGMEM = "MQTT";
  const char txt_wifi[] PROGMEM = "WIFI";
  const char txt_unit[] PROGMEM = "Unitat";
  const char txt_others[] PROGMEM = "Altres";
  const char txt_reset[] PROGMEM = "Restableix la configuració";
  const char txt_reset_confirm[] PROGMEM = "Voleu restablir el controlador?";

  // Buttons
  const char txt_back[] PROGMEM = "Enrere";
  const char txt_save[] PROGMEM = "Desa i reinicia";
  const char txt_logout[] PROGMEM = "Surt";
  const char txt_upgrade[] PROGMEM = "Inicia l'actualització";
  const char txt_login[] PROGMEM = "Inici de sessió";

  // Form choices
  const char txt_f_on[] PROGMEM = "ON";
  const char txt_f_off[] PROGMEM = "OFF";
  const char txt_f_auto[] PROGMEM = "AUTO";
  const char txt_f_heat[] PROGMEM = "CALEFACCIÓ";
  const char txt_f_dry[] PROGMEM = "ASSECAR";
  const char txt_f_cool[] PROGMEM = "REFRIGERACIÓ";
  const char txt_f_fan[] PROGMEM = "VENTILADOR";
  const char txt_f_quiet[] PROGMEM = "SILENCIÓS";
  const char txt_f_speed[] PROGMEM = "VELOCITAT";
  const char txt_f_swing[] PROGMEM = "OSCIL·LACIÓ";
  const char txt_f_pos[] PROGMEM = "POSICIÓ";
  const char txt_f_celsius[] PROGMEM = "Celsius";
  const char txt_f_fh[] PROGMEM = "Fahrenheit";
  const char txt_f_allmodes[] PROGMEM = "Tots els modes";
  const char txt_f_noheat[] PROGMEM = "Tots els modes excepte calefacció";
  const char txt_f_noquiet[] PROGMEM = "No suporta mode silenciós";
  const char txt_f_low[] PROGMEM = "BAIX";
  const char txt_f_medium[] PROGMEM = "MITJÀ";
  const char txt_f_middle[] PROGMEM = "MITJÀ-ALT";
  const char txt_f_high[] PROGMEM = "ALT";

  // Page Reboot, save & Resseting
  const char txt_m_reboot[] PROGMEM = "S'està reiniciant... Refrescant en";
  const char txt_m_reset[] PROGMEM = "S'està restablint... Connectant a l'SSID";
  const char txt_m_reset_1[] PROGMEM = "Podeu tornar a connectar-vos a l'SSID";
  const char txt_m_save[] PROGMEM = "S'està desant la configuració i s'està reiniciant... Refrescant en";

  // Page MQTT
  const char txt_mqtt_title[] PROGMEM = "Configuració MQTT";
  const char txt_mqtt_fn[] PROGMEM = "Nom descriptiu";
  const char txt_mqtt_fn_desc[] PROGMEM = "(sense espais ni caràcters especials)";
  const char txt_mqtt_host[] PROGMEM = "Anfitrió o IP";
  const char txt_mqtt_port[] PROGMEM = "Port";
  const char txt_mqtt_port_desc[] PROGMEM = "(1883 per defecte, ESP32: 8883 CERT requereix)";
  const char txt_mqtt_user[] PROGMEM = "Usuari";
  const char txt_mqtt_password[] PROGMEM = "Contrasenya";
  const char txt_mqtt_topic[] PROGMEM = "Tema";
  const char txt_mqtt_ph_topic[] PROGMEM = "Introduïu el tema Mqtt";
  const char txt_mqtt_ph_user[] PROGMEM = "Introduïu l&#39;usuari Mqtt";
  const char txt_mqtt_ph_pwd[] PROGMEM = "Introduïu la contrasenya Mqtt";
  const char txt_mqtt_root_ca_cert[] PROGMEM = "CA-Root-Certificate (default Letsencrypt)";

  // Page Others
  const char txt_others_title[] PROGMEM = "Altra configuració";
  const char txt_others_haauto[] PROGMEM = "HA autodescobriment";
  const char txt_others_hatopic[] PROGMEM = "HA tema d'autodescobriment";
  const char txt_others_debug_packets[] PROGMEM = "MQTT topic debug packets";
  const char txt_others_debug_log[] PROGMEM = "MQTT topic debug logs";
  const char txt_others_tx_pin[] PROGMEM = "TX pin (ESP32, 0 use UART1)";
  const char txt_others_rx_pin[] PROGMEM = "RX pin (ESP32, 0 use UART1)";
  const char txt_others_tz[] PROGMEM = "Fus horari";
  const char txt_others_tz_list[] PROGMEM = "Vegeu-ne la llista";
  const char txt_others_ntp_server[] PROGMEM = "Server NTP";

  // Page Status
  const char txt_status_title[] PROGMEM = "Estat";
  const char txt_status_hvac[] PROGMEM = "Estat HVAC";
  const char txt_retries_hvac[] PROGMEM = "Nombre d'intents de connexió HVAC";
  const char txt_status_mqtt[] PROGMEM = "Estat MQTT";
  const char txt_status_wifi_ip[] PROGMEM = "IP WIFI";
  const char txt_failed_get_wifi_ip[] PROGMEM = "No s'ha pogut obtenir l'adreça IP";
  const char txt_status_wifi[] PROGMEM = "RSSI WIFI";
  const char txt_build_version[] PROGMEM = "Versió de compilació";
  const char txt_build_date[] PROGMEM = "Data de compilació";
  const char txt_status_freeheap[] PROGMEM = "Espai lliure";
  const char txt_current_time[] PROGMEM = "Hora actual";
  const char txt_boot_time[] PROGMEM = "Temps d'arrencada";
  const char txt_status_connect[] PROGMEM = "CONNECTAT";
  const char txt_status_disconnect[] PROGMEM = "DESCONNECTAT";

  // Page WIFI
  const char txt_wifi_title[] PROGMEM = "Configuració WIFI";
  const char txt_wifi_hostname[] PROGMEM = "Nom d'amfitrió";
  const char txt_wifi_hostname_desc[] PROGMEM = "(sense espais ni caràcters especials)";
  const char txt_wifi_ssid[] PROGMEM = "Nom de la xarxa Wifi";
  const char txt_wifi_ssid_enter[] PROGMEM = "(Introdueix un nom)";
  const char txt_wifi_ssid_select[] PROGMEM = "o trieu una xarxa:";
  const char txt_wifi_psk[] PROGMEM = "Contrasenya Wifi";
  const char txt_wifi_otap[] PROGMEM = "Contrasenya OTA";

  // Page Control
  const char txt_ctrl_title[] PROGMEM = "Control climatització";
  const char txt_ctrl_temp[] PROGMEM = "Temperatura";
  const char txt_ctrl_power[] PROGMEM = "Engega";
  const char txt_ctrl_mode[] PROGMEM = "Mode";
  const char txt_ctrl_fan[] PROGMEM = "Ventilador";
  const char txt_ctrl_vane[] PROGMEM = "Difusor vertical";
  const char txt_ctrl_wvane[] PROGMEM = "Difusor horitzontal";
  const char txt_ctrl_ctemp[] PROGMEM = "Temperatura actual";

  // Page Unit
  const char txt_unit_title[] PROGMEM = "Configuració climatització";
  const char txt_unit_language[] PROGMEM = "Idioma";
  const char txt_unit_temp[] PROGMEM = "Unitat de temperatura";
  const char txt_unit_maxtemp[] PROGMEM = "Temperatura màxima";
  const char txt_unit_mintemp[] PROGMEM = "Temperatura mínima";
  const char txt_unit_steptemp[] PROGMEM = "Pas de temperatura";
  const char txt_unit_modes[] PROGMEM = "Modes suportats";
  const char txt_unit_fan_modes[] PROGMEM = "Modes de ventilació suportats";
  const char txt_unit_password[] PROGMEM = "Contrasenya d'inici de sessió";
  const char txt_unit_password_confirm[] PROGMEM = "Confirmeu la contrasenya d'inici de sessió";
  const char txt_unit_password_not_match[] PROGMEM = "La confirmació de contrasenya no coincideix";
  const char txt_unit_login_username[] PROGMEM = "Nota: l'usuari d'inici de sessió predeterminat és";

  // Page Login
  const char txt_login_title[] PROGMEM = "Autenticació";
  const char txt_login_username[] PROGMEM = "Nom d'usuari";
  const char txt_login_password[] PROGMEM = "Contrasenya";
  const char txt_login_sucess[] PROGMEM = "Inici de sessió correcte, se us redirigirà d'aquí a uns segons";
  const char txt_login_fail[] PROGMEM = "Nom d'usuari/contrasenya incorrectes! Torna-ho a provar.";
  const char txt_login_open_status[] PROGMEM = "Obre la pàgina d'estat";
  const char txt_login_ph_user[] PROGMEM = "Introduïu el nom d&#39;usuari";
  const char txt_login_ph_pwd[] PROGMEM = "Introduïu la contrasenya";

  // Page Upgrade
  const char txt_upgrade_title[] PROGMEM = "Actualització";
  const char txt_upgrade_info[] PROGMEM = "Actualització de microprogramari per càrrega de fitxers";
  const char txt_upgrade_start[] PROGMEM = "S'ha iniciat la pujada";

  // Page Upload
  const char txt_upload[] PROGMEM = "Carrega";
  const char txt_upload_nofile[] PROGMEM = "No s'ha seleccionat cap fitxer";
  const char txt_upload_filetoolarge[] PROGMEM = "La mida del fitxer és més gran que l'espai lliure disponible";
  const char txt_upload_fileheader[] PROGMEM = "La capçalera màgica del fitxer no comença amb 0xE9";
  const char txt_upload_flashsize[] PROGMEM = "La mida del flaix del fitxer és més gran que la mida del flaix del dispositiu";
  const char txt_upload_buffer[] PROGMEM = "La memòria intermèdia de càrrega de fitxers no coincideix";
  const char txt_upload_failed[] PROGMEM = "Ha fallat la pujada. Activa l'opció de registre 3 per a més informació";
  const char txt_upload_aborted[] PROGMEM = "S'ha interromput la pujada";
  const char txt_upload_code[] PROGMEM = "Codi d'error de pujada ";
  const char txt_upload_error[] PROGMEM = "Codi d'error de pujada (vegeu Updater.cpp) ";
  const char txt_upload_success[] PROGMEM = "Correcta";
  const char txt_upload_refresh[] PROGMEM = "Refrescant en";

  // Page Init
  const char txt_init_title[] PROGMEM = "Configuració inicial";
  const char txt_init_reboot_mes[] PROGMEM = "S'està reiniciant i connectant a la xarxa WiFi! Hauries de veure'l llistat al teu punt d'accés.";
  const char txt_init_reboot_mes_1[] PROGMEM = "Utilitzeu aquesta adreça per accedir al vostre dispositiu després de reiniciar-lo. S'està reiniciant";
  const char txt_init_reboot[] PROGMEM = "Reiniciant...";

  // Page Init Firmware Check
  const char txt_init_fw_check_mes[] PROGMEM = "S'està comprovant si hi ha actualització de microprogramari disponible";
  const char txt_init_fw_check_mes_1[] PROGMEM = "Si us plau, mantingueu aquesta pàgina oberta durant 3 minuts. Redirigint en";
  const char txt_b_check_new_fw[] PROGMEM = "Comproveu el nou microprogramari";
  const char txt_fw_check_mes[] PROGMEM = "Si us plau, espereu 30 segons per enviar una nova sol·licitud.";

  // OTA update
  const char txt_ota_latest[] PROGMEM = "La versió actual és l'última.";
  const char txt_ota_invalid[] PROGMEM = "El microprogramari no és vàlid per a aquest dispositiu.";
  const char txt_ota_available[] PROGMEM = "Nou microprogramari disponible. S'està baixant...";
  const char txt_ota_close_n_wait[] PROGMEM = "Podeu tancar aquesta pàgina, l'actualització pot trigar 5 minuts.";
  const char txt_ota_checking[] PROGMEM = "S'està comprovant si hi ha actualització de microprogramari disponible, espereu...";
  const char txt_ota_check_failed[] PROGMEM = "La comprovació OTA ha fallat.";
  const char txt_ota_check_failed_code[] PROGMEM = "La comprovació OTA ha fallat, codi:";
  const char txt_ota_no_data[] PROGMEM = "No s'ha rebut cap dada.";
  const char txt_ota_success[] PROGMEM = "Actualització OTA correcta. S'està reiniciant";
  const char txt_ota_file_corrupt[] PROGMEM = "El fitxer binari està malmès.";
}

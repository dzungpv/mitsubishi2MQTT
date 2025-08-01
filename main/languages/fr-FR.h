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
namespace fr
{
  // Breadcum
  const char txt_setup_page[] PROGMEM = "Page de configuration";
  const char txt_upload_fw_page[] PROGMEM = "Télécharger le firmware";
  const char txt_fw_update_page[] PROGMEM = "Mise à niveau du micrologiciel";
  const char txt_check_fw_page[] PROGMEM = "Vérifier le nouveau firmware";
  const char txt_home_page[] PROGMEM = "Page d'accueil";

  // Main Menu
  const char txt_control[] PROGMEM = "Contrôle";
  const char txt_setup[] PROGMEM = "Configuration";
  const char txt_status[] PROGMEM = "Etats";
  const char txt_firmware_upgrade[] PROGMEM = "Mise à jour";
  const char txt_reboot[] PROGMEM = "Redémarrage";

  // Setup Menu
  const char txt_mqtt[] PROGMEM = "MQTT";
  const char txt_wifi[] PROGMEM = "WIFI";
  const char txt_unit[] PROGMEM = "Unité";
  const char txt_others[] PROGMEM = "Autres";
  const char txt_reset[] PROGMEM = "Remise à zéro";
  const char txt_reset_confirm[] PROGMEM = "Voulez-vous vraiment remettre à zéro?";

  // Buttons
  const char txt_back[] PROGMEM = "Retour";
  const char txt_save[] PROGMEM = "Sauvegarde & Redémarrage";
  const char txt_logout[] PROGMEM = "Déconnexion";
  const char txt_upgrade[] PROGMEM = "Lancer la mise à jour";
  const char txt_login[] PROGMEM = "Entrer";

  // Form choices
  const char txt_f_on[] PROGMEM = "Activé";
  const char txt_f_off[] PROGMEM = "Désactivé";
  const char txt_f_auto[] PROGMEM = "AUTO";
  const char txt_f_heat[] PROGMEM = "CHAUD";
  const char txt_f_dry[] PROGMEM = "SEC";
  const char txt_f_cool[] PROGMEM = "FROID";
  const char txt_f_fan[] PROGMEM = "VENTILLATION";
  const char txt_f_quiet[] PROGMEM = "SILENCIEUX";
  const char txt_f_speed[] PROGMEM = "VITESSE";
  const char txt_f_swing[] PROGMEM = "MOUVEMENT";
  const char txt_f_pos[] PROGMEM = "POSITION";
  const char txt_f_celsius[] PROGMEM = "Celsus";
  const char txt_f_fh[] PROGMEM = "Fahrenheit";
  const char txt_f_allmodes[] PROGMEM = "Clim réversible";
  const char txt_f_noheat[] PROGMEM = "Uniquement climatiseur";
  const char txt_f_noquiet[] PROGMEM = "Not support quiet mode";
  const char txt_f_low[] PROGMEM = "LOW";
  const char txt_f_medium[] PROGMEM = "MEDIUM";
  const char txt_f_middle[] PROGMEM = "MIDDLE";
  const char txt_f_high[] PROGMEM = "HIGH";

  // Message Reboot, save & Resseting
  const char txt_m_reboot[] PROGMEM = "Redémarrage... Rafraichisement dans";
  const char txt_m_reset[] PROGMEM = "Remise à zéro... Connecter vous au SSID";
  const char txt_m_reset_1[] PROGMEM = "Vous pouvez vous reconnecter au SSID";
  const char txt_m_save[] PROGMEM = "Sauvegarde de la configuration et redémarrage... Rafraichisement dans";

  // Page MQTT
  const char txt_mqtt_title[] PROGMEM = "Paramétres MQTT";
  const char txt_mqtt_fn[] PROGMEM = "Nom affiché";
  const char txt_mqtt_host[] PROGMEM = "Adresse serveur";
  const char txt_mqtt_port[] PROGMEM = "Port (défaut 1883)";
  const char txt_mqtt_user[] PROGMEM = "Utilisateur";
  const char txt_mqtt_password[] PROGMEM = "Mot de passe";
  const char txt_mqtt_topic[] PROGMEM = "Topic";
  const char txt_mqtt_fn_desc[] PROGMEM = "(no space or special char)";
  const char txt_mqtt_port_desc[] PROGMEM = "(default 1883, ESP32: 8883 CERT require)";
  const char txt_mqtt_ph_topic[] PROGMEM = "Enter Mqtt topic";
  const char txt_mqtt_ph_user[] PROGMEM = "Enter Mqtt user";
  const char txt_mqtt_ph_pwd[] PROGMEM = "Enter Mqtt password";
  const char txt_mqtt_root_ca_cert[] PROGMEM = "CA-Root-Certificate (default Letsencrypt)";

  // Page Others
  const char txt_others_title[] PROGMEM = "Autres Paramétres";
  const char txt_others_haauto[] PROGMEM = "HA Découverte automatique";
  const char txt_others_hatopic[] PROGMEM = "HA Topic Découverte automatique";
  const char txt_others_debug_packets[] PROGMEM = "MQTT topic debug packets";
  const char txt_others_debug_log[] PROGMEM = "MQTT topic debug logs";
  const char txt_others_tx_pin[] PROGMEM = "TX pin (ESP32, 0 use UART1)";
  const char txt_others_rx_pin[] PROGMEM = "RX pin (ESP32, 0 use UART1)";
  const char txt_others_tz[] PROGMEM = "Fuseau horaire";
  const char txt_others_tz_list[] PROGMEM = "Voir la liste";
  const char txt_others_ntp_server[] PROGMEM = "Serveur NTP";
  const char txt_others_web_panel[] PROGMEM = "Web Panel";

  // Page Status
  const char txt_status_title[] PROGMEM = "Etats";
  const char txt_status_hvac[] PROGMEM = "Etat HVAC";
  const char txt_retries_hvac[] PROGMEM = "HVAC Connection Retries";
  const char txt_status_mqtt[] PROGMEM = "Etat MQTT";
  const char txt_status_wifi[] PROGMEM = "WIFI RSSI";
  const char txt_status_connect[] PROGMEM = "CONNECTE";
  const char txt_status_disconnect[] PROGMEM = "DECONNECTE";
  const char txt_status_wifi_ip[] PROGMEM = "WIFI IP";
  const char txt_failed_get_wifi_ip[] PROGMEM = "Failed to get IP address";
  const char txt_build_version[] PROGMEM = "Build Version";
  const char txt_build_date[] PROGMEM = "Build Date";
  const char txt_status_freeheap[] PROGMEM = "Free Heap";
  const char txt_current_time[] PROGMEM = "Current Time";
  const char txt_boot_time[] PROGMEM = "Boot Time";

  // Page WIFI
  const char txt_wifi_title[] PROGMEM = "Paramétres WIFI";
  const char txt_wifi_hostname[] PROGMEM = "Nom de machine";
  const char txt_wifi_ssid[] PROGMEM = "SSID";
  const char txt_wifi_psk[] PROGMEM = "Clef pré-partagé";
  const char txt_wifi_otap[] PROGMEM = "OTA mot de passe";
  const char txt_wifi_hostname_desc[] PROGMEM = "(no space or special char)";
  const char txt_wifi_ssid_enter[] PROGMEM = "(Enter a name)";
  const char txt_wifi_ssid_select[] PROGMEM = "or choose a network:";
  const char txt_wifi_static_ip[] PROGMEM = "Adresse IP statique";
  const char txt_wifi_static_gw[] PROGMEM = "Adresse de la passerelle";
  const char txt_wifi_static_mask[] PROGMEM = "Masque de réseau";
  const char txt_wifi_static_dns[] PROGMEM = "Adresse IP DNS";

  // Page Control
  const char txt_ctrl_title[] PROGMEM = "Contrôle climatiseur";
  const char txt_ctrl_temp[] PROGMEM = "Température";
  const char txt_ctrl_power[] PROGMEM = "Alimentation";
  const char txt_ctrl_mode[] PROGMEM = "Mode";
  const char txt_ctrl_fan[] PROGMEM = "Ventilateur";
  const char txt_ctrl_vane[] PROGMEM = "Ailettes";
  const char txt_ctrl_wvane[] PROGMEM = "Ailettes horizontale";
  const char txt_ctrl_ctemp[] PROGMEM = "Température en cours";

  // Page Unit
  const char txt_unit_title[] PROGMEM = "Climatiseur configuration";
  const char txt_unit_temp[] PROGMEM = "Unité";
  const char txt_unit_maxtemp[] PROGMEM = "Température maximun";
  const char txt_unit_mintemp[] PROGMEM = "Température minimun";
  const char txt_unit_steptemp[] PROGMEM = "Graduation température";
  const char txt_unit_modes[] PROGMEM = "Modes disponible";
  const char txt_unit_password[] PROGMEM = "Mot de passe web";
  const char txt_unit_language[] PROGMEM = "Language";
  const char txt_unit_fan_modes[] PROGMEM = "Fan mode support";
  const char txt_unit_password_confirm[] PROGMEM = "Confirm Login password";
  const char txt_unit_password_not_match[] PROGMEM = "Confirm Password did not match";
  const char txt_unit_login_username[] PROGMEM = "Note: Default login user is";

  // Page Login
  const char txt_login_title[] PROGMEM = "Authentification";
  const char txt_login_password[] PROGMEM = "Mot de passe";
  const char txt_login_sucess[] PROGMEM = "Authentifcation réussie, vous allez être redirigé dans quelques secondes.";
  const char txt_login_fail[] PROGMEM = "Mauvais mot de passe! Essayer à nouveau.";
  const char txt_login_username[] PROGMEM = "Username";
  const char txt_login_open_status[] PROGMEM = "Open status page";
  const char txt_login_ph_user[] PROGMEM = "Enter username";
  const char txt_login_ph_pwd[] PROGMEM = "Enter password";

  // Page Upgrade
  const char txt_upgrade_title[] PROGMEM = "Mise à jour";
  const char txt_upgrade_info[] PROGMEM = "Mise à jour par envoi d'un fichier bin";
  const char txt_upgrade_start[] PROGMEM = "Mise à jour lancée";

  // Page Upload
  const char txt_upload_nofile[] PROGMEM = "Pas de fichier sélectionné";
  const char txt_upload_filetoolarge[] PROGMEM = "Fichier plus gros que l'espace disponible";
  const char txt_upload_fileheader[] PROGMEM = "Entếte magique du fichier de commence pas par 0xE9";
  const char txt_upload_flashsize[] PROGMEM = "La taille de la flash du fichier est plus large que la flash de l'équipement";
  const char txt_upload_buffer[] PROGMEM = "Erreur de comparaison du tampon d'envoi du fichier";
  const char txt_upload_failed[] PROGMEM = "Erreur d'envoie. Activer les logs option 3 pour plus d'information";
  const char txt_upload_aborted[] PROGMEM = "Envoie annulé";
  const char txt_upload_code[] PROGMEM = "Code d'erreur d'envoi ";
  const char txt_upload_error[] PROGMEM = "Code d'erreur de mise à jour ( voir Updater.cpp) ";
  const char txt_upload_success[] PROGMEM = "Réussi.";
  const char txt_upload_refresh[] PROGMEM = "Rafaichissement dans ";
  const char txt_upload[] PROGMEM = "Upload";

  // Page Init
  const char txt_init_title[] PROGMEM = "Initialisation";
  const char txt_init_reboot_mes[] PROGMEM = "Connexion à votre réseau WiFi avec adresse locale";
  const char txt_init_reboot_mes_1[] PROGMEM = "Utilisation de cette adresse pour accéder à votre appareil après son redémarrage. Redémarrage dans";
  const char txt_init_reboot[] PROGMEM = "Redémarrage...";
}
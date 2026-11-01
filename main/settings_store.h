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

#include "app_state.h"

void mountConfigFilesystem();
bool loadWifi();
bool loadMqtt();
bool loadUnit();
bool loadOthers();
void saveMqtt(String mqttFn, const String &mqttHost, String mqttPort, const String &mqttUser, const String &mqttPwd, String mqttTopic, const String &mqttRootCaCert);
void saveUnit(String tempUnit, String supportMode, String supportFanMode, String loginPassword, String tempStep, String languageIndex);
void saveWifi(String apSsid, const String &apPwd, String hostName, const String &otaPwd, const String &local_ip, const String &gw_ip, const String &subnet_ip, const String &dns_ip);
void saveOthers(const String &haa, const String &haat, const String &debugPckts, const String &debugLogs, const String &webPanel, const String &txPin, const String &rxPin, const String &tz, const String &ntp);
void saveCurrentOthers();
void setDefaults();

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

// Indexes for Home Assistant entity name/tag tables. The published strings,
// not these numbers, are what Home Assistant stores.
constexpr byte ENT_ROOM_TEMPERATURE = 0;
constexpr byte ENT_CONNECTION_STATE = 1;
constexpr byte ENT_UP_TIME = 2;
constexpr byte ENT_FREE_HEAP = 3;
constexpr byte ENT_RSSI = 4;
constexpr byte ENT_BSSI = 5;
constexpr byte ENT_COMPR_FRQ = 6;
constexpr byte ENT_RESTART_BTN = 7;
constexpr byte ENT_WEB_PANEL = 8;
constexpr byte MAX_ENTITY_ID = ENT_WEB_PANEL;

const char *getEntityTag(byte tag_id);
const char *getEntityName(byte tag_id);
void sendDeviceInfo();
void sendHaConfig();

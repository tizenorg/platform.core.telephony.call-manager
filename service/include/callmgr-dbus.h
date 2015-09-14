/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact: Sung Joon Won <sungjoon.won@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __CALLMGR_DBUS_H__
#define __CALLMGR_DBUS_H__

#include "callmgr-core.h"

#define CALLMGR_DBUS_SERVICE         "org.tizen.callmgr"
#define CALLMGR_DBUS_PATH            "/org/tizen/callmgr"
//#define CALLMGR_DBUS_DEFAULT_PATH    "/org/tizen/callmgr/Default"

int _callmgr_dbus_init (callmgr_core_data_t *core_data);
int _callmgr_dbus_deinit(callmgr_core_data_t *core_data);
int _callmgr_dbus_send_call_event(callmgr_core_data_t *core_data, int call_event, unsigned int call_id, callmgr_sim_slot_type_e sim_slot, int end_cause);
int _callmgr_dbus_send_call_status(callmgr_core_data_t *core_data, int call_status, int call_type, const char *call_num);
int _callmgr_dbus_send_dial_status(callmgr_core_data_t *core_data, callmgr_dial_status_e dial_status);
int _callmgr_dbus_send_dtmf_indi(callmgr_core_data_t *core_data, callmgr_dtmf_indi_type_e indi_type, char *dtmf_number);
int _callmgr_dbus_send_audio_status(callmgr_core_data_t *core_data, callmgr_path_type_e audio_path);
int _callmgr_dbus_send_go_foreground_indi(callmgr_core_data_t *core_data);
int _callmgr_dbus_send_vr_status(callmgr_core_data_t *core_data, int vr_status, int extra_type);
int _callmgr_dbus_send_mute_status(callmgr_core_data_t *core_data, int mute_status);

#endif	//__CALLMGR_DBUS_H__


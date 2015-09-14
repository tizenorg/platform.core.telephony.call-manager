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

#ifndef __CALLMGR_RINGER_H__
#define __CALLMGR_RINGER_H__

#include <glib.h>

typedef enum {
	CM_RINGER_EFFECT_CONNECT_TONE_E,
	CM_RINGER_EFFECT_DISCONNECT_TONE_E,
} cm_ringer_effect_type_e;

typedef enum {
	CM_RINGER_SIGNAL_NONE_E,
	CM_RINGER_SIGNAL_USER_BUSY_TONE_E,
	CM_RINGER_SIGNAL_WRONG_NUMBER_TONE_E,
	CM_RINGER_SIGNAL_CALL_FAIL_TONE_E,
	CM_RINGER_SIGNAL_NW_CONGESTION_TONE_E,
} cm_ringer_signal_type_e;

typedef enum {
	CM_RINGER_STATUS_IDLE_E,
	CM_RINGER_STATUS_RINGING_E,
} cm_ringer_status_e;

typedef struct __ringer_data *callmgr_ringer_handle_h;
typedef void (*cm_ringer_effect_finished_cb) (void *user_data);
typedef void (*cm_ringer_signal_finished_cb) (void *user_data);

int _callmgr_ringer_init(callmgr_ringer_handle_h *ringer_handle);
int _callmgr_ringer_deinit(callmgr_ringer_handle_h ringer_handle);
int _callmgr_ringer_get_ringer_status(callmgr_ringer_handle_h ringer_handle, cm_ringer_status_e *status);
int _callmgr_ringer_start_alert(callmgr_ringer_handle_h ringer_handle, char *caller_ringtone_path , gboolean is_earjack_connected);
int _callmgr_ringer_start_alternate_tone(callmgr_ringer_handle_h ringer_handle);
int _callmgr_ringer_stop_alert(callmgr_ringer_handle_h ringer_handle);
int _callmgr_ringer_stop_alternate_tone(callmgr_ringer_handle_h ringer_handle);
int _callmgr_ringer_play_effect(callmgr_ringer_handle_h ringer_handle, cm_ringer_effect_type_e effect_type, cm_ringer_effect_finished_cb cb_fn, void *user_data);
int _callmgr_ringer_stop_effect(callmgr_ringer_handle_h ringer_handle);
int _callmgr_ringer_play_signal(callmgr_ringer_handle_h ringer_handle, cm_ringer_signal_type_e signal_type, cm_ringer_signal_finished_cb cb_fn, void *user_data);
int _callmgr_ringer_stop_signal(callmgr_ringer_handle_h ringer_handle);
int _callmgr_ringer_play_local_ringback_tone(callmgr_ringer_handle_h ringer_handle);
int _callmgr_ringer_stop_local_ringback_tone(callmgr_ringer_handle_h ringer_handle);

#endif	//__CALLMGR_RINGER_H__


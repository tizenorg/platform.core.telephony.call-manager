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

#ifndef __CALLMGR_VCONF_H__
#define __CALLMGR_VCONF_H__

#include <glib.h>

typedef enum {
	CALL_CSC_DEFAULT = 0,

	CALL_CSC_SKT,
	CALL_CSC_KT,
	CALL_CSC_VZW,
	CALL_CSC_ATT,
	CALL_CSC_USC,

	CALL_CSC_SPR,
	CALL_CSC_DCM,
	CALL_CSC_XAC,
	CALL_CSC_CHC,
	CALL_CSC_BTU,	/* UK*/
	CALL_CSC_TPA,	/* Guatemala*/

	CALL_CSC_XJP,	/* Docomo MVNO*/

	CALL_CSC_TMB,
} call_salescode_type;

int _callmgr_vconf_is_security_lock(gboolean *is_security_lock);
int _callmgr_vconf_is_pwlock(gboolean *is_pwlock);
int _callmgr_vconf_is_low_battery(gboolean *is_low_battery);
int _callmgr_vconf_is_sound_setting_enabled(gboolean *is_sound_enabled);
int _callmgr_vconf_is_vibration_setting_enabled(gboolean *is_vibration_enabled);
int _callmgr_vconf_is_vibrate_when_ringing_enabled(gboolean *vibrate_when_ring_flag);
int _callmgr_vconf_get_ringtone_default_path(char **ringtone_path);
int _callmgr_vconf_get_ringtone_path(char **ringtone_path);
int _callmgr_vconf_get_increasing_volume_setting(gboolean *is_increasing);
int _callmgr_vconf_is_data_lock(gboolean *is_datalock);
int _callmgr_vconf_get_salescode(int *sales_code);
int _callmgr_vconf_is_cradle_status(gboolean *is_connected);
int _callmgr_vconf_is_bike_mode_enabled(gboolean *is_bike_mode_enabled);
int _callmgr_vconf_get_answer_message_path(char **answer_msg_path);
int _callmgr_vconf_is_answer_mode_enabled(gboolean *is_answer_mode_enabled);
int _callmgr_vconf_get_auto_answer_time(int *auto_answer_time_in_sec);

int _callmgr_vconf_init_palm_touch_mute_enabled(void *callback, void *data);
int _callmgr_vconf_deinit_palm_touch_mute_enabled(void *callback);
int _callmgr_vconf_is_test_auto_answer_mode_enabled(gboolean *is_auto_answer);
int _callmgr_vconf_is_recording_reject_enabled(gboolean *is_rec_reject_enabled);
int _callmgr_vconf_is_ui_visible(gboolean *o_is_ui_visible);
int _callmgr_vconf_is_do_not_disturb(gboolean *do_not_disturb);

#endif

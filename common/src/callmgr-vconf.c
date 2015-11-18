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

#include <stdio.h>
#include <stdlib.h>
#include <vconf.h>

#include "callmgr-util.h"
#include "callmgr-log.h"
#include "callmgr-vconf.h"

int _callmgr_vconf_is_security_lock(gboolean *is_security_lock)
{
	dbg("_callmgr_vconf_is_security_lock()");
	int lock_state;
	int lock_type = SETTING_SCREEN_LOCK_TYPE_NONE;
	int ret = 0;

	*is_security_lock = FALSE;
	ret = vconf_get_int(VCONFKEY_IDLE_LOCK_STATE, &(lock_state));
	if (ret < 0) {
		err("vconf_get_int error");
	}

	ret = vconf_get_int(VCONFKEY_SETAPPL_SCREEN_LOCK_TYPE_INT, &lock_type);
	if (ret < 0) {
		err("vconf_get_int error");
	}

	if (lock_state == VCONFKEY_IDLE_LOCK) {
		if ((lock_type != SETTING_SCREEN_LOCK_TYPE_SWIPE) && (lock_type != SETTING_SCREEN_LOCK_TYPE_NONE)) {
			*is_security_lock = TRUE;
			warn("Security lock state[%d]", lock_type);
		}
	}

	return 0;
}

int _callmgr_vconf_is_pwlock(gboolean *is_pwlock)
{
	int pwlock_state = -1;
	dbg("_callmgr_vconf_is_pwlock()");

	if (vconf_get_int(VCONFKEY_PWLOCK_STATE, &pwlock_state) < 0) {
		err("vconf_get_int failed.");
		*is_pwlock = FALSE;
		return -1;
	}
	info("pwlock_state:[%d]", pwlock_state);
	if ((pwlock_state == VCONFKEY_PWLOCK_BOOTING_LOCK) || (pwlock_state == VCONFKEY_PWLOCK_RUNNING_LOCK)) {
		*is_pwlock = TRUE;
	} else {
		*is_pwlock = FALSE;
	}
	return 0;
}

int _callmgr_vconf_is_low_battery(gboolean *is_low_battery)
{
	dbg("_callmgr_vconf_is_low_battery()");
	int low_status = -1;
	int charger_status = -1;

	if (!vconf_get_int(VCONFKEY_SYSMAN_BATTERY_STATUS_LOW, &low_status)) {
		if (low_status <= VCONFKEY_SYSMAN_BAT_CRITICAL_LOW) {

			//check charger status
			if (vconf_get_int(VCONFKEY_SYSMAN_BATTERY_CHARGE_NOW, &charger_status)) {
				err("vconf_get_int failed");
				*is_low_battery = FALSE;
				return -1;
			}

			if (charger_status == 1) {	// charging
				*is_low_battery = FALSE;
			} else {
				*is_low_battery = TRUE;
			}
		}
	} else {
		warn("get setting failed %s", VCONFKEY_SYSMAN_BATTERY_STATUS_LOW);
		*is_low_battery = FALSE;
	}

	return -1;
}

int _callmgr_vconf_is_sound_setting_enabled(gboolean *is_sound_enabled)
{
	int sound_status = 0;
	int ret = FALSE;
	dbg("_callmgr_vconf_is_sound_setting_enabled()");

	ret = vconf_get_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, &sound_status);
	if (ret < 0) {
		err("vconf_get_bool() failed");
		*is_sound_enabled = FALSE;
		return -1;
	} else {
		if (sound_status == 0) {
			*is_sound_enabled = FALSE;
		} else {
			*is_sound_enabled = TRUE;
		}
	}
	return 0;
}

int _callmgr_vconf_is_vibration_setting_enabled(gboolean *is_vibration_enabled)
{
	int vibration_status = 0;
	int ret = FALSE;

	dbg("_callmgr_vconf_is_vibration_setting_enabled()");

	ret = vconf_get_bool(VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL, &vibration_status);
	if (ret < 0) {
		err("vconf_get_bool() failed");
		*is_vibration_enabled = FALSE;

		return -1;
	} else {
		if (vibration_status == 0) {
			*is_vibration_enabled = FALSE;
		} else {
			*is_vibration_enabled = TRUE;
		}
	}

	return 0;
}

int _callmgr_vconf_is_vibrate_when_ringing_enabled(gboolean *vibrate_when_ring_flag)
{
	int vibration_status = 0;
	int ret = FALSE;

	dbg("_callmgr_vconf_is_vibrate_when_ringing_enabled()");

	ret = vconf_get_bool(VCONFKEY_SETAPPL_VIBRATE_WHEN_RINGING_BOOL, &vibration_status);
	if (ret < 0) {
		err("vconf_get_bool() failed");
		*vibrate_when_ring_flag = FALSE;

		return -1;
	} else {
		if (vibration_status == 0) {
			*vibrate_when_ring_flag = FALSE;
		} else {
			*vibrate_when_ring_flag = TRUE;
		}
	}

	return 0;
}

int _callmgr_vconf_get_ringtone_default_path(char **ringtone_path)
{
	char *path = NULL;

	dbg("_callmgr_vconf_get_ringtone_default_path()");

	path = vconf_get_str(VCONFKEY_SETAPPL_CALL_RINGTONE_DEFAULT_PATH_STR);
	if ((path == NULL) || (strlen(path) <= 0)) {
		err("VCONFKEY_SETAPPL_CALL_RINGTONE_DEFAULT_PATH_STR is NULL");
		*ringtone_path = NULL;

		return -1;
	} else {
		*ringtone_path = g_strdup(path);

		return 0;
	}
}

int _callmgr_vconf_get_ringtone_path(char **ringtone_path)
{
	char *path = NULL;

	dbg("_callmgr_vconf_get_ringtone_path()");

	path = vconf_get_str(VCONFKEY_SETAPPL_CALL_RINGTONE_PATH_STR);
	if ((path == NULL) || (strlen(path) <= 0)) {
		*ringtone_path = NULL;

		return -1;
	} else {
		*ringtone_path = g_strdup(path);

		return 0;
	}
}

int _callmgr_vconf_get_increasing_volume_setting(gboolean *is_increasing)
{
	int increasing_flag = 0;
	int ret = FALSE;
	dbg("_callmgr_vconf_is_increasing_volume()");

	ret = vconf_get_bool(VCONFKEY_CISSAPPL_INCREASE_RINGTONE_BOOL, &increasing_flag);
	if (ret < 0) {
		err("vconf_get_bool() failed");
		*is_increasing = FALSE;

		return -1;
	} else {
		if (increasing_flag == 0) {
			*is_increasing = FALSE;
		} else {
			*is_increasing = TRUE;
		}

		return 0;
	}
}

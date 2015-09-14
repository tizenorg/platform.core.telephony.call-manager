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

#include "callmgr-log.h"
#include "callmgr-vconf.h"

int _callmgr_vconf_is_data_lock(gboolean *is_datalock)
{
	dbg("_callmgr_vconf_is_data_lock");
	char *data_lock = vconf_get_str(VCONFKEY_ODE_CRYPTO_STATE);
	if (data_lock) {
		if (g_strcmp0(data_lock, "encrypted") == 0) {
			dbg("Data lock state.");
			*is_datalock = TRUE;
		} else {
			*is_datalock = FALSE;
		}
		g_free(data_lock);
	} else {
		*is_datalock = FALSE;
		err("VCONFKEY_ODE_CRYPTO_STATE is NULL");
		return -1;
	}

	return 0;
}

int _callmgr_vconf_get_salescode(int *sales_code)
{
	char *salescode = NULL;
	salescode = vconf_get_str(VCONFKEY_CSC_SALESCODE);
	int _g_salescode = 0;

	if (!g_strcmp0(salescode, "SKC") || !g_strcmp0(salescode, "SKO")) {
		_g_salescode = CALL_CSC_SKT;
	} else if (!g_strcmp0(salescode, "KTC") || !g_strcmp0(salescode, "KTO")) {
		_g_salescode = CALL_CSC_KT;
	} else if (!g_strcmp0(salescode, "VZW")) {
		_g_salescode = CALL_CSC_VZW;
	} else if (!g_strcmp0(salescode, "SPR")) {
		_g_salescode = CALL_CSC_SPR;
	} else if (!g_strcmp0(salescode, "USC")) {
		_g_salescode = CALL_CSC_USC;
	} else if (!g_strcmp0(salescode, "ATT")) {
		_g_salescode = CALL_CSC_ATT;
	} else if (!g_strcmp0(salescode, "DCM")) {
		_g_salescode = CALL_CSC_DCM;
	} else if (!g_strcmp0(salescode, "CHC")) {
		_g_salescode = CALL_CSC_CHC;
	} else if (!g_strcmp0(salescode, "XAC")) {
		_g_salescode = CALL_CSC_XAC;
	} else if (!g_strcmp0(salescode, "TMB")) {
		_g_salescode = CALL_CSC_TMB;
	} else if (!g_strcmp0(salescode, "BTU")) {
		_g_salescode = CALL_CSC_BTU;
	} else if (!g_strcmp0(salescode, "XJP")) {
		_g_salescode = CALL_CSC_XJP;
	} else if (!g_strcmp0(salescode, "TPA")) {
		_g_salescode = CALL_CSC_TPA;
	} else {
		_g_salescode = CALL_CSC_DEFAULT;
	}
	warn("sales : %d", _g_salescode);
	g_free(salescode);

	*sales_code = _g_salescode;
	return 0;
}

int _callmgr_vconf_is_cradle_status(gboolean *is_connected)
{
	CM_RETURN_VAL_IF_FAIL(is_connected, -1);

	int cradle_state = 0;
	if (vconf_get_int(VCONFKEY_SYSMAN_CRADLE_STATUS, &cradle_state) < 0) {
		err("vconf_get_int failed");
		return -1;
	}

	*is_connected = cradle_state;
	return 0;
}

int _callmgr_vconf_is_bike_mode_enabled(gboolean *is_bike_mode_enabled)
{
	dbg("..");

	int bike_mode = -1;
	if (vconf_get_int(VCONFKEY_BIKEMODE_ENABLED, &bike_mode) < 0) {
		err("vconf_get_int failed..");
		*is_bike_mode_enabled = FALSE;
		return -1;
	}

	warn("bike_mode = [%d]", bike_mode);
	if (bike_mode == 1) {
		*is_bike_mode_enabled = TRUE;
	} else {
		*is_bike_mode_enabled = FALSE;
	}

	return 0;
}

int _callmgr_vconf_get_answer_message_path(char **answer_msg_path)
{
	dbg("_callmgr_vconf_get_answer_message_path()");
	char *path = NULL;

	path = vconf_get_str(VCONFKEY_BIKEMODE_ANSWER_MESSAGE);
	if ((path == NULL) || (strlen(path) <= 0)) {
		err("VCONFKEY_BIKEMODE_ANSWER_MESSAGE is NULL");
		*answer_msg_path = NULL;
		return -1;
	} else {
		warn("msg_path: [%s]", path);
		*answer_msg_path = g_strdup(path);
		return 0;
	}
}

int _callmgr_vconf_is_answer_mode_enabled(gboolean *is_answer_mode_enabled)
{
	dbg("..");

	int answer_mode = -1;
	if (vconf_get_int(VCONFKEY_CISSAPPL_ANSWERING_MODE_INT, &answer_mode) < 0) {
		err("vconf_get_int failed..");
		*is_answer_mode_enabled = FALSE;
		return -1;
	}

	dbg("answer_mode = [%d]", answer_mode);
	if (answer_mode == 1) {
		*is_answer_mode_enabled = TRUE;
	} else {
		*is_answer_mode_enabled = FALSE;
	}

	return 0;
}

int _callmgr_vconf_get_auto_answer_time(int *auto_answer_time_in_sec)
{
	dbg("..");

	int auto_answer_time = 0;
	if (vconf_get_int(VCONFKEY_CISSAPPL_ANSWERING_MODE_TIME_INT, &auto_answer_time) < 0) {
		err("vconf_get_int failed..");
		auto_answer_time_in_sec = 0;
		return -1;
	}

	dbg("auto_answer_time = [%d]", auto_answer_time);
	*auto_answer_time_in_sec = auto_answer_time;

	return 0;
}

int _callmgr_vconf_init_palm_touch_mute_enabled(void *callback, void *data)
{
	CM_RETURN_VAL_IF_FAIL(callback, -1);

	if (vconf_notify_key_changed(VCONFKEY_SHOT_TIZEN_PALM_TOUCH_MUTE_ENABLED, (vconf_callback_fn)callback, data) != VCONF_OK) {
		err("vconf_notify_key_changed(VCONFKEY_SHOT_TIZEN_PALM_TOUCH_MUTE_ENABLED) failed");
		return -1;
	}

	return 0;
}

int _callmgr_vconf_deinit_palm_touch_mute_enabled(void *callback)
{
	CM_RETURN_VAL_IF_FAIL(callback, -1);

	if (vconf_ignore_key_changed(VCONFKEY_SHOT_TIZEN_PALM_TOUCH_MUTE_ENABLED, (vconf_callback_fn)callback) != VCONF_OK) {
		err("vconf_ignore_key_changed(VCONFKEY_SHOT_TIZEN_PALM_TOUCH_MUTE_ENABLED) failed");
		return -1;
	}

	return 0;
}

int _callmgr_vconf_is_test_auto_answer_mode_enabled(gboolean *is_auto_answer)
{
	int auto_answer_status = 0;
	int ret = 0;

	ret = vconf_get_int(VCONFKEY_TESTMODE_AUTO_ANSWER, &auto_answer_status);
	if (ret < 0) {
		err("vconf_get_int() failed");
		*is_auto_answer = FALSE;

		return -1;
	} else {
		if (auto_answer_status == 0) {
			*is_auto_answer = FALSE;
		} else {
			*is_auto_answer = TRUE;
		}
	}

	return 0;

}

int _callmgr_vconf_is_recording_reject_enabled(gboolean *is_rec_reject_enabled)
{
	int rec_reject_status = 0;
	int ret = 0;

	ret = vconf_get_bool(VCONFKEY_CALL_RECORDING_REJECT, &rec_reject_status);
	if (ret < 0) {
		err("vconf_get_bool() failed");
		*is_rec_reject_enabled = FALSE;

		return -1;
	} else {
		if (rec_reject_status == 0) {
			*is_rec_reject_enabled = FALSE;
		} else {
			*is_rec_reject_enabled = TRUE;
		}
	}

	return 0;
}

int _callmgr_vconf_is_ui_visible(gboolean *o_is_ui_visible)
{
	int is_ui_visible = 0;
	int ret = 0;

	ret = vconf_get_int(VCONFKEY_CALL_UI_VISIBILITY, &is_ui_visible);
	if (ret < 0) {
		err("vconf_get_int() failed");
		*o_is_ui_visible = FALSE;

		return -1;
	} else {
		if (is_ui_visible == 0) {
			*o_is_ui_visible = FALSE;
		} else {
			warn("UI visible");
			*o_is_ui_visible = TRUE;
		}
	}

	return 0;

}

int _callmgr_vconf_is_do_not_disturb(gboolean *do_not_disturb)
{
	int is_do_not_disturb = 0;
	int ret = 0;

	ret = vconf_get_int(VCONFKEY_QUICKPANEL_SETTING_DO_NOT_DISTURB, &is_do_not_disturb);
	if (ret < 0) {
		err("vconf_get_int() failed");
		*do_not_disturb = FALSE;
		return -1;
	} else {
		if (is_do_not_disturb == 0) {
			*do_not_disturb = FALSE;
		} else {
			dbg("Do not disturb setting is on");
			*do_not_disturb = TRUE;
		}
	}

	return 0;
}

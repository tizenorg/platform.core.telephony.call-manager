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

#include <Elementary.h>
#include <Eina.h>
//#include <Ecore_X.h>
//#include <Ecore_X_Atoms.h>
//#include <utilX.h>
#include <stdio.h>
#include <app_control.h>
#include <app.h>
//#include <app_extension.h>
#include <vconf.h>
#include <device/display.h>

#include "callmgr-popup-common.h"
#include "callmgr-popup-widget.h"
#include "callmgr-popup-dbus-if.h"
#include "callmgr-popup-util.h"

static struct appdata g_ad;

void __callmgr_popup_turn_lcd_on(void);

static bool __callmgr_popup_app_create(void *data)
{
	CallMgrPopAppData_t *ad = (CallMgrPopAppData_t *)data;
	Evas_Object *eo = NULL;

	if (_callmgr_popup_dbus_init(data) < 0) {
		ERR("DBUS INIT FAILED");
		elm_exit();
	}
	/* ToDo: Need check replace */
	//eo = (Evas_Object*)app_get_preinitialized_window(PACKAGE);
	//if (NULL == eo) {
		eo = elm_win_add(NULL, PACKAGE, ELM_WIN_BASIC);
	//} else {
	//	DBG("Process pool");
	//}

	if (!eo) {
		return NULL;
	}

	elm_theme_extension_add(NULL, PATH_EDJ);

	/* Set base scale */
	elm_app_base_scale_set(2.6);

	ad->win_main = NULL;
	ad->popup = NULL;
	ad->dial_num = NULL;
	ad->popup_type = 0;
	ad->request = NULL;

	return TRUE;
}

static void __callmgr_popup_app_control_clone(app_control_h *clone, app_control_h app_control)
{
	if (*clone) {
		app_control_destroy(*clone);
	}
	app_control_clone(clone, app_control);

	return;
}

static void __callmgr_popup_app_service(app_control_h app_control, void *user_data)
{
	DBG("__callmgr_popup_app_service()..");
	int type = -1;
	CallMgrPopAppData_t *ad = (CallMgrPopAppData_t *)user_data;
	char *string = NULL;
	char *extra_data = NULL;
	int ret = 0;

	if (app_control) {
		if (ad->exit_idler) {
			ecore_idler_del(ad->exit_idler);
			ad->exit_idler = NULL;
		}

		ret = app_control_get_extra_data(app_control, "TYPE", &extra_data);
		if (ret != 0/*SERVICE_ERR_NONE*/) {
			DBG("app_control_get_extra_data failed");
		}

		if (extra_data) {
			DBG("TYPE: [%s]", extra_data);
			type = atoi(extra_data);
			free(extra_data);
			extra_data = NULL;
		} else {
			ERR("No type");
			elm_exit();
		}

		ret = app_control_get_extra_data(app_control, "NUMBER", &extra_data);
		if (ret != 0/*SERVICE_ERR_NONE*/) {
			DBG("app_control_get_extra_data failed");
		}

		if (extra_data) {
			DBG("NUMBER: [%s]", extra_data);
			if (ad->dial_num) {
				g_free(ad->dial_num);
				ad->dial_num = NULL;
			}
			ad->dial_num = g_strdup(extra_data);
			free(extra_data);
			extra_data = NULL;
		} else {
			ad->dial_num = NULL;
		}
	} else {
		WARN("No app control");
		elm_exit();
	}

	ad->popup_type = type;

	SEC_WARN("NUMBER(%s)", ad->dial_num);
	WARN("Popup type(%d)", ad->popup_type);

	// Grab HW key
	_callmgr_popup_system_grab_key(ad);

	switch (ad->popup_type) {
	case CALLMGR_POPUP_FLIGHT_MODE_E:
		ret = app_control_get_extra_data(app_control, "SUB_INFO", &extra_data);
		if (ret != 0) {
			WARN("No call type");
		}

		if (extra_data) {
			DBG("INFO: [%s]", extra_data);
			ad->call_type = atoi(extra_data);
			free(extra_data);
			extra_data = NULL;
		}

		ret = app_control_get_extra_data(app_control, "ACTIVE_SIM", &extra_data);
		if (ret != 0) {
			WARN("No active sim");
		}

		if (extra_data) {
			DBG("SIM: [%s]", extra_data);
			ad->active_sim = atoi(extra_data);
			free(extra_data);
			extra_data = NULL;
		}

		__callmgr_popup_app_control_clone(&ad->request, app_control);
		_callmgr_popup_create_flight_mode(ad);
		break;

	case CALLMGR_POPUP_FLIGHT_MODE_DISABLING_E:
		_callmgr_popup_create_popup_checking(ad, _("IDS_CALL_POP_DISABLING_FLIGHT_MODE_ING"));
		break;

	case CALLMGR_POPUP_SS_INFO_E:
		ret = app_control_get_extra_data(app_control, "SUB_INFO", &extra_data);
		if (ret != 0) {
			DBG("app_control_get_extra_data failed");
		}

		if (extra_data) {
			DBG("INFO: [%s]", extra_data);
			type = atoi(extra_data);
			free(extra_data);
			extra_data = NULL;
		} else {
			ERR("No info");
			elm_exit();
		}

		string = _callmgr_popup_get_ss_info_string(type);
		if (string) {
			_callmgr_popup_create_toast_msg(string);
			g_free(string);
			string = NULL;
		}
		break;

	case CALLMGR_POPUP_CALL_ERR_E:
		ret = app_control_get_extra_data(app_control, "SUB_INFO", &extra_data);
		if (ret != 0) {
			DBG("app_control_get_extra_data failed");
		}

		if (extra_data) {
			DBG("INFO: [%s]", extra_data);
			type = atoi(extra_data);
			free(extra_data);
			extra_data = NULL;
		} else {
			ERR("No info");
			elm_exit();
		}

		string = _callmgr_popup_get_call_err_string(type, ad->dial_num);
		if (string) {
			_callmgr_popup_create_toast_msg(string);
			g_free(string);
			string = NULL;
		}
		break;

	case CALLMGR_POPUP_HIDE_E:
		if (ad->request) {
			_callmgr_popup_reply_to_launch_request(ad, "RESULT", "0");	/* "0" means CANCEL */
		}
		elm_exit();
		break;

	case CALLMGR_POPUP_SIM_SELECT_E:
		ret = app_control_get_extra_data(app_control, "SUB_INFO", &extra_data);
		if (ret != 0) {
			WARN("No call type");
		}

		if (extra_data) {
			DBG("INFO: [%s]", extra_data);
			ad->call_type = atoi(extra_data);
			free(extra_data);
			extra_data = NULL;
		}

		__callmgr_popup_app_control_clone(&ad->request, app_control);
		_callmgr_popup_create_sim_selection(ad);
		break;
	case CALLMGR_POPUP_REC_STATUS_E: {
		callmgr_popup_rec_status_sub_info_e rec_status_sub_info = CALLMGR_POPUP_REC_STATUS_NONE_E;
		char *sub_info = NULL;

		ret = app_control_get_extra_data(app_control, "SUB_INFO", &sub_info);
		if (ret != 0 || !sub_info) {
			ERR("app_control_get_extra_data() failed[%d] or no extra_data");
			elm_exit();
		}
		DBG("INFO: [%s]", sub_info);
		rec_status_sub_info = atoi(sub_info);
		free(sub_info);

		switch(rec_status_sub_info) {
		case CALLMGR_POPUP_REC_STATUS_STOP_BY_NORMAL_E:
			_callmgr_popup_create_toast_msg(_("IDS_CALL_TPOP_RECORDING_SAVED_IN_VOICE_RECORDER"));
			break;
		case CALLMGR_POPUP_REC_STATUS_STOP_BY_MEMORY_FULL_E:
			_callmgr_popup_create_toast_msg(_("IDS_CALL_TPOP_RECORDING_SAVED_IN_VOICE_RECORDER_DEVICE_STORAGE_FULL"));
			break;
		case CALLMGR_POPUP_REC_STATUS_STOP_BY_TIME_SHORT_E:
			_callmgr_popup_create_toast_msg(_("IDS_VR_TPOP_UNABLE_TO_SAVE_RECORDING_RECORDING_TOO_SHORT"));
			break;
		case CALLMGR_POPUP_REC_STATUS_STOP_BY_NO_ENOUGH_MEMORY_E:
			_callmgr_popup_create_toast_msg(_("IDS_CALL_POP_UNABLE_TO_RECORD_NOT_ENOUGH_MEMORY"));
			break;
		default:
			ERR("unhandled sub info[%d]", rec_status_sub_info);
			break;
		}
	}	break;
	case CALLMGR_POPUP_TOAST_E:
		ret = app_control_get_extra_data(app_control, "SUB_INFO", &extra_data);
		callmgr_call_popup_toast_type_e toast_popup_type = CALLMGR_POPUP_TOAST_MAX;

		if (ret != 0) {
			WARN("No toast popup type");
		}

		if (extra_data) {
			DBG("INFO: [%s]", extra_data);
			toast_popup_type = atoi(extra_data);
			free(extra_data);
			extra_data = NULL;
		}

		switch (toast_popup_type) {
		case CALLMGR_POPUP_TOAST_SWAP_CALL:
			_callmgr_popup_create_toast_msg(_("IDS_CALL_TPOP_SWAPPING_CALLS_ING"));
			break;
		case CALLMGR_POPUP_TOAST_CUSTOM:
			// show ad->dial_num string as it is.
			_callmgr_popup_create_toast_msg(_(ad->dial_num));
			break;
		default:
			ERR("unhadled toast popup type(%d)", toast_popup_type);
			elm_exit();
			break;
		}
		break;

	case CALLMGR_POPUP_OUT_OF_3G_TRY_VOICE_CALL:
		ret = app_control_get_extra_data(app_control, "SUB_INFO", &extra_data);
		if (ret != 0) {
			WARN("No call type");
		}

		if (extra_data) {
			DBG("INFO: [%s]", extra_data);
			ad->call_type = atoi(extra_data);
			free(extra_data);
			extra_data = NULL;
		}

		ret = app_control_get_extra_data(app_control, "ACTIVE_SIM", &extra_data);
		if (ret != 0) {
			WARN("No active sim");
		}

		if (extra_data) {
			DBG("SIM: [%s]", extra_data);
			ad->active_sim = atoi(extra_data);
			free(extra_data);
			extra_data = NULL;
		}

		__callmgr_popup_app_control_clone(&ad->request, app_control);
		_callmgr_popup_create_try_voice_call(ad);
		break;

	default:
		ERR("unhandled popup type(%d)", ad->popup_type);
		elm_exit();
		break;
	}
}

static void __callmgr_popup_app_pause(void *data)
{
	/*CallMgrPopAppData_t *ad = (CallMgrPopAppData_t *)data;*/

	DBG("__callmgr_popup_app_pause()..");

	elm_exit();
}

static void __callmgr_popup_app_resume(void *data)
{
	DBG("__callmgr_popup_app_resume()..");
}

static void __callmgr_popup_app_terminate(void *data)
{
	CallMgrPopAppData_t *ad = (CallMgrPopAppData_t *)data;

	DBG("__callmgr_popup_app_terminate()..");

	_callmgr_popup_dbus_deinit(ad);
	_callmgr_popup_system_ungrab_key(ad);

	ad->win_main = NULL;
	ad->popup = NULL;
	ad->dial_num = NULL;
	ad->popup_type = 0;

	app_control_destroy(ad->request);
	ad->request = NULL;
}

CALLMRG_POPUP_MODULE_EXPORT int main(int argc, char *argv[])
{
	ui_app_lifecycle_callback_s event_callback = {0,};
	DBG("ENTER");
	memset(&g_ad, 0, sizeof(struct appdata));

	event_callback.create = __callmgr_popup_app_create;
	event_callback.terminate = __callmgr_popup_app_terminate;
	event_callback.pause = __callmgr_popup_app_pause;
	event_callback.resume = __callmgr_popup_app_resume;
	event_callback.app_control = __callmgr_popup_app_service;

	int ret = 0/*APP_ERR_NONE*/;

	ret = ui_app_main(argc, argv, &event_callback, &g_ad);

	if (ret != 0/*APP_ERR_NONE*/) {
		ERR("ui_app_main() is failed. err=%d", ret);
	}

	return ret;
}


void __callmgr_popup_turn_lcd_on(void)
{
	int result = -1;

	result = device_display_change_state(DISPLAY_STATE_NORMAL);
	if (result != 0) {
		ERR("error LCD on : %d", result);
	}
}

void __callmgr_popup_turn_lcd_off(void)
{
	int result = -1;

	result = device_display_change_state(DISPLAY_STATE_SCREEN_OFF);
	if (result != 0) {
		ERR("error LCD off : %d", result);
	}
}


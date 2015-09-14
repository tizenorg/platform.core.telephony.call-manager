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

#include "callmgr-core.h"
#include "callmgr-log.h"
#include "callmgr-dbus.h"
#include "callmgr-vconf.h"
#include "callmgr-util.h"

//Will enable when answering memo change enabled.
#if 0
typedef enum _call_vc_core_flags_t {
	CM_CORE_FLAG_NONE = 0x00000000, /**< NONE state */
	CM_CORE_FLAG_ANSWERING_MEMO = 0x00000001, /**< SET - Answer call with answering memo, UNSET - Normal answer call*/
} callmgr_core_flags_e;

//call core flag status
static unsigned int core_flags_status;
#endif

static void __callmgr_core_send_bt_events(callmgr_core_data_t *core_data, int idle_call_id);
static int __callmgr_core_process_dtmf_number(callmgr_core_data_t *core_data);
static int __callmgr_core_get_signal_type(cm_telephony_end_cause_type_e end_cause_type, callmgr_play_signal_type_e *signal_type);

//Will enable when answering memo change enabled.
#if 0
static void __callmgr_core_set_flag_status(callmgr_core_flags_e core_flags, gboolean bstatus);
static gboolean __callmgr_core_get_flag_status(callmgr_core_flags_e core_flags);

static void __callmgr_core_set_flag_status(callmgr_core_flags_e core_flags, gboolean bstatus)
{
	warn("Before Set, core_flags_status:[0x%x]", core_flags_status);

	if (CM_CORE_FLAG_NONE == core_flags) {
		/*Set the document flag to defaults */
		core_flags_status = CM_CORE_FLAG_NONE;
		return;
	}

	if (TRUE == bstatus) {
		/*Set Flag */
		core_flags_status |= core_flags;
	} else {
		/*Remove bit field only if it is already set/ otherwise ignore it */
		if ((core_flags_status & core_flags) == core_flags) {
			core_flags_status = (core_flags_status ^ core_flags);
		}
	}

	warn("After SET, core_flags_status:[0x%x]", core_flags_status);
}

static gboolean __callmgr_core_get_flag_status(callmgr_core_flags_e core_flags)
{
	warn("core_flags:[0x%x]", core_flags);

	if ((core_flags_status & core_flags) == core_flags) {
		warn("Flag [0x%x] is available", core_flags);
		return TRUE;
	}

	warn("Flag [0x%x] is not available", core_flags);
	return FALSE;
}
#endif

static int __callmgr_core_set_call_status(callmgr_core_data_t *core_data, callmgr_call_status_e call_status, callmgr_call_type_e call_type, char *call_number)
{
	CM_RETURN_VAL_IF_FAIL(core_data, -1);
	core_data->call_status = call_status;
	_callmgr_dbus_send_call_status(core_data, call_status, call_type, call_number);
	return 0;
}

static int __callmgr_core_convert_tel_call_type(cm_telephony_call_type_e tel_call_type, callmgr_call_type_e *cm_call_type) {
	dbg(">>");
	if((tel_call_type == CM_TEL_CALL_TYPE_CS_VOICE) || (tel_call_type == CM_TEL_CALL_TYPE_PS_VOICE) || (tel_call_type == CM_TEL_CALL_TYPE_E911)) {
		*cm_call_type = CALL_TYPE_VOICE_E;
	}
	else if ((tel_call_type == CM_TEL_CALL_TYPE_CS_VIDEO) || (tel_call_type == CM_TEL_CALL_TYPE_PS_VIDEO)) {
		*cm_call_type = CALL_TYPE_VIDEO_E;
	}
	else {
		*cm_call_type = CALL_TYPE_INVALID_E;
	}
	return 0;
	/* To do: not handled CM_TEL_CALL_TYPE_VSHARE_RX nad CM_TEL_CALL_TYPE_VSHARE_TX */
}

static int __callmgr_core_set_telephony_audio_route(callmgr_core_data_t *core_data, callmgr_audio_device_e active_device)
{
	cm_telephony_audio_path_type_e tel_path;
	gboolean is_extra_vol = FALSE;
	gboolean is_nrec = FALSE;
	CM_RETURN_VAL_IF_FAIL(core_data, -1);
	dbg(">>");
	int volume = 0;

	/* Set Telephony route */
	switch (active_device) {
		case CALLMGR_AUDIO_DEVICE_BT_E:
			/* ToDo: get BT NREC status */
			_callmgr_bt_is_nrec_enabled(core_data->bt_handle, &is_nrec);
			if (is_nrec) {
				tel_path = CM_TAPI_SOUND_PATH_BT_NSEC_OFF;
			}
			else {
				tel_path = CM_TAPI_SOUND_PATH_BLUETOOTH;
			}
			break;
		case CALLMGR_AUDIO_DEVICE_SPEAKER_E:
			tel_path = CM_TAPI_SOUND_PATH_SPK_PHONE;
			break;
		case CALLMGR_AUDIO_DEVICE_EARJACK_E:
			tel_path = CM_TAPI_SOUND_PATH_HEADSET;
			break;
		case CALLMGR_AUDIO_DEVICE_RECEIVER_E:
		default:
			tel_path = CM_TAPI_SOUND_PATH_HANDSET;
			break;
	}

	_callmgr_audio_get_extra_vol(core_data->audio_handle, &is_extra_vol);
	if (_callmgr_telephony_set_audio_path(core_data->telephony_handle, tel_path, is_extra_vol) < 0) {
		return -1;
	}

	if (active_device != CALLMGR_AUDIO_DEVICE_BT_E) {
		/* If previous route is same with voice session, volume changed callback does not occur */
		/* We need to set volume for modem */
		/* BT volume was not controlled on the device */
		if (_callmgr_audio_get_current_volume(core_data->audio_handle, &volume) == 0) {
			dbg("volume : %d", volume);
			_callmgr_telephony_set_modem_volume(core_data->telephony_handle, tel_path, volume);
		}
	}

	return 0;
}

static int __callmgr_core_set_default_audio_route(callmgr_core_data_t *core_data)
{
	dbg("__callmgr_core_set_default_audio_route");
	CM_RETURN_VAL_IF_FAIL(core_data, -1);
	cm_telephony_call_data_t *call_data = NULL;
	callmgr_audio_route_e default_route = CALLMGR_AUDIO_ROUTE_NONE_E;
	int ret = -1;

	_callmgr_telephony_get_video_call(core_data->telephony_handle, &call_data);
	if (call_data) {
		default_route = CALLMGR_AUDIO_ROUTE_SPEAKER_EARJACK_E;
	} else {
		default_route = CALLMGR_AUDIO_ROUTE_RECEIVER_EARJACK_E;
	}

	ret = _callmgr_audio_set_audio_route(core_data->audio_handle, default_route);
	if (ret < 0) {
		err("_callmgr_audio_set_audio_route() fails");
		return -1;
	}

	return 0;
}

static int __callmgr_core_create_call_data(callmgr_call_data_t **call_data)
{
	dbg("__callmgr_core_create_call_data");
	CM_RETURN_VAL_IF_FAIL(call_data, -1);

	*call_data = g_new0(callmgr_call_data_t, 1);
	if (*call_data == NULL) {
		err("memory allocation failed");
		return -1;
	}
	return 0;
}

static int __callmgr_core_delete_call_data(callmgr_call_data_t *call_data)
{
	dbg("__callmgr_core_delete_call_data");
	CM_RETURN_VAL_IF_FAIL(call_data, -1);

	g_free(call_data->call_number);
	g_free(call_data);
	return 0;
}

static int __callmgr_core_update_call_data(callmgr_core_data_t *core_data, callmgr_call_data_t *call_data, cm_telephony_call_data_t *tel_call_data)
{
	unsigned int call_id = 0;
	cm_telephony_call_state_e call_state = CM_TEL_CALL_STATE_MAX;
	cm_telephony_call_direction_e call_direction = CM_TEL_CALL_DIRECTION_MO;
	cm_telephony_name_mode_e name_mode = CM_TEL_NAME_MODE_NONE;
	cm_telephony_call_type_e call_type = CM_TEL_CALL_TYPE_INVALID;
	cm_telepony_sim_slot_type_e active_sim_slot = CM_TELEPHONY_SIM_UNKNOWN;
	gboolean is_conference = FALSE;
	gboolean is_ecc = FALSE;
	gboolean is_voicemail_number = FALSE;
	char *number = NULL;
	char *calling_name = NULL;
	long start_time = 0;

	CM_RETURN_VAL_IF_FAIL(call_data, -1);
	CM_RETURN_VAL_IF_FAIL(tel_call_data, -1);
	CM_RETURN_VAL_IF_FAIL(core_data, -1);

	_callmgr_telephony_get_call_id(tel_call_data, &call_id);
	call_data->call_id = call_id;

	_callmgr_telephony_get_call_direction(tel_call_data, &call_direction);
	call_data->call_direction = call_direction;

	_callmgr_telephony_get_call_number(tel_call_data, &number);
	if (number) {
		g_free(call_data->call_number);
		call_data->call_number = number;
	} else {
		/* Reset previous number */
		g_free(call_data->call_number);
		call_data->call_number = NULL;
	}

	_callmgr_telephony_get_calling_name(tel_call_data, &calling_name);
	if (calling_name) {
		g_free(call_data->calling_name);
		call_data->calling_name = calling_name;
	}

	_callmgr_telephony_get_call_type(tel_call_data, &call_type);
	__callmgr_core_convert_tel_call_type(call_type, &(call_data->call_type));

	_callmgr_telephony_get_call_state(tel_call_data, &call_state);
	call_data->call_state = call_state;

	_callmgr_telephony_get_is_conference(tel_call_data, &is_conference);
	call_data->is_conference = is_conference;

	_callmgr_telephony_get_is_ecc(tel_call_data, &is_ecc);
	call_data->is_ecc = is_ecc;

	_callmgr_telephony_get_call_start_time(tel_call_data, &start_time);
	call_data->start_time = start_time;

	_callmgr_telephony_get_call_name_mode(tel_call_data, &name_mode);
	switch(name_mode) {
		case CM_TEL_NAME_MODE_UNKNOWN:
			call_data->name_mode = CALL_NAME_MODE_UNKNOWN;
			break;
		case CM_TEL_NAME_MODE_PRIVATE:
			call_data->name_mode = CALL_NAME_MODE_PRIVATE;
			break;
		case CM_TEL_NAME_MODE_PAYPHONE:
			call_data->name_mode = CALL_NAME_MODE_PAYPHONE;
			break;
		default:
			call_data->name_mode = CALL_NAME_MODE_NONE;
			break;
	}

	_callmgr_telephony_get_active_sim_slot(core_data->telephony_handle, &active_sim_slot);
	if (number)
		_callmgr_util_service_check_voice_mail(number, active_sim_slot, &is_voicemail_number);

	call_data->is_voicemail_number = is_voicemail_number;

	/*
	 * To do
	 * call_domain has to be updated
	 */

	return 0;

}
static void __callmgr_core_update_all_call_data(callmgr_core_data_t *core_data)
{
	CM_RETURN_IF_FAIL(core_data);
	cm_telephony_call_data_t *incom_call_data = NULL;
	cm_telephony_call_data_t *active_dial_call_data = NULL;
	cm_telephony_call_data_t *held_call_data = NULL;
	callmgr_telephony_t telephony_handle = core_data->telephony_handle;
	CM_RETURN_IF_FAIL(telephony_handle);

	// TODO: update incom/active/held here based on telephony call list get by callmgr_telephony_get_call_list()
	//Later in this function, we will implement updating hfp/sap call data.

	/* Incoming call */
	_callmgr_telephony_get_call_by_state(telephony_handle, CM_TEL_CALL_STATE_INCOMING, &incom_call_data);
	if(incom_call_data) {
		if(NULL == core_data->incom) {
			if(__callmgr_core_create_call_data(&core_data->incom) < 0) {
				err("__callmgr_core_create_call_data failed");
				return;
			}
		}
		__callmgr_core_update_call_data(core_data, core_data->incom, incom_call_data);
	} else {
		if (core_data->incom) {
			__callmgr_core_delete_call_data(core_data->incom);
			core_data->incom = NULL;
		}
	}

	/* Active call */
	_callmgr_telephony_get_call_by_state(telephony_handle, CM_TEL_CALL_STATE_DIALING, &active_dial_call_data);
	if(active_dial_call_data == NULL) {
		_callmgr_telephony_get_call_by_state(telephony_handle, CM_TEL_CALL_STATE_ALERT, &active_dial_call_data);
		if(active_dial_call_data == NULL) {
			_callmgr_telephony_get_call_by_state(telephony_handle, CM_TEL_CALL_STATE_ACTIVE, &active_dial_call_data);
			if(active_dial_call_data == NULL) {
				if (core_data->active_dial) {
					__callmgr_core_delete_call_data(core_data->active_dial);
					core_data->active_dial = NULL;
				}
			}
		}
	}

	if (active_dial_call_data) {
		if(core_data->active_dial == NULL) {
			if (__callmgr_core_create_call_data(&core_data->active_dial) < 0) {
				err("__callmgr_core_create_call_data failed");
				return;
			}
		}
		__callmgr_core_update_call_data(core_data, core_data->active_dial, active_dial_call_data);
	}

	/* Held call */
	_callmgr_telephony_get_call_by_state(telephony_handle, CM_TEL_CALL_STATE_HELD, &held_call_data);
	if(held_call_data) {
		if(NULL == core_data->held) {
			if(__callmgr_core_create_call_data(&core_data->held) < 0) {
				err("__callmgr_core_create_call_data failed");
				return;
			}
		}
		__callmgr_core_update_call_data(core_data, core_data->held, held_call_data);
	} else {
		if (core_data->held) {
			__callmgr_core_delete_call_data(core_data->held);
			core_data->held = NULL;
		}
	}

	return;
}

static gboolean __callmgr_core_dtmf_pause_timer_cb(gpointer puser_data)
{
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)puser_data;
	CM_RETURN_VAL_IF_FAIL(core_data, FALSE);

	/*Remove Pause Timer */
	if (core_data->dtmf_pause_timer > 0) {
		g_source_remove(core_data->dtmf_pause_timer);
		core_data->dtmf_pause_timer = -1;
	}
	__callmgr_core_process_dtmf_number(core_data);

	return FALSE;
}

static int __callmgr_core_process_dtmf_number(callmgr_core_data_t *core_data)
{
	char *dtmf_number = NULL;
	cm_telephony_call_data_t *call = NULL;
	CM_RETURN_VAL_IF_FAIL(core_data, -1);

	_callmgr_telephony_get_call_by_state(core_data->telephony_handle, CM_TEL_CALL_STATE_ACTIVE, &call);
	CM_RETURN_VAL_IF_FAIL(call, -1);
	_callmgr_telephony_get_dtmf_number(call, &dtmf_number);

	if (dtmf_number == NULL) {
		cm_telephony_sat_event_type_e sat_event_type = CM_TELEPHONY_SAT_EVENT_NONE;
		_callmgr_telephony_get_sat_event_type(core_data->telephony_handle, &sat_event_type);

		dbg("sat_event_type:%d", sat_event_type);
		if (sat_event_type == CM_TELEPHONY_SAT_EVENT_SEND_DTMF) {
			_callmgr_telephony_send_sat_response(core_data->telephony_handle,
					CM_TELEPHONY_SAT_EVENT_SEND_DTMF, CM_TELEPHONY_SAT_RESPONSE_ME_RET_SUCCESS, CM_TELEPHONY_SIM_UNKNOWN);
		}
		return 0;
	}

	dbg("Dtmf Number:%s", dtmf_number);

	/*Remove Pause Timer */
	if (core_data->dtmf_pause_timer > 0) {
		g_source_remove(core_data->dtmf_pause_timer);
		core_data->dtmf_pause_timer = -1;
	}

	/*Send DTMF */
	if ((dtmf_number[0]== 'p' ) || (dtmf_number[0] == 'P') || (dtmf_number[0] == ',')) {
		info("pause");
		_callmgr_dbus_send_dtmf_indi(core_data, CALL_DTMF_INDI_IDLE_E, NULL);
		if (strlen(dtmf_number) > 1) {
			_callmgr_telephony_set_dtmf_number(call, &dtmf_number[1]);
			core_data->dtmf_pause_timer = g_timeout_add(3000, __callmgr_core_dtmf_pause_timer_cb, core_data);
		}
		else {
			_callmgr_telephony_set_dtmf_number(call, NULL);
		}
	} else if ((dtmf_number[0] == 'w') || (dtmf_number[0] == 'W') || (dtmf_number[0] == ';')) {
		info("wait");
		_callmgr_dbus_send_dtmf_indi(core_data, CALL_DTMF_INDI_IDLE_E, NULL);
		if (strlen(dtmf_number) > 1) {
			_callmgr_telephony_set_dtmf_number(call, &dtmf_number[1]);
			_callmgr_dbus_send_dtmf_indi(core_data, CALL_DTMF_INDI_WAIT_E, &dtmf_number[1]);
		}
		else {
			_callmgr_telephony_set_dtmf_number(call, NULL);
		}
	} else {
		_callmgr_dbus_send_dtmf_indi(core_data, CALL_DTMF_INDI_PROGRESSING_E, dtmf_number);
		_callmgr_telephony_start_dtmf(core_data->telephony_handle, dtmf_number[0]);
		_callmgr_telephony_stop_dtmf(core_data->telephony_handle);

		if (strlen(dtmf_number) > 1) {
			_callmgr_telephony_set_dtmf_number(call, &dtmf_number[1]);
		}
		else {
			_callmgr_telephony_set_dtmf_number(call, NULL);
			_callmgr_dbus_send_dtmf_indi(core_data, CALL_DTMF_INDI_IDLE_E, NULL);
		}
	}

	g_free(dtmf_number);
	return 0;
}

static gboolean __callmgr_core_auto_answer_timer_cb(gpointer puser_data)
{
	dbg("..");
	int ret = -1;
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)puser_data;
	CM_RETURN_VAL_IF_FAIL(core_data, FALSE);

	if (core_data->auto_answer_timer > 0) {
		g_source_remove(core_data->auto_answer_timer);
		core_data->auto_answer_timer = 0;
	}

	if(core_data->active_dial == NULL && core_data->held == NULL){
		ret = _callmgr_core_process_answer_call(core_data,0);
		if (ret == 0) {
			warn("_callmgr_core_process_answer_call() is successfully done");
			core_data->is_auto_answered = TRUE;
		}
	}

	return FALSE;
}

static void __callmgr_core_auto_answer(callmgr_core_data_t *core_data)
{
	dbg("..");

	gboolean is_answer_mode_enabled = FALSE;
	gboolean is_bike_mode_enabled = FALSE;
	gboolean is_test_answer_mode_enabled = FALSE;

	gboolean is_bt_connected = FALSE;
	gboolean is_earjack_available = FALSE;
	int auto_answer_time_in_sec = 0;

	CM_RETURN_IF_FAIL(core_data);

	_callmgr_vconf_is_bike_mode_enabled(&is_bike_mode_enabled);
	_callmgr_vconf_is_answer_mode_enabled(&is_answer_mode_enabled);
	_callmgr_vconf_is_test_auto_answer_mode_enabled(&is_test_answer_mode_enabled);

	if (core_data->auto_answer_timer > 0) {
		g_source_remove(core_data->auto_answer_timer);
		core_data->auto_answer_timer = 0;
	}
	core_data->is_auto_answered = FALSE;

	if (TRUE == is_bike_mode_enabled) {
		warn("Bikemode enabled");
		core_data->auto_answer_timer = g_timeout_add_seconds(5, __callmgr_core_auto_answer_timer_cb, core_data);
	} else if (TRUE == is_test_answer_mode_enabled) {
		warn("Request testmode auto answer");
		core_data->auto_answer_timer = g_timeout_add_seconds(3, __callmgr_core_auto_answer_timer_cb, core_data);
	} else if (TRUE == is_answer_mode_enabled){

		_callmgr_bt_is_connected(core_data->bt_handle, &is_bt_connected);
		_callmgr_audio_is_sound_device_available(CALLMGR_AUDIO_DEVICE_EARJACK_E, &is_earjack_available);

		if (TRUE == is_bt_connected || TRUE == is_earjack_available) {
			if (_callmgr_vconf_get_auto_answer_time(&auto_answer_time_in_sec) < 0) {
				err("_callmgr_vconf_get_auto_answer_time failed");
				return;
			}

			core_data->auto_answer_timer = g_timeout_add_seconds(auto_answer_time_in_sec, __callmgr_core_auto_answer_timer_cb,core_data);
		}
		else{
			dbg("Without earjack or bt, skip auto answer ");
		}

	}

	return;
}

static void __callmgr_core_cancel_auto_answer(callmgr_core_data_t *core_data)
{
	dbg("..");

	CM_RETURN_IF_FAIL(core_data);

	if (core_data->auto_answer_timer > 0) {
		g_source_remove(core_data->auto_answer_timer);
		core_data->auto_answer_timer = 0;
	}
}

static void __callmgr_core_process_incoming_call(callmgr_core_data_t *core_data, cm_telephony_call_data_t *incom_call, callmgr_sim_slot_type_e sim_slot)
{
	gboolean has_active_call = FALSE;
	gboolean has_held_call = FALSE;

	gboolean is_callui_running = FALSE;
	gboolean is_earjack_available = FALSE;
	unsigned int call_id = -1;
	char *number = NULL;
	callmgr_contact_info_t *contact_out_info = NULL;
	cm_telephony_call_type_e call_type = CM_TEL_CALL_TYPE_CS_VOICE;
	callmgr_audio_session_mode_e sound_mode = CALLMGR_AUDIO_SESSION_NONE_E;
	int ret = -1;
	int person_id = -1;

	cm_telephony_call_type_e tel_call_type = CM_TEL_CALL_TYPE_INVALID;
	callmgr_call_type_e cm_call_type = CALL_TYPE_INVALID_E;
	cm_telephony_end_cause_type_e end_cause = CM_TELEPHONY_ENDCAUSE_MAX;

	dbg("__callmgr_core_process_incoming_call() called");
	CM_RETURN_IF_FAIL(core_data);
	CM_RETURN_IF_FAIL(incom_call);

	/*Launch CallUI*/
	_callmgr_telephony_get_call_id(incom_call, &call_id);
	_callmgr_telephony_get_call_type(incom_call, &tel_call_type);
	__callmgr_core_convert_tel_call_type(tel_call_type, &cm_call_type);

	/*Update Contact Info in contact list*/
	_callmgr_telephony_get_call_number(incom_call, &number);
	_callmgr_ct_add_ct_info((const char *)number, call_id, &contact_out_info);
	if (contact_out_info) {
		_callmgr_ct_get_person_id(call_id, &person_id);
		dbg("Contact Info added successfully for CallId : %d, ContactIdx : %d", call_id, person_id);
	} else {
		dbg("Failed to add contact object");
	}
	g_free(number);

	_callmgr_util_is_callui_running(&is_callui_running);
	if (FALSE == is_callui_running) {
		_callmgr_util_launch_callui(call_id, sim_slot, cm_call_type);
	} else {
		_callmgr_dbus_send_call_event(core_data, CALL_MANAGER_CALL_EVENT_INCOMING_E, call_id, sim_slot, end_cause);
	}

	__callmgr_core_send_bt_events(core_data, 0);

	_callmgr_telephony_has_call_by_state(core_data->telephony_handle, CM_TEL_CALL_STATE_ACTIVE, &has_active_call);
	_callmgr_telephony_has_call_by_state(core_data->telephony_handle, CM_TEL_CALL_STATE_HELD, &has_held_call);

	if (has_active_call || has_held_call) {
		info("Second incoming call!! Play waiting tone");
		_callmgr_ringer_start_alternate_tone(core_data->ringer_handle);
		return;
	}

	_callmgr_telephony_get_call_type(incom_call, &call_type);
	if (call_type == CM_TEL_CALL_TYPE_CS_VOICE) {
		sound_mode = CALLMGR_AUDIO_SESSION_VOICE_E;
	}
	else if (call_type == CM_TEL_CALL_TYPE_CS_VIDEO) {
		sound_mode = CALLMGR_AUDIO_SESSION_VIDEO_E;
	}
	else {
		err("Invalid call type[%d].", call_type);
		sound_mode = CALLMGR_AUDIO_SESSION_VIDEO_E;
	}

	_callmgr_ringer_stop_signal(core_data->ringer_handle);
	_callmgr_ringer_stop_effect(core_data->ringer_handle);

	ret = _callmgr_audio_create_call_sound_session(core_data->audio_handle, sound_mode);
	if (ret < 0) {
		err("_callmgr_audio_create_call_sound_session() failed");
	}

	_callmgr_audio_is_sound_device_available(CALLMGR_AUDIO_DEVICE_EARJACK_E, &is_earjack_available);
	if (person_id == -1) {
		_callmgr_ringer_start_alert(core_data->ringer_handle, NULL, is_earjack_available);
	} else {
		char *caller_ringtone_path = NULL;
		_callmgr_ct_get_caller_ringtone_path(call_id, &caller_ringtone_path);
		_callmgr_ringer_start_alert(core_data->ringer_handle, caller_ringtone_path, is_earjack_available);

	}

	/* Init motion sensor to use turn over mute */
	_callmgr_sensor_face_down_start(core_data->sensor_handle);

	/* Auto Answering */
	__callmgr_core_auto_answer(core_data);

	return;
}

static void __callmgr_core_start_incom_noti(callmgr_core_data_t *core_data)
{
	gboolean has_active_call = FALSE;
	gboolean has_held_call = FALSE;
	gboolean is_earjack_available = FALSE;
	cm_telephony_call_data_t *incom_call = NULL;
	cm_telephony_call_type_e tel_call_type = CM_TEL_CALL_TYPE_INVALID;
	int person_id = -1;
	unsigned int call_id = -1;

	dbg("..");
	CM_RETURN_IF_FAIL(core_data);
	_callmgr_telephony_get_call_by_state(core_data->telephony_handle, CM_TEL_CALL_STATE_INCOMING, &incom_call);

	if(incom_call) {
		_callmgr_telephony_has_call_by_state(core_data->telephony_handle, CM_TEL_CALL_STATE_ACTIVE, &has_active_call);
		_callmgr_telephony_has_call_by_state(core_data->telephony_handle, CM_TEL_CALL_STATE_HELD, &has_held_call);

		if (has_active_call || has_held_call) {
			info("Second incoming call!! Play waiting tone");
			_callmgr_ringer_start_alternate_tone(core_data->ringer_handle);
			return;
		}
		_callmgr_telephony_get_call_id(incom_call, &call_id);
		_callmgr_telephony_get_call_type(incom_call, &tel_call_type);
		_callmgr_ct_get_person_id(call_id, &person_id);

		_callmgr_ringer_stop_signal(core_data->ringer_handle);
		_callmgr_ringer_stop_effect(core_data->ringer_handle);

		_callmgr_audio_is_sound_device_available(CALLMGR_AUDIO_DEVICE_EARJACK_E, &is_earjack_available);
		if (person_id == -1) {
			_callmgr_ringer_start_alert(core_data->ringer_handle, NULL, is_earjack_available);
		} else {
			char *caller_ringtone_path = NULL;
			_callmgr_ct_get_caller_ringtone_path(call_id, &caller_ringtone_path);
			_callmgr_ringer_start_alert(core_data->ringer_handle, caller_ringtone_path, is_earjack_available);

		}

		/* Init motion sensor to use turn over mute */
		_callmgr_sensor_face_down_start(core_data->sensor_handle);

	} else {
		dbg("no incoming call exists...return");
	}
}

static void __callmgr_core_stop_incom_noti(callmgr_core_data_t *core_data)
{
	CM_RETURN_IF_FAIL(core_data);

	/* Stop ringtone */
	_callmgr_ringer_stop_alternate_tone(core_data->ringer_handle);
	_callmgr_ringer_stop_alert(core_data->ringer_handle);

	/* Stop motion sensor */
	_callmgr_sensor_face_down_stop(core_data->sensor_handle);
}

static void __callmgr_core_disconnect_tone_finished_cb(void *user_data)
{
	int call_cnt = 0;
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_IF_FAIL(core_data);

	_callmgr_telephony_get_call_count(core_data->telephony_handle, &call_cnt);
	if (0 == call_cnt) {
		_callmgr_audio_destroy_call_sound_session(core_data->audio_handle);
	}
	return;
}

static void __callmgr_core_signal_tone_finished_cb(void *user_data)
{
	int call_cnt = 0;
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_IF_FAIL(core_data);
	_callmgr_telephony_get_call_count(core_data->telephony_handle, &call_cnt);
	if (0 == call_cnt) {
		_callmgr_audio_destroy_call_sound_session(core_data->audio_handle);
	}
	return;
}

static void __callmgr_core_answer_msg_finished_cb(void *user_data)
{
	dbg("..");
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	callmgr_audio_device_e active_device = CALLMGR_AUDIO_DEVICE_NONE_E;
	char *call_name = NULL;
	int ret = -1;

	_callmgr_audio_set_link_direction_downlink();
	_callmgr_audio_get_active_device(core_data->audio_handle, &active_device);
	_callmgr_audio_set_audio_route(core_data->audio_handle, active_device);

	_callmgr_ct_get_call_name(core_data->active_dial->call_id, &call_name);
	ret = _callmgr_vr_start_record(core_data->vr_handle, core_data->active_dial->call_number, call_name, TRUE);
	g_free(call_name);
	if(ret < 0) {
		err("_callmgr_vr_start_record() failed");
		_callmgr_core_process_end_call(core_data, 0, CM_TEL_CALL_RELEASE_TYPE_ALL_ACTIVE_CALLS);
	}
	return;
}


int __callmgr_core_get_signal_type(cm_telephony_end_cause_type_e end_cause_type, callmgr_play_signal_type_e *signal_type)
{
	dbg("end_cause_type: %d", end_cause_type);
	*signal_type = CALL_SIGNAL_NONE_E;

	switch (end_cause_type) {
	case CM_TELEPHONY_ENDCAUSE_USER_DOESNOT_RESPOND:
	case CM_TELEPHONY_ENDCAUSE_USER_BUSY:
	case CM_TELEPHONY_ENDCAUSE_USER_UNAVAILABLE:
	case CM_TELEPHONY_ENDCAUSE_NO_ANSWER:
		{
			*signal_type = CALL_SIGNAL_USER_BUSY_TONE_E;
		}
		break;
	case CM_TELEPHONY_ENDCAUSE_INVALID_NUMBER_FORMAT:
	case CM_TELEPHONY_ENDCAUSE_NUMBER_CHANGED:
	case CM_TELEPHONY_ENDCAUSE_UNASSIGNED_NUMBER:
		{
			*signal_type = CALL_SIGNAL_WRONG_NUMBER_TONE_E;
		}
		break;
	case CM_TELEPHONY_ENDCAUSE_SERVICE_TEMP_UNAVAILABLE:
		{
			*signal_type = CALL_SIGNAL_CALL_FAIL_TONE_E;
		}
		break;
	case CM_TELEPHONY_ENDCAUSE_NW_BUSY:
		{
			*signal_type = CALL_SIGNAL_NW_CONGESTION_TONE_E;
		}
		break;
	default:
		break;
	}

	dbg("------ Signal Type : %d ---------", *signal_type);
	return 0;
}

static void __callmgr_core_process_telephony_events(cm_telephony_event_type_e event_type, void *event_data, void *user_data)
{
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	callmgr_audio_device_e active_device = CALLMGR_AUDIO_DEVICE_NONE_E;
	cm_telepony_sim_slot_type_e active_sim_slot = 0;
	gboolean is_bt_connected = FALSE;
	gboolean is_earjack_connected = FALSE;
	cm_telephony_end_cause_type_e end_cause = CM_TELEPHONY_ENDCAUSE_MAX;
	dbg("__callmgr_core_process_telephony_events() called %d", event_type);
	CM_RETURN_IF_FAIL(core_data);

	if ((event_type > CM_TELEPHONY_EVENT_IDLE) && (event_type <= CM_TELEPHONY_EVENT_RETRIEVED)) {
		/* Update call data for status event*/
		__callmgr_core_update_all_call_data(core_data);
	}

	_callmgr_telephony_get_active_sim_slot(core_data->telephony_handle, &active_sim_slot);

	switch (event_type) {
		case CM_TELEPHONY_EVENT_IDLE:
			{
				unsigned int call_id = GPOINTER_TO_UINT(event_data);
				callmgr_call_type_e cm_call_type = CALL_TYPE_INVALID_E;
				cm_telephony_call_data_t *call_data_out = NULL;
				cm_telephony_call_direction_e call_direction;
				cm_telephony_call_state_e call_state = CM_TEL_CALL_STATE_MAX;
				cm_telephony_call_type_e call_type = CM_TEL_CALL_TYPE_INVALID;
				cm_ct_plog_reject_type_e reject_type = 0;
				cm_ct_plog_data_t *log_data = NULL;
				int call_duration = 0;
				long start_time = 0;
				gboolean is_ecc = FALSE;
				gboolean b_play_effect_tone = TRUE;
				int call_cnt = 0;

				__callmgr_core_cancel_auto_answer(core_data);
				__callmgr_core_stop_incom_noti(core_data);
				_callmgr_ringer_stop_local_ringback_tone(core_data->ringer_handle);

				info("Call[%d] is ended", call_id);
				_callmgr_telephony_get_call_by_call_id(core_data->telephony_handle, call_id, &call_data_out);
				CM_RETURN_IF_FAIL(call_data_out);

				_callmgr_telephony_get_call_start_time(call_data_out, &start_time);
				_callmgr_util_get_time_diff(start_time, &call_duration);
				_callmgr_telephony_get_call_direction(call_data_out, &call_direction);
				_callmgr_telephony_get_call_state(call_data_out, &call_state);
				_callmgr_telephony_get_call_type(call_data_out, &call_type);
				__callmgr_core_convert_tel_call_type(call_type, &cm_call_type);
				_callmgr_ct_get_log_reject_type(call_id, &reject_type);
				_callmgr_telephony_get_call_end_cause(call_data_out, &end_cause);
				_callmgr_telephony_get_is_ecc(call_data_out, &is_ecc);

				log_data = (cm_ct_plog_data_t*)calloc(1, sizeof(cm_ct_plog_data_t));
				CM_RETURN_IF_FAIL(log_data);
				log_data->call_id = call_id;
				log_data->sim_slot = active_sim_slot;
				log_data->call_duration = call_duration;
				log_data->presentation = CM_CT_PLOG_PRESENT_DEFAULT;

				/* Set log presentation */
				if (is_ecc) {
					log_data->presentation = CM_CT_PLOG_PRESENT_EMERGENCY;
				}
				else {
					cm_telephony_name_mode_e name_mode = CM_TEL_NAME_MODE_NONE;
					_callmgr_telephony_get_call_name_mode(call_data_out, &name_mode);
					if (name_mode == CM_TEL_NAME_MODE_UNKNOWN) {
						log_data->presentation = CM_CT_PLOG_PRESENT_UNAVAILABLE;
					}
					else if (name_mode == CM_TEL_NAME_MODE_PRIVATE) {
						log_data->presentation = CM_CT_PLOG_PRESENT_REJECT_BY_USER;
					}
					else if (name_mode == CM_TEL_NAME_MODE_PAYPHONE) {
						log_data->presentation = CM_CT_PLOG_PRESENT_PAYPHONE;
					}
					else {
						log_data->presentation = CM_CT_PLOG_PRESENT_DEFAULT;
					}
				}

				/* Set log type */
				if (call_direction == CM_TEL_CALL_DIRECTION_MT) {
					if (call_state == CM_TEL_CALL_STATE_INCOMING) {
						if (reject_type == CM_CT_PLOG_REJECT_TYPE_NORMAL_E) {
							if (cm_call_type == CALL_TYPE_VIDEO_E) {
								log_data->log_type = CM_CT_PLOG_TYPE_VIDEO_REJECT_E;
							}
							else {
								log_data->log_type = CM_CT_PLOG_TYPE_VOICE_REJECT_E;
							}
						}
						else if (reject_type == CM_CT_PLOG_REJECT_TYPE_BLOCKED_E) {
							if (cm_call_type == CALL_TYPE_VIDEO_E) {
								log_data->log_type = CM_CT_PLOG_TYPE_VIDEO_BLOCKED_E;
							}
							else {
								log_data->log_type = CM_CT_PLOG_TYPE_VOICE_BLOCKED_E;
							}
						}
						else if (reject_type == CM_CT_PLOG_REJECT_TYPE_SETTING_REJECT_E) {
							if (cm_call_type == CALL_TYPE_VIDEO_E) {
								log_data->log_type = CM_CT_PLOG_TYPE_VIDEO_REJECT_E;
							}
							else {
								log_data->log_type = CM_CT_PLOG_TYPE_VOICE_REJECT_E;
							}
						}
						else if (reject_type == CM_CT_PLOG_REJECT_TYPE_REC_REJECT_E) {
							if (cm_call_type == CALL_TYPE_VIDEO_E) {
								log_data->log_type = CM_CT_PLOG_TYPE_VIDEO_BLOCKED_E;
							}
							else {
								log_data->log_type = CM_CT_PLOG_TYPE_VOICE_BLOCKED_E;
							}
						}
						else {
							if (cm_call_type == CALL_TYPE_VIDEO_E) {
								log_data->log_type = CM_CT_PLOG_TYPE_VIDEO_INCOMMING_UNSEEN_E;
							}
							else {
								log_data->log_type = CM_CT_PLOG_TYPE_VOICE_INCOMMING_UNSEEN_E;
							}
						}
						b_play_effect_tone = FALSE;
					}
					else {
						if (cm_call_type == CALL_TYPE_VIDEO_E) {
							log_data->log_type = CM_CT_PLOG_TYPE_VIDEO_INCOMMING_E;
						}
						else {
							log_data->log_type = CM_CT_PLOG_TYPE_VOICE_INCOMMING_E;
						}
					}
				}
				else {
					if (cm_call_type == CALL_TYPE_VIDEO_E) {
						log_data->log_type = CM_CT_PLOG_TYPE_VIDEO_OUTGOING_E;
					}
					else {
						log_data->log_type = CM_CT_PLOG_TYPE_VOICE_OUTGOING_E;
					}
				}
				_callmgr_ct_add_log(log_data);
				g_free(log_data);

				/* Delete the callData */
				_callmgr_telephony_call_delete(core_data->telephony_handle, call_id);
				_callmgr_ct_delete_ct_info(call_id);
				__callmgr_core_update_all_call_data(core_data);

				_callmgr_dbus_send_call_event(core_data, CALL_MANAGER_CALL_EVENT_IDLE_E, call_id, active_sim_slot, end_cause);
				__callmgr_core_send_bt_events(core_data, call_id);

				if (core_data->answer_msg_handle) {
					_callmgr_answer_msg_deinit(core_data->answer_msg_handle);
					core_data->answer_msg_handle = NULL;
				}
				_callmgr_telephony_get_call_count(core_data->telephony_handle, &call_cnt);

				/* Destroy call session */
				if (call_cnt == 0) {
					int ret = -1;
					callmgr_vr_state_e record_state = CALLMGR_VR_NONE;
					cm_ringer_status_e ringer_status = CM_RINGER_STATUS_IDLE_E;

					__callmgr_core_set_call_status(core_data, CALL_MANAGER_CALL_STATUS_IDLE_E, cm_call_type, NULL);

					_callmgr_audio_set_media_mode_with_current_device();
					if (call_direction == CM_TEL_CALL_DIRECTION_MO) {
						callmgr_play_signal_type_e signal_type = CALL_SIGNAL_NONE_E;
						__callmgr_core_get_signal_type(end_cause, &signal_type);
						if (_callmgr_ringer_play_signal(core_data->ringer_handle, signal_type, __callmgr_core_signal_tone_finished_cb, core_data) == 0) {
							err("_callmgr_ringer_play_signal() success");
							b_play_effect_tone = FALSE;
						}
					}
					if (b_play_effect_tone) {
						if (_callmgr_ringer_play_effect(core_data->ringer_handle, CM_RINGER_EFFECT_DISCONNECT_TONE_E, __callmgr_core_disconnect_tone_finished_cb, core_data) < 0) {
							err("_callmgr_ringer_play_effect() is failed");
						}
					}

					_callmgr_ringer_get_ringer_status(core_data->ringer_handle, &ringer_status);
					if (ringer_status == CM_RINGER_STATUS_IDLE_E) {
						_callmgr_audio_destroy_call_sound_session(core_data->audio_handle);
					}

					/* When we make a call and switch on mute and then end the call, the next time we call the mute should be reset to off. So the below API is called here.*/
					_callmgr_telephony_set_audio_tx_mute(core_data->telephony_handle, FALSE);
					core_data->is_mute_on = FALSE;
					_callmgr_dbus_send_mute_status(core_data, 0);

					_callmgr_vr_get_recording_state(core_data->vr_handle, &record_state);
					if (record_state == CALLMGR_VR_RECORD_REQUESTED ||
							record_state == CALLMGR_VR_RECORDING ||
							record_state == CALLMGR_VR_PAUSED) {
						ret = _callmgr_core_process_record_stop(core_data);
						if (ret < 0) {
							err("_callmgr_core_process_record_stop() failed[%d]", ret);
						}
					}
					core_data->is_auto_answered = FALSE;
					_callmgr_telephony_set_active_sim_slot(core_data->telephony_handle, CM_TELEPHONY_SIM_UNKNOWN);
				}
				else if (call_cnt > 0) {
					call_data_out = NULL;
					_callmgr_telephony_get_call_by_state(core_data->telephony_handle, CM_TEL_CALL_STATE_INCOMING, &call_data_out);
					if ((call_data_out == NULL) && (core_data->call_status != CALL_MANAGER_CALL_STATUS_OFFHOOK_E)) {
						__callmgr_core_set_call_status(core_data, CALL_MANAGER_CALL_STATUS_OFFHOOK_E, cm_call_type, NULL);
					}
					gboolean answer_requesting = FALSE;
					_callmgr_telephony_is_answer_requesting(core_data->telephony_handle, &answer_requesting);
					if ((call_cnt == 1) &&(call_data_out)) {
						// All other call is ended except incoming call. Restart to play ringtone
						if (!answer_requesting) {
							/* Release and Accept request is not done */
							char *caller_ringtone_path = NULL;
							unsigned int call_id = 0;
							_callmgr_telephony_get_call_id(call_data_out, &call_id);

							_callmgr_ct_get_caller_ringtone_path(call_id, &caller_ringtone_path);
							_callmgr_audio_is_sound_device_available(CALLMGR_AUDIO_DEVICE_EARJACK_E, &is_earjack_connected);
							_callmgr_ringer_start_alert(core_data->ringer_handle, caller_ringtone_path, is_earjack_connected);
							CM_SAFE_FREE(caller_ringtone_path);
						}
					} else if (answer_requesting && (call_cnt > 1) && (call_data_out)) {
						/* Incoming + Other Calls Are Present. */
						gboolean is_held_call = FALSE;
						_callmgr_telephony_has_call_by_state(core_data->telephony_handle, CM_TEL_CALL_STATE_HELD, &is_held_call);
						if(FALSE == is_held_call) {
							/* All held call is ended... */
							cm_telephony_answer_request_type_e end_active_after_held_end = CM_TEL_INVALID;
							_callmgr_telephony_get_answer_request_type(core_data->telephony_handle, &end_active_after_held_end);
							if(CM_TEL_END_ACTIVE_AFTER_HELD_END == end_active_after_held_end) {
								dbg("End active call and accept incoming call...");
								_callmgr_core_process_answer_call(core_data, CM_TEL_CALL_ANSWER_TYPE_RELEASE_ACTIVE_AND_ACCEPT);
							} else if (CM_TEL_HOLD_ACTIVE_AFTER_HELD_END == end_active_after_held_end) {
								dbg("Hold active call and accept incoming call...");
								_callmgr_core_process_answer_call(core_data, CM_TEL_CALL_ANSWER_TYPE_HOLD_ACTIVE_AND_ACCEPT);
							}
							_callmgr_telephony_set_answer_request_type(core_data->telephony_handle, CM_TEL_INVALID);
						}
					}
				}
			}
			break;
		case CM_TELEPHONY_EVENT_ACTIVE:
			{
				unsigned int call_id = GPOINTER_TO_UINT(event_data);
				cm_telephony_call_data_t *call_data_out = NULL;
				callmgr_call_type_e cm_call_type = CALL_TYPE_INVALID_E;
				cm_telephony_call_type_e call_type = CM_TEL_CALL_TYPE_INVALID;
				gboolean is_bike_mode_enabled = FALSE;

				_callmgr_telephony_get_call_by_call_id(core_data->telephony_handle, call_id, &call_data_out);
				_callmgr_telephony_get_call_type(call_data_out, &call_type);
				__callmgr_core_convert_tel_call_type(call_type, &cm_call_type);

				_callmgr_dbus_send_call_event(core_data, CALL_MANAGER_CALL_EVENT_ACTIVE_E, call_id, active_sim_slot, end_cause);
				_callmgr_ringer_stop_alert(core_data->ringer_handle);
				_callmgr_ringer_stop_local_ringback_tone(core_data->ringer_handle);

				_callmgr_vconf_is_bike_mode_enabled(&is_bike_mode_enabled);
				if ((core_data->is_auto_answered) && (is_bike_mode_enabled)) {
					warn("Auto-answered call!![Bike Mode]");
					_callmgr_audio_set_link_direction_uplink();
				}

				_callmgr_audio_get_active_device(core_data->audio_handle, &active_device);
				if (active_device == CALLMGR_AUDIO_DEVICE_NONE_E) {
					_callmgr_bt_is_connected(core_data->bt_handle, &is_bt_connected);
					if (is_bt_connected) {
						if (_callmgr_bt_open_sco(core_data->bt_handle) < 0) {
							err("SCO open failed.");
							__callmgr_core_set_default_audio_route(core_data);
						}
					} else {
						__callmgr_core_set_default_audio_route(core_data);
					}
				}
				else {
					gboolean is_ringtone_mode = FALSE;

					_callmgr_audio_is_ringtone_mode(core_data->audio_handle, &is_ringtone_mode);
					if (is_ringtone_mode) {
						/* Active > 2nd incoming > End active > Incoming > Active */
						/* We need to set previous audio route */
						warn("Set prev route");
						int ret = _callmgr_audio_set_audio_route(core_data->audio_handle, active_device);
						if (ret < 0) {
							err("_callmgr_audio_set_audio_route() fails");
						}
					}
				}

				if (core_data->call_status != CALL_MANAGER_CALL_STATUS_OFFHOOK_E) {
					__callmgr_core_set_call_status(core_data, CALL_MANAGER_CALL_STATUS_OFFHOOK_E, cm_call_type, NULL);
				}

				__callmgr_core_send_bt_events(core_data, 0);

				__callmgr_core_process_dtmf_number(core_data);

				if ((core_data->is_auto_answered) && (is_bike_mode_enabled)) {
					warn("Auto-answered call!![Bike Mode]");
					// Play answer msg
					if (_callmgr_answer_msg_init(&core_data->answer_msg_handle) < 0) {
						err("_callmgr_answer_msg_init() failed");
						_callmgr_core_process_end_call(core_data, call_id, CM_TEL_CALL_RELEASE_TYPE_BY_CALL_HANDLE);
						core_data->is_auto_answered = FALSE;
					} else if (_callmgr_answer_msg_play_msg(core_data->answer_msg_handle, __callmgr_core_answer_msg_finished_cb, core_data) < 0) {
						_callmgr_core_process_end_call(core_data, call_id, CM_TEL_CALL_RELEASE_TYPE_BY_CALL_HANDLE);
						_callmgr_answer_msg_deinit(core_data->answer_msg_handle);
						core_data->answer_msg_handle = NULL;
						core_data->is_auto_answered = FALSE;
					}
				}
			}
			break;
		case CM_TELEPHONY_EVENT_HELD:
			{
				unsigned int call_id = GPOINTER_TO_UINT(event_data);

				_callmgr_dbus_send_call_event(core_data, CALL_MANAGER_CALL_EVENT_HELD_E, call_id, active_sim_slot, end_cause);
				__callmgr_core_send_bt_events(core_data, 0);
			}
			break;
		case CM_TELEPHONY_EVENT_DIALING:
			{
				int ret = 0;
				unsigned int call_id = GPOINTER_TO_UINT(event_data);
				char *number = NULL;
				callmgr_contact_info_t *contact_out_info = NULL;
				cm_telephony_call_data_t *call_data_out = NULL;
				cm_telephony_call_type_e tel_call_type = CM_TEL_CALL_TYPE_INVALID;
				callmgr_call_type_e cm_call_type = CALL_TYPE_INVALID_E;
				callmgr_audio_session_mode_e audio_session = CALLMGR_AUDIO_SESSION_NONE_E;
				cm_telephony_call_data_t *sat_call = NULL;

				if (_callmgr_telephony_get_sat_originated_call(core_data->telephony_handle, &sat_call) == 0) {
					_callmgr_telephony_send_sat_response(core_data->telephony_handle,
							CM_TELEPHONY_SAT_EVENT_SETUP_CALL, CM_TELEPHONY_SAT_RESPONSE_ME_RET_SUCCESS, CM_TELEPHONY_SIM_UNKNOWN);
				}

				_callmgr_telephony_get_call_by_call_id(core_data->telephony_handle, call_id, &call_data_out);
				/*Update Contact Info in contact list*/
				if (call_data_out) {
					_callmgr_telephony_get_call_type(call_data_out, &tel_call_type);
					_callmgr_telephony_get_call_number(call_data_out, &number);
					_callmgr_ct_add_ct_info((const char *)number, call_id, &contact_out_info);
					if (contact_out_info) {
						int person_id = -1;
						_callmgr_ct_get_person_id(call_id, &person_id);
						dbg("Contact Info added successfully for CallId : %d, ContactIdx : %d", call_id, person_id);
					} else {
						dbg("Failed to add contact object");
					}

					g_free(number);
				}

				_callmgr_dbus_send_call_event(core_data, CALL_MANAGER_CALL_EVENT_DIALING_E, call_id, active_sim_slot, end_cause);
				__callmgr_core_send_bt_events(core_data, 0);

				__callmgr_core_convert_tel_call_type(tel_call_type, &cm_call_type);
				if(CALL_TYPE_VOICE_E == cm_call_type) {
					audio_session = CALLMGR_AUDIO_SESSION_VOICE_E;
				} else {
					audio_session = CALLMGR_AUDIO_SESSION_VIDEO_E;
				}

				_callmgr_ringer_stop_signal(core_data->ringer_handle);
				_callmgr_ringer_stop_effect(core_data->ringer_handle);

				ret = _callmgr_audio_create_call_sound_session(core_data->audio_handle, audio_session);
				if (ret < 0) {
					err("_callmgr_audio_create_call_sound_session() failed");
				}

				_callmgr_audio_get_active_device(core_data->audio_handle, &active_device);
				if (active_device == CALLMGR_AUDIO_DEVICE_NONE_E) {
					_callmgr_bt_is_connected(core_data->bt_handle, &is_bt_connected);

					if (is_bt_connected) {
						if (_callmgr_bt_open_sco(core_data->bt_handle) < 0) {
							err("SCO open failed.");
							__callmgr_core_set_default_audio_route(core_data);
						}
					}
					else {
						__callmgr_core_set_default_audio_route(core_data);
					}
				}

				if (core_data->call_status != CALL_MANAGER_CALL_STATUS_OFFHOOK_E) {
					__callmgr_core_set_call_status(core_data, CALL_MANAGER_CALL_STATUS_OFFHOOK_E, cm_call_type, NULL);
				}
			}
			break;
		case CM_TELEPHONY_EVENT_ALERT:
			{
				unsigned int call_id = GPOINTER_TO_UINT(event_data);

				_callmgr_ringer_play_effect(core_data->ringer_handle, CM_RINGER_EFFECT_CONNECT_TONE_E, NULL, NULL);
				_callmgr_dbus_send_call_event(core_data, CALL_MANAGER_CALL_EVENT_ALERT_E, call_id, active_sim_slot, end_cause);
				__callmgr_core_send_bt_events(core_data, 0);
			}
			break;
		case CM_TELEPHONY_EVENT_INCOMING:
			{
				cm_telephony_call_data_t *call = (cm_telephony_call_data_t *)event_data;
				callmgr_call_type_e cm_call_type = CALL_TYPE_INVALID_E;
				cm_telephony_call_type_e call_type = CM_TEL_CALL_TYPE_INVALID;

				if (call) {
					char *number = NULL;
					_callmgr_telephony_get_call_number(call, &number);
					gboolean is_blocked_num = FALSE;
					gboolean is_rec_blocked = FALSE;
					gboolean is_bike_mode_enabled = FALSE;
					int call_cnt = -1;
					gboolean is_do_not_disturb = FALSE;

					_callmgr_telephony_get_call_type(call, &call_type);
					__callmgr_core_convert_tel_call_type(call_type, &cm_call_type);
					
					/* Video call does not support on SPIN. Reject video call */
					if (cm_call_type == CALL_TYPE_VIDEO_E) {
						warn("Video call does not support currently! Rejecting incoming call");
						_callmgr_telephony_reject_call(core_data->telephony_handle);
						g_free(number);
						return;
					}

					_callmgr_util_check_block_mode_num(number, &is_blocked_num);
					_callmgr_vconf_is_recording_reject_enabled(&is_rec_blocked);
					_callmgr_vconf_is_bike_mode_enabled(&is_bike_mode_enabled);

					_callmgr_util_check_disturbing_setting(&is_do_not_disturb);
					_callmgr_telephony_get_call_count(core_data->telephony_handle, &call_cnt);
					if((is_blocked_num == TRUE)
						|| (is_rec_blocked == TRUE)
						|| (is_do_not_disturb == TRUE)
						|| ((is_bike_mode_enabled == TRUE) && (call_cnt > 1))	// if 2nd incoming call
						|| ((is_bike_mode_enabled == TRUE) && (cm_call_type == CALL_TYPE_VIDEO_E))) {
						warn("Auto reject incoming call");
						callmgr_contact_info_t *contact_out_info = NULL;

						int ret = _callmgr_telephony_reject_call(core_data->telephony_handle);
						if (ret < 0) {
							err("_callmgr_telephony_reject_call() get failed");
						}
						/* Add contact info for blocked number */
						_callmgr_ct_add_ct_info((const char *)number, core_data->incom->call_id, &contact_out_info);

						if (is_rec_blocked) {
							_callmgr_ct_set_log_reject_type(core_data->incom->call_id, CM_CT_PLOG_REJECT_TYPE_REC_REJECT_E);
						} else if (is_do_not_disturb) {
							_callmgr_ct_set_log_reject_type(core_data->incom->call_id, CM_CT_PLOG_REJECT_TYPE_SETTING_REJECT_E);
						} else {
							_callmgr_ct_set_log_reject_type(core_data->incom->call_id, CM_CT_PLOG_REJECT_TYPE_BLOCKED_E);
						}
					} else {
						__callmgr_core_process_incoming_call(core_data, call, active_sim_slot);
						__callmgr_core_set_call_status(core_data, CALL_MANAGER_CALL_STATUS_RINGING_E, cm_call_type, number);
					}
					g_free(number);
					break;
				}
			}
			break;
		case CM_TELEPHONY_EVENT_WAITING:
			{
				unsigned int call_id = GPOINTER_TO_UINT(event_data);
				_callmgr_dbus_send_call_event(core_data, CALL_MANAGER_CALL_EVENT_WAITING_E, call_id, active_sim_slot, end_cause);
				__callmgr_core_send_bt_events(core_data, 0);
			}
			break;
		case CM_TELEPHONY_EVENT_JOIN:
			{
				unsigned int call_id = GPOINTER_TO_UINT(event_data);
				_callmgr_dbus_send_call_event(core_data, CALL_MANAGER_CALL_EVENT_JOIN_E, call_id, active_sim_slot, end_cause);
				__callmgr_core_send_bt_events(core_data, 0);
			}
			break;
		case CM_TELEPHONY_EVENT_SPLIT:
			{
				unsigned int call_id = GPOINTER_TO_UINT(event_data);
				_callmgr_dbus_send_call_event(core_data, CALL_MANAGER_CALL_EVENT_SPLIT_E, call_id, active_sim_slot, end_cause);
				__callmgr_core_send_bt_events(core_data, 0);
			}
			break;
		case CM_TELEPHONY_EVENT_SWAP:
			{
				unsigned int call_id = GPOINTER_TO_UINT(event_data);
				_callmgr_dbus_send_call_event(core_data, CALL_MANAGER_CALL_EVENT_SWAP_E, call_id, active_sim_slot, end_cause);
				__callmgr_core_send_bt_events(core_data, 0);
			}
			break;
		case CM_TELEPHONY_EVENT_RETRIEVED:
			{
				unsigned int call_id = GPOINTER_TO_UINT(event_data);
				_callmgr_dbus_send_call_event(core_data, CALL_MANAGER_CALL_EVENT_RETRIEVED_E, call_id, active_sim_slot, end_cause);
				__callmgr_core_send_bt_events(core_data, 0);
			}
			break;
		case CM_TELEPHONY_EVENT_NETWORK_ERROR:
			{
				int result = GPOINTER_TO_INT(event_data);
				if (result == CM_TEL_CALL_ERR_NONE) {
					_callmgr_dbus_send_dial_status(core_data, CALL_MANAGER_DIAL_SUCCESS);
				}
				else {
					cm_telephony_call_data_t *call = NULL;

					if (result == CM_TEL_CALL_ERR_FDN_ONLY) {
						_callmgr_util_launch_popup(CALL_POPUP_CALL_ERR, CALL_ERR_FDN_CALL_ONLY, NULL, 0, NULL, NULL);
						_callmgr_dbus_send_dial_status(core_data, CALL_MANAGER_DIAL_FAIL_FDN);
					}
					else if (result == CM_TEL_CALL_ERR_FM_OFF_FAIL) {
						_callmgr_util_launch_popup(CALL_POPUP_CALL_ERR, CALL_ERR_FAILED, NULL, 0, NULL, NULL);
						_callmgr_dbus_send_dial_status(core_data, CALL_MANAGER_DIAL_FAIL_FLIGHT_MODE);
					}
					else {
						_callmgr_util_launch_popup(CALL_POPUP_CALL_ERR, CALL_ERR_FAILED, NULL, 0, NULL, NULL);
						_callmgr_dbus_send_dial_status(core_data, CALL_MANAGER_DIAL_FAIL);
					}

					if (_callmgr_telephony_get_sat_originated_call(core_data->telephony_handle, &call) == 0) {
						_callmgr_telephony_send_sat_response(core_data->telephony_handle,
								CM_TELEPHONY_SAT_EVENT_SETUP_CALL, CM_TELEPHONY_SAT_RESPONSE_ME_UNABLE_TO_PROCESS_COMMAND, CM_TELEPHONY_SIM_UNKNOWN);
					}
				}
			}
			break;
		case CM_TELEPHONY_EVENT_SOUND_WBAMR:
			{
				//int wb_status = (int)event_data;
				/* ToDo : We need to set WB information and update it */
			}
			break;

		case CM_TELEPHONY_EVENT_SOUND_CLOCK_STATUS:
			{
				/* ToDo : We need to set path for this event */
			}
			break;
		case CM_TELEPHONY_EVENT_SOUND_RINGBACK_TONE:
			{
				int ringbacktone_info = GPOINTER_TO_INT(event_data);
				/* play /stop ringback tone */
				dbg("ringback info received = %d ", ringbacktone_info);

				if (ringbacktone_info == 1) {
					_callmgr_ringer_play_local_ringback_tone(core_data->ringer_handle);
				}
			}
			break;
		case CM_TELEPHONY_EVENT_SS_INFO:
			{
				int info_type = GPOINTER_TO_INT(event_data);
				_callmgr_util_launch_popup(CALL_POPUP_SS_INFO, info_type, NULL, 0, NULL, NULL);
			}
			break;
		case CM_TELEPHONY_EVENT_FLIGHT_MODE_TIME_OVER_E:
			{
				cm_telephony_call_data_t *call = NULL;
				int ret = 0;
				_callmgr_util_launch_popup(CALL_POPUP_HIDE, CALL_ERR_NONE, NULL, 0, NULL, NULL);

				ret = _callmgr_telephony_get_call_by_call_id(core_data->telephony_handle, NO_CALL_HANDLE, &call);
				if (ret < 0 || call == NULL) {
					warn("Invalid call");
					_callmgr_util_launch_popup(CALL_POPUP_CALL_ERR, CALL_ERR_FAILED, NULL, 0, NULL, NULL);
				}
				else {
					ret = _callmgr_telephony_dial(core_data->telephony_handle, call);
					if (ret < 0) {
						err("_callmgr_telephony_dial get failed");
						_callmgr_telephony_call_delete(core_data->telephony_handle, NO_CALL_HANDLE);
					}
				}
			}
			break;
		case CM_TELEPHONY_EVENT_PREFERRED_SIM_CHANGED:
			{
				int preferred_sim = GPOINTER_TO_INT(event_data);
				dbg("preferred SIM [%d]", preferred_sim);
				_callmgr_util_launch_popup(CALL_POPUP_HIDE, CALL_ERR_NONE, NULL, 0, NULL, NULL);
			}
			break;
		case CM_TELEPHONY_EVENT_SAT_SETUP_CALL:
			{
				int ret = 0;
				char *number = NULL, *name = NULL;
				_callmgr_telephony_get_sat_setup_call_number(core_data->telephony_handle, &number);
				_callmgr_telephony_get_sat_setup_call_name(core_data->telephony_handle, &name);

				if (!number) {
					err("number is NULL.");
					goto SAT_SETUP_CALL_FAIL_EXIT;
				}

				ret = _callmgr_util_launch_callui_by_sat(active_sim_slot);
				if (ret != 0) {
					err("_callmgr_util_launch_callui_by_sat() is failed[%d].", ret);
					goto SAT_SETUP_CALL_FAIL_EXIT;
				}

				ret = _callmgr_core_process_dial(core_data, number, CALL_TYPE_VOICE_E,
						active_sim_slot, 0, FALSE, TRUE);
				if (ret != 0) {
					err("_callmgr_core_process_dial() is failed[%d].", ret);
					goto SAT_SETUP_CALL_FAIL_EXIT;
				}

				// will send sat response when setup call is succesfull.
				return;

SAT_SETUP_CALL_FAIL_EXIT:
				_callmgr_telephony_send_sat_response(core_data->telephony_handle,
						CM_TELEPHONY_SAT_EVENT_SETUP_CALL, CM_TELEPHONY_SAT_RESPONSE_ME_UNABLE_TO_PROCESS_COMMAND, CM_TELEPHONY_SIM_UNKNOWN);
				return;
			}
			break;
		case CM_TELEPHONY_EVENT_SAT_SEND_DTMF:
			{
				cm_telephony_call_data_t *call = NULL;
				char *sat_dtmf_number = (char *)event_data;

				_callmgr_telephony_get_call_by_state(core_data->telephony_handle, CM_TEL_CALL_STATE_ACTIVE, &call);
				if (call == NULL) {
					err("No active call");
					goto SAT_SEND_DTMF_FAIL_EXIT;
				}

				_callmgr_telephony_set_dtmf_number(call, sat_dtmf_number);
				if (__callmgr_core_process_dtmf_number(core_data) != 0) {
					err("__callmgr_core_process_dtmf_number() is failed.");
					goto SAT_SEND_DTMF_FAIL_EXIT;
				}

				// will send sat response when dtmp processing is finished.
				return;

SAT_SEND_DTMF_FAIL_EXIT:
				_callmgr_telephony_send_sat_response(core_data->telephony_handle,
						CM_TELEPHONY_SAT_EVENT_SEND_DTMF, CM_TELEPHONY_SAT_RESPONSE_ME_UNABLE_TO_PROCESS_COMMAND, CM_TELEPHONY_SIM_UNKNOWN);
				return;
			}
			break;
		case CM_TELEPHONY_EVENT_SAT_CALL_CONTROL_RESULT:
			{
				gboolean *b_ui_update = event_data;

				_callmgr_telephony_send_sat_response(core_data->telephony_handle,
						CM_TELEPHONY_SAT_EVENT_CALL_CONTROL_RESULT, CM_TELEPHONY_SAT_RESPONSE_NONE, CM_TELEPHONY_SIM_UNKNOWN);

				if (*b_ui_update == TRUE)
					_callmgr_dbus_send_call_event(core_data, CALL_MANAGER_CALL_EVENT_CALL_CONTROL_E, 0, active_sim_slot, end_cause);
				return;
			}
			break;
		case CM_TELEPHON_EVENT_DTMF_STOP_ACK:
			{
				cm_telephony_dtmf_result_e result = (cm_telephony_dtmf_result_e)event_data;
				cm_telephony_call_data_t *active_call = NULL;

				_callmgr_telephony_get_call_by_state(core_data->telephony_handle, CM_TEL_CALL_STATE_ACTIVE, &active_call);
				CM_RETURN_IF_FAIL(active_call);

				if (result == CM_TEL_DTMF_SEND_SUCCESS) {
					__callmgr_core_process_dtmf_number(core_data);
				} else {
					_callmgr_telephony_set_dtmf_number(active_call, NULL);
					_callmgr_dbus_send_dtmf_indi(core_data, CALL_DTMF_INDI_IDLE_E, NULL);
				}
			}
			break;
		case CM_TELEPHON_EVENT_DTMF_START_ACK:
			{
				cm_telephony_dtmf_result_e result = (cm_telephony_dtmf_result_e)event_data;
				cm_telephony_call_data_t *active_call = NULL;

				_callmgr_telephony_get_call_by_state(core_data->telephony_handle, CM_TEL_CALL_STATE_ACTIVE, &active_call);
				CM_RETURN_IF_FAIL(active_call);

				if (result != CM_TEL_DTMF_SEND_SUCCESS) {
					_callmgr_telephony_set_dtmf_number(active_call, NULL);
					_callmgr_dbus_send_dtmf_indi(core_data, CALL_DTMF_INDI_IDLE_E, NULL);
				}
			}
			break;
		default:
			break;
	}
}

static void __callmgr_core_process_audio_events(cm_audio_event_type_e event_type, void *event_data, void *user_data)
{
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;

	CM_RETURN_IF_FAIL(core_data);
	info("audio event : %d", event_type);
	switch (event_type) {
		case CM_AUDIO_EVENT_PATH_CHANGED_E:
			{
				callmgr_audio_device_e active_device = (callmgr_audio_device_e)event_data;
				callmgr_path_type_e route = CALL_AUDIO_PATH_NONE_E;

				_callmgr_core_get_audio_state(core_data, &route);
				_callmgr_dbus_send_audio_status(core_data, route);
				__callmgr_core_set_telephony_audio_route(core_data, active_device);
			}
			break;
		case CM_AUDIO_EVENT_EARJACK_CHANGED_E:
			{
				gboolean is_available = GPOINTER_TO_INT(event_data);
				dbg("Earjack state : %d", is_available);
				if (core_data->active_dial || core_data->held) {
					/*Change path only if outgoing call or connected call exists */
					callmgr_audio_device_e active_device = CALLMGR_AUDIO_DEVICE_NONE_E;
					dbg("Change path");

					_callmgr_audio_get_active_device(core_data->audio_handle, &active_device);


					if (is_available == TRUE) {
						if (active_device == CALLMGR_AUDIO_DEVICE_BT_E) {
							_callmgr_bt_close_sco(core_data->bt_handle);
						}
						else {
							_callmgr_audio_set_audio_route(core_data->audio_handle, CALLMGR_AUDIO_ROUTE_EARJACK_E);
						}
					}
					else {
						gboolean is_bt_connected = FALSE;
						_callmgr_bt_is_connected(core_data->bt_handle, &is_bt_connected);

						if (is_bt_connected) {
							_callmgr_bt_open_sco(core_data->bt_handle);
						} else if (active_device != CALLMGR_AUDIO_DEVICE_SPEAKER_E) {
							gboolean is_cradle_conn = FALSE;
							_callmgr_vconf_is_cradle_status(&is_cradle_conn);

							if (is_cradle_conn) {
								info("Cradle is connected. Turn on SPK");
								_callmgr_audio_set_audio_route(core_data->audio_handle, CALLMGR_AUDIO_ROUTE_SPEAKER_E);
							} else {
								__callmgr_core_set_default_audio_route(core_data);
							}
						}
					}
				}
				else if (core_data->incom) {
				}
			}
			break;
		case CM_AUDIO_EVENT_VOLUME_CHANGED_E:
			{
				int volume = GPOINTER_TO_INT(event_data);
				callmgr_path_type_e route = CALL_AUDIO_PATH_NONE_E;
				cm_telephony_audio_path_type_e telephony_type = CM_TAPI_SOUND_PATH_HANDSET;

				_callmgr_core_get_audio_state(core_data, &route);
				switch(route){
					case CALL_AUDIO_PATH_SPEAKER_E:
						telephony_type = CM_TAPI_SOUND_PATH_SPK_PHONE;
						break;
					case CALL_AUDIO_PATH_EARJACK_E:
						telephony_type = CM_TAPI_SOUND_PATH_HEADSET;
						break;
					case CALL_AUDIO_PATH_BT_E:
						telephony_type = CM_TAPI_SOUND_PATH_BLUETOOTH;
						break;
					default:
						telephony_type = CM_TAPI_SOUND_PATH_HANDSET;
						break;
				}
				dbg("type : %d, vol : %d", telephony_type, volume);

				_callmgr_telephony_set_modem_volume(core_data->telephony_handle, telephony_type, volume);
			}
			break;
		default:
			break;
	}
}

static void __callmgr_core_send_bt_list(callmgr_core_data_t *core_data, int call_cnt)
{
	GSList *call_list = NULL;
	unsigned int call_id = 0;
	char *call_number = NULL;
	cm_telephony_call_state_e call_state = CM_TEL_CALL_STATE_MAX;
	cm_telephony_call_data_t *call_obj = NULL;
	CM_RETURN_IF_FAIL(core_data);

	info("call_cnt : %d", call_cnt);
	if (_callmgr_telephony_get_call_list(core_data->telephony_handle, &call_list) == 0) {
		while (call_list) {
			call_obj = (cm_telephony_call_data_t*)call_list->data;
			if (!call_obj) {
				warn("[ error ] call object : 0");
				call_list = call_list->next;
				continue;
			}

			_callmgr_telephony_get_call_id(call_obj, &call_id);

			_callmgr_telephony_get_call_number(call_obj, &call_number);

			_callmgr_telephony_get_call_state(call_obj, &call_state);

			_callmgr_bt_add_call_list(core_data->bt_handle, call_id, call_state, call_number);

			g_free(call_number);
			call_list = g_slist_next(call_list);
		}

		g_slist_free(call_list);
		_callmgr_bt_send_call_list(core_data->bt_handle);
	}
	else {
		err("Err");
	}
}

static void __callmgr_core_send_bt_events(callmgr_core_data_t *core_data, int idle_call_id)
{
	dbg("__callmgr_core_send_bt_events()");
	int call_cnt = 0;
	CM_RETURN_IF_FAIL(core_data);

	if (idle_call_id != 0) {
		/* Send IDLE event*/

		_callmgr_bt_send_event(core_data->bt_handle, CM_BT_AG_RES_CALL_IDLE_E, idle_call_id, NULL);
		return;
	}

	_callmgr_telephony_get_call_count(core_data->telephony_handle, &call_cnt);
	info("call_cnt : %d", call_cnt);
	if (call_cnt == 0) {
		info("No Call");
		return;
	} else {
		cm_bt_call_event_type_e call_event = CM_BT_AG_RES_CALL_IDLE_E;
		char *number = NULL;
		int call_id = 0;

		if (call_cnt == 1) {
			if (core_data->incom) {
				call_event = CM_BT_AG_RES_CALL_INCOM_E;
				number = g_strdup(core_data->incom->call_number);
				call_id = core_data->incom->call_id;
			} else if (core_data->active_dial) {
				if (core_data->active_dial->call_state == CM_TEL_CALL_STATE_DIALING) {
					call_event = CM_BT_AG_RES_CALL_DIALING_E;
				} else if (core_data->active_dial->call_state == CM_TEL_CALL_STATE_ALERT) {
					call_event = CM_BT_AG_RES_CALL_ALERT_E;
				} else {
					call_event = CM_BT_AG_RES_CALL_ACTIVE_E;
				}
				call_id = core_data->active_dial->call_id;
				number = g_strdup(core_data->active_dial->call_number);
			} else if (core_data->held) {
				call_event = CM_BT_AG_RES_CALL_HELD_E;
				call_id = core_data->held->call_id;
				number = g_strdup(core_data->held->call_number);
			} else {
				warn("Invalid state");
				return;
			}

			_callmgr_bt_send_event(core_data->bt_handle, call_event, call_id, number);
			g_free(number);
		} else {
			__callmgr_core_send_bt_list(core_data, call_cnt);

			if (core_data->incom) {
				call_event = CM_BT_AG_RES_CALL_INCOM_E;
				number = g_strdup(core_data->incom->call_number);
				call_id = core_data->incom->call_id;
			} else if (core_data->active_dial) {
				if (core_data->active_dial->call_state == CM_TEL_CALL_STATE_DIALING) {
					call_event = CM_BT_AG_RES_CALL_DIALING_E;
				} else if (core_data->active_dial->call_state == CM_TEL_CALL_STATE_ALERT) {
					call_event = CM_BT_AG_RES_CALL_ALERT_E;
				} else {
					call_event = CM_BT_AG_RES_CALL_ACTIVE_E;
				}
				call_id = core_data->active_dial->call_id;
				number = g_strdup(core_data->active_dial->call_number);
			}
			if (call_event != CM_BT_AG_RES_CALL_IDLE_E) {
				_callmgr_bt_send_event(core_data->bt_handle, call_event, call_id, number);
			}
			g_free(number);
		}
	}
}

static void __callmgr_core_process_bt_events(cm_bt_event_type_e event_type, void *event_data, void *user_data)
{
	gboolean is_connected = FALSE;
	gboolean is_sco_opned = FALSE;
	char *dtmf_char = NULL;
	int spk_gain = 0;
	int req_call_id = 0;
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_IF_FAIL(core_data);
	info("bt event : %d", event_type);

	switch (event_type) {
		case CM_BT_EVENT_CONNECTION_CHANGED_E:
			{
				is_connected = GPOINTER_TO_INT(event_data);
				if (is_connected) {
					int call_cnt = 0;

					_callmgr_telephony_get_call_count(core_data->telephony_handle, &call_cnt);
					if (call_cnt == 1) {
						/* Send call list for init */
						__callmgr_core_send_bt_list(core_data, call_cnt);
					}

					/* Send init event */
					__callmgr_core_send_bt_events(core_data, 0);

				/*	Headset is connected, Set the Sound Status to Headset
					and change the path only incase of mocall and connected call */

					if (core_data->active_dial || core_data->held) {
						/* Open SCO to set BT path */
						_callmgr_bt_open_sco(core_data->bt_handle);
					}
					else {
						info("No active call. Ignore");
					}
				}
				else {
					callmgr_audio_device_e active_device = CALLMGR_AUDIO_DEVICE_NONE_E;
					_callmgr_audio_get_active_device(core_data->audio_handle, &active_device);
					if (active_device == CALLMGR_AUDIO_DEVICE_BT_E) {
						if (core_data->active_dial || core_data->held) {
						}
						else {
							info("No active call. Ignore");
						}
					}
					else {
						info("Not BT. ignore event");
					}

				}
				/* Todo : Send event to UI*/
			}
			break;
		case CM_BT_EVENT_SCO_CHANGED_E:
			{
				is_sco_opned = GPOINTER_TO_INT(event_data);

				/* Todo : We need to check incom for inband ringtone */
				if (core_data->active_dial || core_data->held) {
					if (is_sco_opned) {
						_callmgr_audio_set_audio_route(core_data->audio_handle, CALLMGR_AUDIO_ROUTE_BT_E);
					}
					else {
						callmgr_audio_device_e active_device = CALLMGR_AUDIO_DEVICE_NONE_E;
						_callmgr_audio_get_active_device(core_data->audio_handle, &active_device);
						if (active_device == CALLMGR_AUDIO_DEVICE_BT_E) {
							__callmgr_core_set_default_audio_route(core_data);
						}
					}
					/* Todo : Send event to UI*/
				}
				else {
					info("No active call. Ignore");
				}
			}
			break;
		case CM_BT_EVENT_DTMF_TRANSMITTED_E:
			{
				dtmf_char = (char *)event_data;
				dbg("dtmf : %s", dtmf_char);

				if (core_data->active_dial) {
					_callmgr_telephony_start_dtmf(core_data->telephony_handle, dtmf_char[0]);
					_callmgr_telephony_stop_dtmf(core_data->telephony_handle);
				}
				else {
					info("No active call. Ignore");
				}
			}
			break;
		case CM_BT_EVENT_SPK_GAIN_CHANGED_E:
			{
				spk_gain = GPOINTER_TO_INT(event_data);
				dbg("spk gain : %d", spk_gain);

				if (core_data->active_dial || core_data->held) {
					/* Todo: Send event to UI */
				}
				else {
					info("No active call. Ignore");
				}
			}
			break;
		case CM_BT_EVENT_CALL_HANDLE_ACCEPT_E:
			{
				gboolean is_bike_mode_enabled = FALSE;
				req_call_id = GPOINTER_TO_INT(event_data);
				dbg("req call id : %d", req_call_id);

				_callmgr_vconf_is_bike_mode_enabled(&is_bike_mode_enabled);
				if (is_bike_mode_enabled) {
					err("Bikemode enabled. Ignore this event");
					break;
				}

				if (core_data->incom) {
					_callmgr_core_process_answer_call(core_data, CM_TEL_CALL_ANSWER_TYPE_NORMAL);
				}
				else {
					info("No incom call. Ignore");
				}
			}
			break;

		case CM_BT_EVENT_CALL_HANDLE_REJECT_E:
			{
				gboolean is_bike_mode_enabled = FALSE;
				req_call_id = GPOINTER_TO_INT(event_data);
				dbg("req call id : %d", req_call_id);

				_callmgr_vconf_is_bike_mode_enabled(&is_bike_mode_enabled);
				if (is_bike_mode_enabled) {
					err("Bikemode enabled. Ignore this event");
					break;
				}

				if (core_data->incom) {
					_callmgr_core_process_reject_call(core_data);
				}
				else {
					info("No incom call. Ignore");
				}
			}
			break;
		case CM_BT_EVENT_CALL_HANDLE_RELEASE_E:
			{
				req_call_id = GPOINTER_TO_INT(event_data);
				dbg("req call id : %d", req_call_id);

				if (core_data->active_dial || core_data->held) {
					_callmgr_core_process_end_call(core_data, req_call_id, CM_TEL_CALL_RELEASE_TYPE_BY_CALL_HANDLE);
				}
				else {
					info("No connect call. Ignore");
				}
			}
			break;
		case CM_BT_EVENT_CALL_HANDLE_0_SEND_E:
			{
				_callmgr_telephony_process_incall_ss(core_data->telephony_handle, "0");
			}
			break;
		case CM_BT_EVENT_CALL_HANDLE_1_SEND_E:
			{
				_callmgr_telephony_process_incall_ss(core_data->telephony_handle, "1");
			}
			break;
		case CM_BT_EVENT_CALL_HANDLE_2_SEND_E:
			{
				_callmgr_telephony_process_incall_ss(core_data->telephony_handle, "2");
			}
			break;
		case CM_BT_EVENT_CALL_HANDLE_3_SEND_E:
			{
				_callmgr_telephony_process_incall_ss(core_data->telephony_handle, "3");
			}
			break;
		case CM_BT_EVENT_CALL_HANDLE_4_SEND_E:
			{
				_callmgr_telephony_process_incall_ss(core_data->telephony_handle, "4");
			}
			break;

		default:
			warn("Invalid event");
			break;
	}
}
static void __callmgr_core_process_vr_events(callmgr_vr_status_event_e status, callmgr_vr_status_extra_type_e extra_data, void *user_data)
{
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_IF_FAIL(core_data);
	info("vr event : vr_status[%d], extra_data[0x%02x]", status, extra_data);

	switch(status) {
	case CALLMGR_VR_STATUS_EVENT_STARTED:
		_callmgr_dbus_send_vr_status(core_data, status, extra_data);
		break;
	case CALLMGR_VR_STATUS_EVENT_STOPPED: {
		cm_util_rec_status_sub_info_e sub_info = CM_UTIL_REC_STATUS_NONE_E;
		_callmgr_dbus_send_vr_status(core_data, status, extra_data);

		switch(extra_data) {
		case CALLMGR_VR_STATUS_EXTRA_STOP_TYPE_BY_NO_FREE_SPACE:
			sub_info = CM_UTIL_REC_STATUS_STOP_BY_MEMORY_FULL_E;
			break;
		case CALLMGR_VR_STATUS_EXTRA_STOP_TYPE_BY_TIME_SHORT:
			sub_info = CM_UTIL_REC_STATUS_STOP_BY_TIME_SHORT;
			break;
		default:
			sub_info = CM_UTIL_REC_STATUS_STOP_BY_NORMAL_E;
			break;
		}

		_callmgr_util_launch_popup(CALL_POPUP_REC_STATUS, sub_info, NULL, 0, NULL, NULL);
//		if (core_data->vr_handle->is_answering) {
//		//if (ad->answering_memo_status != VCUI_ANSWERING_MEMO_NONE) {
//			//_callmgr_util_launch_popup(CALL_POPUP_TOAST, CALL_POPUP_TOAST_SWAP_CALL, NULL, 0 , NULL, NULL);
//			//_callmgr_popup_create_toast_msg(_("IDS_CALL_TPOP_RECORDED_MESSAGE_SAVED"));
//		} else {
//			//_callmgr_util_launch_popup(CALL_POPUP_TOAST, CALL_POPUP_TOAST_SWAP_CALL, NULL, 0 , NULL, NULL);
//			//_callmgr_popup_create_toast_msg(_("IDS_CALL_POP_RECORDING_SAVED"));
//		}
	}	break;
	default:
		warn("Invalid event");
		break;
	}

	return;
}

static void __callmgr_core_process_sensor_events(cm_sensor_event_type_e event_type, void *event_data, void *user_data)
{
	info("sensor event : %d", event_type);
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_IF_FAIL(core_data);

	switch (event_type) {
		case CM_SENSOR_EVENT_TURN_OVER_E:
			{
				dbg("Detect turn over");
				__callmgr_core_stop_incom_noti(core_data);
			}
			break;
		default:
			warn("Invalid event");
			break;
	}
}

int _callmgr_core_init(callmgr_core_data_t **o_core_data)
{
	CM_RETURN_VAL_IF_FAIL(o_core_data, -1);

	callmgr_core_data_t *core_data = NULL;
	core_data = calloc(1, sizeof(callmgr_core_data_t));
	CM_RETURN_VAL_IF_FAIL(core_data, -1);

	_callmgr_dbus_init (core_data);

	_callmgr_telephony_init(&core_data->telephony_handle, __callmgr_core_process_telephony_events, core_data);
	CM_RETURN_VAL_IF_FAIL(core_data->telephony_handle, -1);

	_callmgr_audio_init(&core_data->audio_handle, __callmgr_core_process_audio_events, core_data);
	CM_RETURN_VAL_IF_FAIL(core_data->audio_handle, -1);

	_callmgr_ringer_init(&core_data->ringer_handle);
	CM_RETURN_VAL_IF_FAIL(core_data->ringer_handle, -1);

	_callmgr_bt_init(&core_data->bt_handle, __callmgr_core_process_bt_events, core_data);
	CM_RETURN_VAL_IF_FAIL(core_data->bt_handle, -1);

	_callmgr_sensor_init(&core_data->sensor_handle, __callmgr_core_process_sensor_events, core_data);
	CM_RETURN_VAL_IF_FAIL(core_data->sensor_handle, -1);

	_callmgr_vr_init(&core_data->vr_handle, __callmgr_core_process_vr_events, core_data);
	CM_RETURN_VAL_IF_FAIL(core_data->vr_handle, -1);

	*o_core_data = core_data;

	_callmgr_ct_set_missed_call_notification();

	return 0;
}

int _callmgr_core_deinit(callmgr_core_data_t *core_data)
{
	CM_RETURN_VAL_IF_FAIL(core_data, -1);

	_callmgr_telephony_deinit(core_data->telephony_handle);
	_callmgr_audio_deinit(core_data->audio_handle);
	_callmgr_ringer_deinit(core_data->ringer_handle);
	_callmgr_bt_deinit(core_data->bt_handle);
	_callmgr_sensor_deinit(core_data->sensor_handle);
	_callmgr_vr_deinit(core_data->vr_handle);

	g_free(core_data);
	return 0;
}

int _callmgr_core_get_audio_state(callmgr_core_data_t *core_data, callmgr_path_type_e *o_audio_state)
{
	CM_RETURN_VAL_IF_FAIL(core_data, -1);
	dbg("_callmgr_core_get_audio_state is called");
	callmgr_audio_device_e active_device = CALLMGR_AUDIO_DEVICE_NONE_E;
	if (_callmgr_audio_get_active_device(core_data->audio_handle, &active_device) < 0) {
		dbg("_callmgr_audio_get_active_device() failed");
		return -1;
	}

	switch(active_device) {
		case CALLMGR_AUDIO_DEVICE_SPEAKER_E:
			*o_audio_state = CALL_AUDIO_PATH_SPEAKER_E;
			break;
		case CALLMGR_AUDIO_DEVICE_RECEIVER_E:
			*o_audio_state = CALL_AUDIO_PATH_RECEIVER_E;
			break;
		case CALLMGR_AUDIO_DEVICE_EARJACK_E:
			*o_audio_state = CALL_AUDIO_PATH_EARJACK_E;
			break;
		case CALLMGR_AUDIO_DEVICE_BT_E:
			*o_audio_state = CALL_AUDIO_PATH_BT_E;
			break;
		default:
			err("Invalid device type: [%d]", active_device);
			*o_audio_state = CALL_AUDIO_PATH_NONE_E;
			break;
	}

	return 0;
}

static gboolean __callmgr_core_check_is_mocall_possible(callmgr_core_data_t *core_data, gboolean is_emergency, callmgr_call_type_e call_type, callmgr_call_error_cause_e* reason)
{
	CM_RETURN_VAL_IF_FAIL(core_data, FALSE);
	cm_telephony_call_data_t *call = NULL;
	int ret_val = -1;
	gboolean is_exist = FALSE;
	gboolean is_phone_initialized = FALSE;
	gboolean is_fm_enabled = FALSE;
	cm_telepony_sim_slot_type_e active_sim = CM_TELEPHONY_SIM_UNKNOWN;
	gboolean b_has_active_call = FALSE;
	gboolean b_has_held_call = FALSE;
	int call_count = -1;
	dbg("__callmgr_core_check_is_mocall_possible()");

	_callmgr_telephony_is_phone_initialized(core_data->telephony_handle, &is_phone_initialized);
	if (is_phone_initialized == FALSE) {
		err("Couldn't make outgoing call. because phone is not initialized");
		*reason = CALL_ERR_CAUSE_PHONE_NOT_INITIALIZED_E;
		return FALSE;
	}

	_callmgr_telephony_is_flight_mode_enabled(core_data->telephony_handle, &is_fm_enabled);
	if (is_fm_enabled) {
		err("Flight Mode Enabled");
		*reason = CALL_ERR_CAUSE_FLIGHT_MODE_E;
		return FALSE;
	}

	_callmgr_telephony_get_active_sim_slot(core_data->telephony_handle, &active_sim);
	if (active_sim == CM_TELEPHONY_SIM_UNKNOWN) {
		err("Sim is no selected.");
		*reason = CALL_ERR_CAUSE_NOT_REGISTERED_ON_NETWORK_E;
		return FALSE;
	}

	if(CALL_TYPE_VIDEO_E == call_type) {
		_callmgr_telephony_get_voice_call(core_data->telephony_handle, &call);
		if (call) {
			unsigned int call_id = NO_CALL_HANDLE;
			_callmgr_telephony_get_call_id(call, &call_id);
			err("Couldn't make outgoing call. because of Voice call[%d] is exist", call_id);
			*reason = CALL_ERR_CAUSE_NOT_ALLOWED_E;
			return FALSE;
		}
	}

	_callmgr_telephony_has_call_by_state(core_data->telephony_handle, CM_TEL_CALL_STATE_DIALING, &is_exist);
	if (is_exist) {
		err("Couldn't make outgoing call. because dialing call is already exist");
		*reason = CALL_ERR_CAUSE_NOT_ALLOWED_E;
		return FALSE;
	}

	_callmgr_telephony_has_call_by_state(core_data->telephony_handle, CM_TEL_CALL_STATE_ALERT, &is_exist);
	if (is_exist) {
		err("Couldn't make outgoing call. because alerting call is already exist");
		*reason = CALL_ERR_CAUSE_NOT_ALLOWED_E;
		return FALSE;
	}

	_callmgr_telephony_has_call_by_state(core_data->telephony_handle, CM_TEL_CALL_STATE_INCOMING, &is_exist);
	if (is_exist) {
		err("Couldn't make outgoing call. because incoming call is exist");
		*reason = CALL_ERR_CAUSE_NOT_ALLOWED_E;
		return FALSE;
	}

	_callmgr_telephony_has_call_by_state(core_data->telephony_handle, CM_TEL_CALL_STATE_ACTIVE, &b_has_active_call);
	_callmgr_telephony_has_call_by_state(core_data->telephony_handle, CM_TEL_CALL_STATE_HELD, &b_has_held_call);
	if ((b_has_active_call) && (b_has_held_call)) {
		err("1 active & 1 hold status. Couldn't make outgoing call.");
		*reason = CALL_ERR_CAUSE_NOT_ALLOWED_E;
		return FALSE;
	}

	ret_val = _callmgr_telephony_get_call_count(core_data->telephony_handle, &call_count);
	if ((ret_val < 0) || (call_count > CALLMGR_MAX_CALL_GROUP_MEMBER)) {
		err("maximum call count over!! [%d]", call_count);
		*reason = CALL_ERR_CAUSE_NOT_ALLOWED_E;
		return FALSE;
	}

	_callmgr_telephony_get_video_call(core_data->telephony_handle, &call);
	if (call) {
		unsigned int call_id = NO_CALL_HANDLE;
		_callmgr_telephony_get_call_id(call, &call_id);
		err("Couldn't make outgoing call. because of Video call[%d] is exist", call_id);
		*reason = CALL_ERR_CAUSE_NOT_ALLOWED_E;
		return FALSE;
	}

	if (!is_emergency) {
		cm_network_status_e network_status = CM_NETWORK_STATUS_OUT_OF_SERVICE;
		gboolean is_no_sim = FALSE;
		gboolean is_cs_available = FALSE;

		_callmgr_telephony_is_no_sim(core_data->telephony_handle, &is_no_sim);
		if (is_no_sim) {
			err("Sim Not available. Emergency call only available");
			*reason = CALL_ERR_CAUSE_NOT_REGISTERED_ON_NETWORK_E;
			return FALSE;
		}

		ret_val = _callmgr_telephony_get_network_status(core_data->telephony_handle, &network_status);
		if ((ret_val < 0) || network_status == CM_NETWORK_STATUS_OUT_OF_SERVICE) {
			err("Network mode: Out of service");
			*reason = CALL_ERR_CAUSE_NOT_ALLOWED_E;
			return FALSE;
		}
		else if (network_status == CM_NETWORK_STATUS_EMERGENCY_ONLY) {
			err("Network mode: Emergency only");
			*reason = CALL_ERR_CAUSE_EMERGENCY_ONLY_E;
			return FALSE;
		}
		else if(CALL_TYPE_VIDEO_E == call_type && network_status == CM_NETWORK_STATUS_IN_SERVICE_2G) {
			err("Network mode: 2G...Not sufficient for video call...");
			*reason = CALL_ERR_CAUSE_NETWORK_2G_ONLY_E;
			return FALSE;
		}

		_callmgr_telephony_is_cs_available(core_data->telephony_handle, &is_cs_available);
		if (is_cs_available == FALSE) {
			err("CS unavailable");
			*reason = CALL_ERR_CAUSE_NOT_ALLOWED_E;
			return FALSE;
		}
		// TODO: MDM check
	}

	return TRUE;
}

static void __callmgr_core_popup_result_cb(call_popup_type popup_type, void *result, void *user_data)
{
	dbg("__callmgr_core_popup_result_cb()");
	callmgr_core_data_t *core_data = (callmgr_core_data_t*)user_data;
	if (popup_type == CALL_POPUP_FLIGHT_MODE) {
		cm_util_popup_result_e popup_result = (cm_util_popup_result_e)result;
		if (popup_result == CM_UTIL_POPUP_RESP_CANCEL) {
			_callmgr_dbus_send_dial_status(core_data, CALL_MANAGER_DIAL_FAIL_FLIGHT_MODE);
		}
		else {
			info("OK button on FM popup is clicked ");
		}
	}
	else if ((popup_type == CALL_POPUP_SIM_SELECT) || (popup_type == CALL_POPUP_OUT_OF_3G_TRY_VOICE_CALL)) {
		cm_util_popup_result_e popup_result = (cm_util_popup_result_e)result;
		if (popup_result == CM_UTIL_POPUP_RESP_CANCEL) {
			_callmgr_dbus_send_dial_status(core_data, CALL_MANAGER_DIAL_CANCEL);
		}
	}

	return;
}

static int __callmgr_core_launch_error_popup(callmgr_call_error_cause_e error_cause, const char *number)
{
	CM_RETURN_VAL_IF_FAIL(number, -1);

	switch(error_cause) {
	case CALL_ERR_CAUSE_FLIGHT_MODE_E:
		_callmgr_util_launch_popup(CALL_POPUP_FLIGHT_MODE, CALL_ERR_NONE, number, 0, NULL, NULL);
		break;
	case CALL_ERR_CAUSE_EMERGENCY_ONLY_E:
		_callmgr_util_launch_popup(CALL_POPUP_CALL_ERR, CALL_ERR_EMERGENCY_ONLY, NULL, 0, NULL, NULL);
		break;
	case CALL_ERR_CAUSE_NETWORK_2G_ONLY_E:
		_callmgr_util_launch_popup(CALL_POPUP_OUT_OF_3G_TRY_VOICE_CALL, CALL_ERR_NONE, number, 0, NULL, NULL);
		break;
	case CALL_ERR_CAUSE_IS_NOT_EMERGENCY_NUMBER_E:
		_callmgr_util_launch_popup(CALL_POPUP_CALL_ERR, CALL_ERR_IS_NOT_EMERGENCY, number, 0, NULL, NULL);
		break;
	case CALL_ERR_CAUSE_NOT_ALLOWED_E:
		_callmgr_util_launch_popup(CALL_POPUP_CALL_ERR, CALL_ERR_NOT_ALLOWED, NULL, 0, NULL, NULL);
		break;
	case CALL_ERR_CAUSE_WRONG_NUMBER_E:
		_callmgr_util_launch_popup(CALL_POPUP_CALL_ERR, CALL_ERR_WRONG_NUMBER, NULL, 0, NULL, NULL);
		break;
	case CALL_ERR_CAUSE_PHONE_NOT_INITIALIZED_E:
		_callmgr_util_launch_popup(CALL_POPUP_CALL_ERR, CALL_ERR_PHONE_NOT_INITIALIZED, NULL, 0, NULL, NULL);
		break;
	case CALL_ERR_CAUSE_NOT_REGISTERED_ON_NETWORK_E:
		_callmgr_util_launch_popup(CALL_POPUP_CALL_ERR, CALL_ERR_NOT_REGISTERED_ON_NETWORK, NULL, 0, NULL, NULL);
		break;
	default:
		_callmgr_util_launch_popup(CALL_POPUP_CALL_ERR, CALL_ERR_FAILED, NULL, 0, NULL, NULL);
		break;
	}

	return 0;
}

static int __callmgr_core_select_sim_slot(callmgr_core_data_t *core_data, const char *number, int call_type, gboolean is_emergency_contact)
{
	int sim_init_cnt = 0;
	gboolean is_sim1_initialized = FALSE;
	gboolean is_sim2_initialized = FALSE;
	cm_telepony_sim_slot_type_e preferred_slot = CM_TELEPHONY_SIM_UNKNOWN;
	cm_telepony_preferred_sim_type_e pref_sim_type = CM_TEL_PREFERRED_UNKNOWN_E;
	CM_RETURN_VAL_IF_FAIL(core_data, -1);
	CM_RETURN_VAL_IF_FAIL(number, -1);	// TODO: should return proper error code.

	_callmgr_telephony_is_sim_initialized(core_data->telephony_handle, CM_TELEPHONY_SIM_1, &is_sim1_initialized);
	if (is_sim1_initialized) {
		sim_init_cnt++;
	}
	_callmgr_telephony_is_sim_initialized(core_data->telephony_handle, CM_TELEPHONY_SIM_2, &is_sim2_initialized);
	if (is_sim2_initialized) {
		sim_init_cnt++;
	}

	_callmgr_telephony_set_active_sim_slot(core_data->telephony_handle, CM_TELEPHONY_SIM_UNKNOWN);
	if (sim_init_cnt > 1) {
		_callmgr_telephony_get_preferred_sim_type(core_data->telephony_handle, &pref_sim_type);
		if (pref_sim_type == CM_TEL_PREFERRED_SIM_1_E) {
			preferred_slot = CM_TELEPHONY_SIM_1;
		}
		else if (pref_sim_type == CM_TEL_PREFERRED_SIM_2_E) {
			preferred_slot = CM_TELEPHONY_SIM_2;
		}
		else {
			if (!is_emergency_contact) {
				_callmgr_util_launch_popup(CALL_POPUP_SIM_SELECT, call_type, number, 0, __callmgr_core_popup_result_cb, core_data);
				return -2;
			}
			else {
				preferred_slot = CM_TELEPHONY_SIM_1;
			}
		}
	}
	else if (sim_init_cnt == 1) {
		if (is_sim1_initialized) {
			preferred_slot = CM_TELEPHONY_SIM_1;
		}
		else {
			preferred_slot = CM_TELEPHONY_SIM_2;
		}
	}
	else {
		err("SIM not initialized!!");
		preferred_slot = CM_TELEPHONY_SIM_1;
	}
	_callmgr_telephony_set_active_sim_slot(core_data->telephony_handle, preferred_slot);

	if (is_emergency_contact) {
		cm_network_status_e net_status = CM_NETWORK_STATUS_OUT_OF_SERVICE;
		_callmgr_telephony_get_network_status(core_data->telephony_handle, &net_status);
		if (net_status == CM_NETWORK_STATUS_OUT_OF_SERVICE || net_status == CM_NETWORK_STATUS_EMERGENCY_ONLY) {
			int modem_cnt = 0;
			_callmgr_telephony_get_modem_count(core_data->telephony_handle, &modem_cnt);
			if (modem_cnt > 1) {
				cm_telepony_sim_slot_type_e alternative_slot = CM_TELEPHONY_SIM_UNKNOWN;
				if (preferred_slot == CM_TELEPHONY_SIM_1) {
					alternative_slot = CM_TELEPHONY_SIM_2;
				}
				else {
					alternative_slot = CM_TELEPHONY_SIM_1;
				}

				_callmgr_telephony_set_active_sim_slot(core_data->telephony_handle, alternative_slot);
				_callmgr_telephony_get_network_status(core_data->telephony_handle, &net_status);
				if (net_status == CM_NETWORK_STATUS_OUT_OF_SERVICE || net_status == CM_NETWORK_STATUS_EMERGENCY_ONLY) {
					err("No in-service slot exist");
					return -1;
				}
			}
		}
	}

	return 0;
}

int _callmgr_core_process_dial(callmgr_core_data_t *core_data, const char *number,
		callmgr_call_type_e call_type, callmgr_sim_slot_type_e sim_slot, int disable_fm,
		gboolean is_emergency_contact, gboolean is_sat_originated_call)
{
	cm_telephony_call_data_t *call = NULL;
	int ret = -1;
	int call_cnt = 0;
	gboolean is_mo_possible = FALSE;
	gboolean is_emergency_call = FALSE;
	gboolean is_incall_ss = FALSE;
	gboolean is_ss = FALSE;
	gboolean is_number_valid = FALSE;
	gboolean is_pwlock = FALSE;
	gboolean is_security_lock = FALSE;
	gboolean is_ui_visible = FALSE;
	cm_telepony_sim_slot_type_e active_sim = CM_TELEPHONY_SIM_UNKNOWN;

	int ecc_category = -1;
	callmgr_call_error_cause_e err_cause = CALL_ERR_CAUSE_UNKNOWN_E;
	char *number_after_removal = NULL;
	char *extracted_call_num = NULL;
	char *extracted_dtmf_num = NULL;
	dbg("_callmgr_core_process_dial()");
	CM_RETURN_VAL_IF_FAIL(number, -1);
	CM_RETURN_VAL_IF_FAIL(core_data, -1);

	_callmgr_util_extract_call_number(number, &extracted_call_num, &extracted_dtmf_num);
	_callmgr_telephony_check_ecc_number(core_data->telephony_handle, extracted_call_num, &is_emergency_call,&ecc_category);
	CM_SAFE_FREE(extracted_call_num);
	CM_SAFE_FREE(extracted_dtmf_num);

	_callmgr_vconf_is_pwlock(&is_pwlock);
	_callmgr_vconf_is_security_lock(&is_security_lock);
	_callmgr_vconf_is_ui_visible(&is_ui_visible);
	if (is_pwlock || is_security_lock) {
		if (is_ui_visible) {
			warn("Callback case. Allow this operation!!");
		} else if (!is_emergency_call && !is_emergency_contact) {
			err("not emergency call or emergency contact number");
			_callmgr_util_launch_popup(CALL_POPUP_CALL_ERR, CALL_ERR_IS_NOT_EMERGENCY, number, 0, NULL, NULL);
			return -1;
		}
	}

	/* Get call count */
	if (_callmgr_telephony_get_call_count(core_data->telephony_handle, &call_cnt) < 0) {
		err("get call count err");
	}
	if (call_cnt > 0) {
		/* Check incall SS */
		if (_callmgr_util_is_incall_ss_string(number, &is_incall_ss) < 0) {
			err("is incall ss err");
		}

		if (is_incall_ss) {
			info("incall SS");
			if (_callmgr_telephony_process_incall_ss(core_data->telephony_handle, number) < 0) {
				/* TODO : Launch cm popup to display error */
				_callmgr_dbus_send_dial_status(core_data, CALL_MANAGER_DIAL_FAIL);
				return -1;
			}
			_callmgr_dbus_send_dial_status(core_data, CALL_MANAGER_DIAL_SUCCESS);
			return 0;
		}
	}
	else {
		if (!is_emergency_call) {
			/*Set active SIM slot to support multi SIM*/
			if (sim_slot == CALL_MANAGER_SIM_SLOT_DEFAULT_E) {
				ret = __callmgr_core_select_sim_slot(core_data, number, call_type, is_emergency_contact);
				if (ret ==-2) {	// this means we request to launch SIM selection popup, so just return 0(SUCCESS).
					return 0;
				}
			}
			else {
				if (sim_slot == CALL_MANAGER_SIM_SLOT_1_E) {
					_callmgr_telephony_set_active_sim_slot(core_data->telephony_handle, CM_TELEPHONY_SIM_1);
				}
				else if (sim_slot == CALL_MANAGER_SIM_SLOT_2_E) {
					_callmgr_telephony_set_active_sim_slot(core_data->telephony_handle, CM_TELEPHONY_SIM_2);
				}
				else {
					err("unhandled sim slot[%d]", sim_slot);
				}
			}
		}
	}
	_callmgr_telephony_get_active_sim_slot(core_data->telephony_handle, &active_sim);
	_callmgr_telephony_is_ss_string(core_data->telephony_handle, active_sim, number, &is_ss);

	if (is_ss) {
		info("SS string");
		_callmgr_util_launch_ciss(number, active_sim);
		_callmgr_dbus_send_dial_status(core_data, CALL_MANAGER_DIAL_FAIL_SS);
		return 0;
	}

	is_mo_possible = __callmgr_core_check_is_mocall_possible(core_data, is_emergency_call, call_type, &err_cause);
	if (is_mo_possible == FALSE) {
		if (err_cause == CALL_ERR_CAUSE_FLIGHT_MODE_E) {
			/* Check if already a MO Invalid handle calldata exists */
			ret = _callmgr_telephony_get_call_by_call_id(core_data->telephony_handle, NO_CALL_HANDLE, &call);
			if ((ret == 0) && (call != NULL)) {
				err(" already a MO request is under process, hence return error ");
				return -1;
			}

			if (disable_fm == 1 || is_emergency_call) {
				_callmgr_util_launch_popup(CALL_POPUP_FLIGHT_MODE_DISABLING, call_type, number, active_sim, NULL, NULL);
				if (_callmgr_telephony_disable_flight_mode(core_data->telephony_handle) < 0) {
					err("Fail to disable flight mode");
					_callmgr_util_launch_popup(CALL_POPUP_CALL_ERR, CALL_ERR_FAILED, NULL, 0, NULL, NULL);
					_callmgr_dbus_send_dial_status(core_data, CALL_MANAGER_DIAL_FAIL_FLIGHT_MODE);
					return -1;
				}
			}
			else {
				_callmgr_util_launch_popup(CALL_POPUP_FLIGHT_MODE, call_type, number, active_sim, __callmgr_core_popup_result_cb, core_data);
				return 0;
			}
		}
		else if (err_cause == CALL_ERR_CAUSE_NETWORK_2G_ONLY_E) {
			_callmgr_util_launch_popup(CALL_POPUP_OUT_OF_3G_TRY_VOICE_CALL, CALL_TYPE_VOICE_E, number, active_sim, __callmgr_core_popup_result_cb, core_data);
			return 0;
		}
		else {
			err("Couldn't make a call.");
			__callmgr_core_launch_error_popup(err_cause, number);
			return -1;	// TODO: return proper error cause
		}
	}

	_callmgr_util_extract_call_number(number, &extracted_call_num, &extracted_dtmf_num);
	_callmgr_telephony_is_number_valid(extracted_call_num, &is_number_valid);
	if (is_number_valid == FALSE) {
		__callmgr_core_launch_error_popup(CALL_ERR_CAUSE_WRONG_NUMBER_E, extracted_call_num);
		CM_SAFE_FREE(extracted_call_num);
		CM_SAFE_FREE(extracted_dtmf_num);
		return -1;
	}
	_callmgr_util_remove_invalid_chars_from_call_number(number, &number_after_removal);

	if (is_emergency_call) {
		warn("Adding new emergency call data...");
		ret = _callmgr_telephony_call_new(core_data->telephony_handle, CM_TEL_CALL_TYPE_E911,
				CM_TEL_CALL_DIRECTION_MO, number_after_removal, &call);
		_callmgr_telephony_set_ecc_category(call, ecc_category);
	}
	else {
		/*ToDo: Should know the CallDomain and decide on the Call as per domain*/
		if (call_type == CALL_TYPE_VIDEO_E) {
			warn("Adding new call data...");
			ret = _callmgr_telephony_call_new(core_data->telephony_handle, CM_TEL_CALL_TYPE_CS_VIDEO,
					CM_TEL_CALL_DIRECTION_MO, number_after_removal, &call);
		} else {
			warn("Adding new call data...");
			ret = _callmgr_telephony_call_new(core_data->telephony_handle, CM_TEL_CALL_TYPE_CS_VOICE,
					CM_TEL_CALL_DIRECTION_MO, number_after_removal, &call);
		}
	}
	g_free(number_after_removal);
	CM_RETURN_VAL_IF_FAIL(!ret, -1);

	if (is_sat_originated_call)
		_callmgr_telephony_set_sat_originated_call_flag(call, TRUE);

	/* We request dial after disabling flight mode */
	if (err_cause != CALL_ERR_CAUSE_FLIGHT_MODE_E) {
		ret = _callmgr_telephony_dial(core_data->telephony_handle, call);
		if (ret < 0) {
			err("_callmgr_telephony_dial get failed");
			_callmgr_telephony_call_delete(core_data->telephony_handle, NO_CALL_HANDLE);
			return -1;
		}
	}
	return 0;
}

int _callmgr_core_process_end_call(callmgr_core_data_t *core_data, unsigned int call_id, int release_type)
{
	int ret = -1;
	dbg("callid[%d], release_type[%d]");
	CM_RETURN_VAL_IF_FAIL(core_data, -1);

	switch (release_type) {
		case CM_TEL_CALL_RELEASE_TYPE_BY_CALL_HANDLE:
			break;
		case CM_TEL_CALL_RELEASE_TYPE_ALL_CALLS:
			if (core_data->active_dial) {
				call_id = core_data->active_dial->call_id;
			}
			else if (core_data->held) {
				call_id = core_data->held->call_id;
			}
			else if (core_data->incom) {
				call_id = core_data->incom->call_id;
			}
			break;
		case CM_TEL_CALL_RELEASE_TYPE_ALL_HOLD_CALLS:
			if (core_data->held) {
				call_id = core_data->held->call_id;
			}
			break;
		case CM_TEL_CALL_RELEASE_TYPE_ALL_ACTIVE_CALLS:
			if (core_data->active_dial) {
				call_id = core_data->active_dial->call_id;
			}
			break;
		default:
			err("wrong release_type");
			return -1;
	}

	ret = _callmgr_telephony_end_call(core_data->telephony_handle, call_id, release_type);
	if (ret < 0) {
		err("_callmgr_telephony_end_call() get failed");
		return -1;
	}

	return 0;
}

int _callmgr_core_process_reject_call(callmgr_core_data_t *core_data)
{
	int ret = -1;
	dbg("_callmgr_core_process_reject_call()");

	__callmgr_core_cancel_auto_answer(core_data);

	if (core_data->incom){
		/* Reject incoming call */
		info("Reject incoming call");
		__callmgr_core_stop_incom_noti(core_data);
		ret = _callmgr_telephony_reject_call(core_data->telephony_handle);
		if (ret < 0) {
			err("_callmgr_telephony_reject_call() get failed");
			return -1;
		}
		_callmgr_ct_set_log_reject_type(core_data->incom->call_id, CM_CT_PLOG_REJECT_TYPE_NORMAL_E);
	}
	else{
		err("incom call data is NULL");
		return -1;
	}
	return 0;
}


int _callmgr_core_process_swap_call(callmgr_core_data_t *core_data)
{
	int ret = -1;
	dbg("_callmgr_core_process_swap_call()");
	CM_RETURN_VAL_IF_FAIL(core_data, -1);

	ret = _callmgr_telephony_swap_call(core_data->telephony_handle);
	if (ret < 0) {
		err("_callmgr_telephony_swap_call() get failed");
		return -1;
	}
	return 0;
}

int _callmgr_core_process_hold_call(callmgr_core_data_t *core_data)
{
	int ret = -1;
	dbg("_callmgr_core_process_hold_call()");
	CM_RETURN_VAL_IF_FAIL(core_data, -1);

	ret = _callmgr_telephony_hold_call(core_data->telephony_handle);
	if (ret < 0) {
		err("_callmgr_core_process_hold_call() get failed");
		return -1;
	}
	return 0;
}

int _callmgr_core_process_unhold_call(callmgr_core_data_t *core_data)
{
	int ret = -1;
	dbg("_callmgr_core_process_unhold_call()");
	CM_RETURN_VAL_IF_FAIL(core_data, -1);

	ret = _callmgr_telephony_active_call(core_data->telephony_handle);
	if (ret < 0) {
		err("_callmgr_telephony_active_call() get failed");
		return -1;
	}
	return 0;
}


int _callmgr_core_process_join_call(callmgr_core_data_t *core_data)
{
	int ret = -1;
	dbg("_callmgr_core_process_join_call()");
	CM_RETURN_VAL_IF_FAIL(core_data, -1);

	ret = _callmgr_telephony_join_call(core_data->telephony_handle);
	if (ret < 0) {
		err("_callmgr_telephony_join_call() get failed");
		return -1;
	}
	return 0;
}

int _callmgr_core_process_split_call(callmgr_core_data_t *core_data, unsigned int call_id)
{
	int ret = -1;
	dbg("_callmgr_core_process_split_call()");
	CM_RETURN_VAL_IF_FAIL(core_data, -1);

	ret = _callmgr_telephony_split_call(core_data->telephony_handle, call_id);
	if (ret < 0) {
		err("_callmgr_telephony_split_call() get failed");
		return -1;
	}
	return 0;
}

int _callmgr_core_process_transfer_call(callmgr_core_data_t *core_data)
{
	int ret = -1;
	cm_telephony_call_data_t *active_call_data = NULL;
	dbg("_callmgr_core_process_transfer_call()");
	CM_RETURN_VAL_IF_FAIL(core_data, -1);

	ret = _callmgr_telephony_get_call_by_state(core_data->telephony_handle, CM_TEL_CALL_STATE_ACTIVE, &active_call_data);
	if (ret < 0) {
		err("_callmgr_core_process_transfer_call() failed to fetch active call data");
		return -1;
	}

	ret = _callmgr_telephony_transfer_call(core_data->telephony_handle, active_call_data);
	if (ret < 0) {
		err("_callmgr_telephony_transfer_call() get failed");
		return -1;
	}
	return 0;
}

int _callmgr_core_process_answer_call(callmgr_core_data_t *core_data, int ans_type)
{
	int ret = -1;
	dbg("_callmgr_core_process_answer_call()");
	CM_RETURN_VAL_IF_FAIL(core_data, -1);

	ret = _callmgr_telephony_answer_call(core_data->telephony_handle, ans_type);

	if (ret < 0) {
		err("_callmgr_telephony_answer_call() failed");
	}

	__callmgr_core_cancel_auto_answer(core_data);
	__callmgr_core_stop_incom_noti(core_data);

	return ret;
}

int _callmgr_core_process_spk_on(callmgr_core_data_t *core_data)
{
	callmgr_audio_device_e cur_route = CALLMGR_AUDIO_DEVICE_RECEIVER_E;
	dbg(">>");
	CM_RETURN_VAL_IF_FAIL(core_data, -1);

	if (_callmgr_audio_get_active_device(core_data->audio_handle, &cur_route) < 0) {
		err("_callmgr_audio_get_active_device() failed");
		return -1;
	}
	if (cur_route == CALLMGR_AUDIO_DEVICE_SPEAKER_E) {
		warn("Already SPK");
		return -1;
	}

	_callmgr_bt_close_sco(core_data->bt_handle);

	if (_callmgr_audio_set_audio_route(core_data->audio_handle, CALLMGR_AUDIO_ROUTE_SPEAKER_E) < 0) {
		err("_callmgr_audio_set_audio_route() failed");
		return -1;
	}

	return 0;
}

int _callmgr_core_process_spk_off(callmgr_core_data_t *core_data)
{
	callmgr_audio_device_e cur_route = CALLMGR_AUDIO_DEVICE_RECEIVER_E;
	dbg(">>");
	CM_RETURN_VAL_IF_FAIL(core_data, -1);

	if (_callmgr_audio_get_active_device(core_data->audio_handle, &cur_route) < 0) {
		err("_callmgr_audio_get_active_device() failed");
		return -1;
	}
	if (cur_route != CALLMGR_AUDIO_DEVICE_SPEAKER_E) {
		warn("Not SPK");
		return -1;
	}

	if (_callmgr_audio_set_audio_route(core_data->audio_handle, CALLMGR_AUDIO_ROUTE_RECEIVER_EARJACK_E) < 0) {
		err("_callmgr_audio_set_audio_route() failed");
		return -1;
	}

	return 0;
}

int _callmgr_core_process_bluetooth_on(callmgr_core_data_t *core_data)
{
	dbg(">>");
	CM_RETURN_VAL_IF_FAIL(core_data, -1);
	int ret = -1;
	ret = _callmgr_bt_open_sco(core_data->bt_handle);
	if (ret < 0) {
		err("_callmgr_bt_open_sco() get failed");
	}

	return ret;
}

int _callmgr_core_process_bluetooth_off(callmgr_core_data_t *core_data)
{
	dbg(">>");
	CM_RETURN_VAL_IF_FAIL(core_data, -1);
	int ret = -1;
	ret = _callmgr_bt_close_sco(core_data->bt_handle);
	if (ret < 0) {
		err("_callmgr_bt_close_sco() get failed");
	}

	return ret;
}
int _callmgr_core_process_record_start(callmgr_core_data_t *core_data, const char *call_num, gboolean is_answering_message)
{
	cm_telephony_call_data_t *active_call = NULL;
	char *call_name = NULL;
	int ret = -1;
	unsigned int call_id = -1;
	CM_RETURN_VAL_IF_FAIL(core_data, -1);
	dbg(">>");

	_callmgr_telephony_get_call_by_state(core_data->telephony_handle, CM_TEL_CALL_STATE_ACTIVE, &active_call);
	if (NULL == active_call) {
		err("No active call exists");
		return -1;
	}
	_callmgr_telephony_get_call_id(active_call, &call_id);
	_callmgr_ct_get_call_name(call_id, &call_name);
	dbg("The name gotten for callmgr_vr is [%s],", call_name);
	ret = _callmgr_vr_start_record(core_data->vr_handle, call_num, call_name, is_answering_message);
	g_free(call_name);
	if(ret < 0) {
		err("_callmgr_vr_start_record() failed");
		return -1;
	}
	return 0;
}

int _callmgr_core_process_record_stop(callmgr_core_data_t *core_data)
{
	int ret = -1;
	CM_RETURN_VAL_IF_FAIL(core_data, -1);

	dbg(">>");
	ret = _callmgr_vr_stop_record(core_data->vr_handle);
	if(ret < 0) {
		err("_callmgr_vr_stop_record() failed");
		return -1;
	}

	return 0;
}

int _callmgr_core_process_set_extra_vol(callmgr_core_data_t *core_data, gboolean is_extra_vol)
{
	dbg(">>");
	callmgr_audio_device_e cur_route = CALLMGR_AUDIO_DEVICE_RECEIVER_E;
	CM_RETURN_VAL_IF_FAIL(core_data, -1);

	if (_callmgr_audio_set_extra_vol(core_data->audio_handle, is_extra_vol) < 0) {
		err("_callmgr_audio_set_extra_vol() failed");
		return -1;
	}

	_callmgr_audio_get_active_device(core_data->audio_handle, &cur_route);
	if (__callmgr_core_set_telephony_audio_route(core_data, cur_route) < 0) {
		err("__callmgr_core_set_telephony_audio_route() failed");
		return -1;
	}

	return 0;
}

int _callmgr_core_process_set_noise_reduction(callmgr_core_data_t *core_data, gboolean is_noise_reduction)
{
	dbg(">>");
	CM_RETURN_VAL_IF_FAIL(core_data, -1);

	if (_callmgr_audio_set_noise_reduction(core_data->audio_handle, is_noise_reduction) < 0) {
		err("_callmgr_audio_set_noise_reduction() failed");
		return -1;
	}
#ifdef SUPPORT_NOISE_REDUCTION
	if (_callmgr_telephony_set_noise_reduction(core_data->telephony_handle, is_noise_reduction) < 0) {
		err("_callmgr_telephony_set_noise_reduction() failed");
		return -1;
	}
#endif
	return 0;
}

int _callmgr_core_process_set_mute_state(callmgr_core_data_t *core_data, gboolean is_mute_state)
{
	cm_telephony_call_data_t *active_call = NULL;
	cm_telephony_call_type_e call_type =CM_TEL_CALL_TYPE_INVALID ;
	callmgr_call_type_e cm_call_type = CALL_TYPE_INVALID_E;
	dbg(">>");
	CM_RETURN_VAL_IF_FAIL(core_data, -1);

	_callmgr_telephony_get_call_by_state(core_data->telephony_handle, CM_TEL_CALL_STATE_ACTIVE, &active_call);
	CM_RETURN_VAL_IF_FAIL(active_call, -1);
	_callmgr_telephony_get_call_type(active_call, &call_type);
	__callmgr_core_convert_tel_call_type(call_type, &cm_call_type);

	if (cm_call_type == CALL_TYPE_VOICE_E) {
		if (_callmgr_audio_set_audio_tx_mute(core_data->audio_handle, is_mute_state) < 0 ) {
			err("_callmgr_audio_set_audio_tx_mute() failed");
			return -1;
		}

		if (_callmgr_telephony_set_audio_tx_mute(core_data->telephony_handle, is_mute_state) < 0) {
			err("_callmgr_telephony_set_audio_tx_mute() failed");
			return -1;
		}
	}
	core_data->is_mute_on = is_mute_state;
	_callmgr_dbus_send_mute_status(core_data, (int)is_mute_state);
	return 0;
}

int _callmgr_core_process_dtmf_resp(callmgr_core_data_t *core_data, callmgr_dtmf_resp_type_e resp_type)
{
	cm_telephony_call_data_t *active_call = NULL;
	CM_RETURN_VAL_IF_FAIL(core_data, -1);

	_callmgr_telephony_get_call_by_state(core_data->telephony_handle, CM_TEL_CALL_STATE_ACTIVE, &active_call);
	CM_RETURN_VAL_IF_FAIL(active_call, -1);

	if (resp_type == CALL_DTMF_RESP_CONTINUE_E) {
		__callmgr_core_process_dtmf_number(core_data);
	}
	else {		//CALL_DTMF_RESP_CANCEL_E
		_callmgr_telephony_set_dtmf_number(active_call, NULL);
	}

	return 0;
}

int _callmgr_core_process_stop_alert(callmgr_core_data_t *core_data)
{
	CM_RETURN_VAL_IF_FAIL(core_data, -1);
	__callmgr_core_stop_incom_noti(core_data);
	return 0;
}

int _callmgr_core_process_start_alert(callmgr_core_data_t *core_data)
{
	CM_RETURN_VAL_IF_FAIL(core_data, -1);
	__callmgr_core_start_incom_noti(core_data);
	return 0;
}


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

#include <ITapiCall.h>
#include <ITapiSim.h>
#include <ITapiModem.h>
#include <ITapiNetwork.h>
#include <ITapiSat.h>

#include <vconf.h>
#include <assert.h>
#include <sys/sysinfo.h>
#include <stdlib.h>

#include "callmgr-telephony.h"
#include "callmgr-log.h"
#include "callmgr-util.h"
#include "callmgr-vconf.h"

#define __FLIGHT_MODE_TIMER 5000 /* 5sec*/

#define CALLMGR_CLI_SHOW_ID "*31#"
#define CALLMGR_CLI_HIDE_ID "#31#"

#define IS1CHARUSSDDIGIT(X) ((X >= '1') && (X <= '9'))
#define ISECCDIGIT(X) ((X >= '2') && (X <= '6'))
#define ISUSSDDIGIT(X) ((X >= '2') && (X <= '9'))

#define IS_DIGIT(value)		((value) >= '0' && (value) <= '9')

static int g_disable_fm_cnt;

struct __cm_tel_call_data {
	unsigned int call_id;							/**< Unique call id*/
	cm_telephony_call_direction_e call_direction;						/**< 0 : MO, 1 : MT*/
	char *call_number;							/**< call number*/
	char *dtmf_number;
	char *calling_name;						/**< Calling part name */
	cm_telephony_call_type_e call_type;			/**< Specifies type of call (voice, data, emergency) */
	cm_telephony_call_state_e call_state;		/**< Current Call state */
	gboolean is_sat_originated_call;
	gboolean is_conference;						/**< Whether Call is in Conference or not*/
	int ecc_category;			/**< emergency category(seethe TelSimEccEmergencyServiceInfo_t or TelCallEmergencyCategory_t)*/
	long start_time;	/**< start time of the Call*/
	cm_telephony_end_cause_type_e end_cause;		/* End cause of ended call*/
	cm_telephony_name_mode_e name_mode;

	gboolean retreive_flag;
};

struct __cm_tel_sat_data {
	cm_telephony_sat_event_type_e event_type;

	TelSatSetupCallIndCallData_t *setup_call;
	TelSatSendDtmfIndDtmfData_t *send_dtmf;
	TelSatCallCtrlIndData_t *call_control_result;
};

struct __modem_info {
	gboolean is_no_sim;
	gboolean is_sim_initialized;
	gboolean is_phone_initialized;
	gboolean is_cs_available;
	cm_telephony_card_type_e card_type;				/**< SIM Card Type */
	cm_network_status_e network_status;
	unsigned long net_mcc;
	unsigned long net_mnc;
	unsigned long sim_mcc;
	unsigned long sim_mnc;
	TelSimEccList_t sim_ecc_list;
	struct __cm_tel_sat_data sat_data;

	GSList *call_list;			/* Call List */
	cm_telephony_event_type_e wait_event;		/* Pending event which is not published to callmgr-core*/
	GSList *wait_call_list;		/* Call list which is to be updated when the event comes from Telephony */
};

struct __callmgr_telephony {
	TapiHandle *multi_handles[CALLMGR_TAPI_HANDLE_MAX+1];			/* Handles for Dual SIM*/
	int modem_cnt;			/* the number of modem*/
	struct __modem_info modem_info[CALLMGR_TAPI_HANDLE_MAX+1];			/* the information for each tapi handle*/
	cm_telepony_sim_slot_type_e active_sim_slot;					/* Active SIM slot number */
	int disable_fm_timer;
	gboolean ans_requesting;	/* Answer request is done or not */
	cm_telephony_answer_request_type_e end_active_after_held_end; /*End Active Call after held call end */

	telephony_event_cb cb_fn;
	void *user_data;
};

/*Global Variable Declerations */
static const char *g_call_event_list[] = {
	/*Events For Voice Call*/
	TAPI_NOTI_VOICE_CALL_STATUS_IDLE,
	TAPI_NOTI_VOICE_CALL_STATUS_ACTIVE,
	TAPI_NOTI_VOICE_CALL_STATUS_HELD,
	TAPI_NOTI_VOICE_CALL_STATUS_DIALING,
	TAPI_NOTI_VOICE_CALL_STATUS_ALERT,
	TAPI_NOTI_VOICE_CALL_STATUS_INCOMING,

	/*Events For Video Call*/
	TAPI_NOTI_VIDEO_CALL_STATUS_IDLE,
	TAPI_NOTI_VIDEO_CALL_STATUS_ACTIVE,
	TAPI_NOTI_VIDEO_CALL_STATUS_DIALING,
	TAPI_NOTI_VIDEO_CALL_STATUS_ALERT,
	TAPI_NOTI_VIDEO_CALL_STATUS_INCOMING,
};

static const char *g_info_event_list[] = {
	TAPI_NOTI_CALL_INFO_WAITING,
	TAPI_NOTI_CALL_INFO_FORWARDED,
	TAPI_NOTI_CALL_INFO_BARRED_INCOMING,
	TAPI_NOTI_CALL_INFO_BARRED_OUTGOING,
	TAPI_NOTI_CALL_INFO_DEFLECTED,
	TAPI_NOTI_CALL_INFO_CLIR_SUPPRESSION_REJECT,
	TAPI_NOTI_CALL_INFO_FORWARD_UNCONDITIONAL,
	TAPI_NOTI_CALL_INFO_FORWARD_CONDITIONAL,
	TAPI_NOTI_CALL_INFO_FORWARDED_CALL,
	TAPI_NOTI_CALL_INFO_DEFLECTED_CALL,
	TAPI_NOTI_CALL_INFO_TRANSFERED_CALL,
	TAPI_NOTI_CALL_INFO_HELD,
	TAPI_NOTI_CALL_INFO_ACTIVE,
	TAPI_NOTI_CALL_INFO_JOINED,
	TAPI_NOTI_CALL_INFO_TRANSFER_ALERT,
};

static const char *g_sound_event_list[] = {
	TAPI_NOTI_CALL_SOUND_WBAMR,
	TAPI_NOTI_CALL_SOUND_CLOCK_STATUS,
	TAPI_NOTI_CALL_SOUND_RINGBACK_TONE,
};

static const char *g_network_event_list[] = {
	TAPI_PROP_NETWORK_PLMN,
	TAPI_PROP_NETWORK_SERVICE_TYPE,
	TAPI_PROP_NETWORK_CIRCUIT_STATUS,
};

static const char *g_sat_event_list[] = {
	TAPI_NOTI_SAT_SEND_DTMF,
	TAPI_NOTI_SAT_SETUP_CALL,
	TAPI_NOTI_SAT_CALL_CONTROL_RESULT,
};

static int __callmgr_telephony_set_call_id(cm_telephony_call_data_t *call, unsigned int call_id);
static int __callmgr_telephony_set_call_state(cm_telephony_call_data_t *call, cm_telephony_call_state_e state);
static int __callmgr_telephony_set_conference_call_state(cm_telephony_call_data_t *call, gboolean bConferenceState);
static int __callmgr_telephony_set_name_mode(cm_telephony_call_data_t *call, cm_telephony_name_mode_e name_mode);
static int __callmgr_telephony_set_start_time(cm_telephony_call_data_t *call);
static int __callmgr_telephony_get_all_call_list(callmgr_telephony_t telephony_handle);
static int __callmgr_telephony_set_retreive_flag(cm_telephony_call_data_t *call, gboolean is_set);
static int __callmgr_telephony_get_call_to_be_retreived(callmgr_telephony_t telephony_handle, cm_telephony_call_data_t **call_data_out);
static int __callmgr_telephony_set_end_cause(cm_telephony_call_data_t *call, cm_telephony_end_cause_type_e cause);
static int __callmgr_telephony_get_end_cause_type(TelCallCause_t call_cause, TelTapiEndCause_t cause, cm_telephony_end_cause_type_e *end_cause_type);
static int __callmgr_telephony_end_all_call_by_state(callmgr_telephony_t telephony_handle, cm_telephony_call_state_e state);

static int __callmgr_telephony_create_wait_call_list(callmgr_telephony_t telephony_handle, cm_telephony_event_type_e event_type, cm_telephony_call_data_t *active_call, cm_telephony_call_data_t *held_call)
{
	struct __modem_info *modem_info = NULL;
	GSList *call_list = NULL;
	cm_telephony_call_data_t *call_data = NULL;
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);

	/* Check if wait call list already exist or not. If it exists, it means we are waiting for telephony events to update call data*/
	modem_info = &telephony_handle->modem_info[telephony_handle->active_sim_slot];
	if ((modem_info) && (modem_info->wait_call_list)) {
		err("Already have waiting call list. Ignore this request");
		int i = 0;
		int list_len = g_slist_length(modem_info->wait_call_list);

		err("The wait_event of modem_info = %d", modem_info->wait_event);

		if ((CM_TELEPHONY_EVENT_SWAP == event_type) && (CM_TELEPHONY_EVENT_SWAP == modem_info->wait_event)) {
			dbg("Waiting for swap_event..Request for swap_call...");
			_callmgr_util_launch_popup(CALL_POPUP_TOAST, CALL_POPUP_TOAST_SWAP_CALL, NULL, 0 , NULL, NULL);
		}

		for (i = 0; i < list_len; i++) {
			call_data = (cm_telephony_call_data_t *)g_slist_nth_data(modem_info->wait_call_list, i);
			if (call_data == NULL) {
				continue;
			}
			err("call_id = %d", call_data->call_id);
			err("call_state = %d", call_data->call_state);
		}
		return -1;
	}

	/* Make wait call list */
	switch (event_type) {
		case CM_TELEPHONY_EVENT_ACTIVE:
		{
			CM_RETURN_VAL_IF_FAIL(active_call, -1);
			modem_info->wait_call_list = g_slist_append(modem_info->wait_call_list, active_call);
			break;
		}
		case CM_TELEPHONY_EVENT_HELD:
		{
			CM_RETURN_VAL_IF_FAIL(active_call, -1);
			if (active_call->is_conference) {
				call_list = modem_info->call_list;
				while (call_list) {
					call_data = (cm_telephony_call_data_t*)call_list->data;
					if ((call_data) && (call_data->is_conference)) {
						modem_info->wait_call_list = g_slist_append(modem_info->wait_call_list, call_data);
					}
					call_list = g_slist_next(call_list);
				}
			} else {
				modem_info->wait_call_list = g_slist_append(modem_info->wait_call_list, active_call);
			}
			break;
		}
		case CM_TELEPHONY_EVENT_JOIN:
		{
			CM_RETURN_VAL_IF_FAIL(held_call, -1);
			if (held_call->is_conference) {
				call_list = modem_info->call_list;
				while (call_list) {
					call_data = (cm_telephony_call_data_t*)call_list->data;
					if ((call_data) && (call_data->is_conference)) {
						modem_info->wait_call_list = g_slist_append(modem_info->wait_call_list, call_data);
					}
					call_list = g_slist_next(call_list);
				}
			} else {
				modem_info->wait_call_list = g_slist_append(modem_info->wait_call_list, held_call);
			}
			break;
		}
		case CM_TELEPHONY_EVENT_SPLIT:
		{
			CM_RETURN_VAL_IF_FAIL(active_call, -1);
			call_list = modem_info->call_list;
			while (call_list) {
				call_data = (cm_telephony_call_data_t*)call_list->data;
				if ((call_data) && (call_data->call_id != active_call->call_id)) {
					modem_info->wait_call_list = g_slist_append(modem_info->wait_call_list, call_data);
				}
				call_list = g_slist_next(call_list);
			}
			break;
		}
		case CM_TELEPHONY_EVENT_RETRIEVED:
		{
			CM_RETURN_VAL_IF_FAIL(held_call, -1);
			if (held_call->is_conference) {
				call_list = modem_info->call_list;
				while (call_list) {
					call_data = (cm_telephony_call_data_t*)call_list->data;
					if ((call_data) && (call_data->is_conference)) {
						modem_info->wait_call_list = g_slist_append(modem_info->wait_call_list, call_data);
					}
					call_list = g_slist_next(call_list);
				}
			} else {
				modem_info->wait_call_list = g_slist_append(modem_info->wait_call_list, held_call);
			}
			break;
		}
		case CM_TELEPHONY_EVENT_SWAP:
		{
			/* All call data should be appended to wait list in SWAP case */
			call_list = modem_info->call_list;
			while (call_list) {
				call_data = (cm_telephony_call_data_t*)call_list->data;
				if (call_data) {
					modem_info->wait_call_list = g_slist_append(modem_info->wait_call_list, call_data);
				}
				call_list = g_slist_next(call_list);
			}
			break;
		}
		default:
			err("unhandled event type: %d", event_type);
			return -1;
	}
	modem_info->wait_event = event_type;

	return 0;
}

static int __callmgr_telephony_remove_wait_call(callmgr_telephony_t telephony_handle, cm_telephony_call_data_t *call_data)
{
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(call_data, -1);
	struct __modem_info *modem_info = &telephony_handle->modem_info[telephony_handle->active_sim_slot];
	CM_RETURN_VAL_IF_FAIL(modem_info, -1);

	if (modem_info->wait_call_list) {
		modem_info->wait_call_list = g_slist_remove(modem_info->wait_call_list, call_data);
	}

	return 0;
}

static int __callmgr_telephony_remove_wait_call_list(callmgr_telephony_t telephony_handle)
{
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	struct __modem_info *modem_info = &telephony_handle->modem_info[telephony_handle->active_sim_slot];
	CM_RETURN_VAL_IF_FAIL(modem_info, -1);

	g_slist_free(modem_info->wait_call_list);
	modem_info->wait_call_list = NULL;

	return 0;
}

static int __callmgr_telephony_get_name_mode(char *number, TelCallCliMode_t cli_mode, TelCallNoCliCause_t cli_cause, cm_telephony_name_mode_e *name_mode_out)
{
	dbg("__callmgr_telephony_get_name_mode");
	CM_RETURN_VAL_IF_FAIL(number, -1);
	CM_RETURN_VAL_IF_FAIL(name_mode_out, -1);
	cm_telephony_name_mode_e name_mode = CM_TEL_NAME_MODE_NONE;

	warn("cli mode[%d] cli cause[%d]", cli_mode, cli_cause);
	if (cli_mode == TAPI_CALL_PRES_RESTRICTED) {
		switch (cli_cause) {
		case TAPI_CALL_NO_CLI_CAUSE_REJECTBY_USER:
			name_mode = CM_TEL_NAME_MODE_PRIVATE;
			break;
		case TAPI_CALL_NO_CLI_CAUSE_COINLINE_PAYPHONE:
			name_mode = CM_TEL_NAME_MODE_PAYPHONE;
			break;
		default:
			name_mode = CM_TEL_NAME_MODE_UNKNOWN;
		}
	} else if (cli_mode == TAPI_CALL_PRES_AVAIL) {
		name_mode = CM_TEL_NAME_MODE_NONE;
	} else if (cli_mode == 3 /*Default*/) {
		if (strlen(number) > 0) {
			name_mode = CM_TEL_NAME_MODE_NONE;
		} else {
			name_mode = CM_TEL_NAME_MODE_UNKNOWN;
		}
	} else {
		name_mode = CM_TEL_NAME_MODE_UNKNOWN;
	}

	*name_mode_out = name_mode;
	return 0;
}

static int __callmgr_telephony_check_special_string(char *num, cm_telephony_name_mode_e *name_mode_out)
{
	dbg("__callmgr_telephony_check_special_string");
	CM_RETURN_VAL_IF_FAIL(num, -1);
	CM_RETURN_VAL_IF_FAIL(name_mode_out, -1);
	TelCallCliMode_t cli_mode = TAPI_CALL_NUM_UNAVAIL;
	TelCallNoCliCause_t cli_cause = TAPI_CALL_NO_CLI_CAUSE_NONE;

	if (!g_strcmp0(num, "P")
		|| !g_strcmp0(num, "PRIVATE")
		|| !g_strcmp0(num, "PRIVATE NUMBER")
		|| !g_strcmp0(num, "RES")) {
		warn("P num");
		cli_mode = TAPI_CALL_PRES_RESTRICTED;
		cli_cause = TAPI_CALL_NO_CLI_CAUSE_REJECTBY_USER;
		__callmgr_telephony_get_name_mode("", cli_mode, cli_cause, name_mode_out);
		return 0;
	} else if (!g_strcmp0(num, "UNAVAILABLE")
		|| !g_strcmp0(num, "UNKNOWN")
		|| !g_strcmp0(num, "PRIVATE UNA")
		|| !g_strcmp0(num, "U")) {
		warn("U num");
		cli_mode = TAPI_CALL_NUM_UNAVAIL;
		cli_cause = TAPI_CALL_NO_CLI_CAUSE_UNAVAILABLE;
		__callmgr_telephony_get_name_mode("", cli_mode, cli_cause, name_mode_out);
		return 0;
	}

	return -1;
}

static void __callmgr_telephony_handle_tapi_events(TapiHandle *handle, const char *noti_id, void *data, void *user_data)
{
	callmgr_telephony_t telephony_handle = (callmgr_telephony_t)user_data;
	int ret_val = -1;
	unsigned int call_handle = -1;
	struct __modem_info *modem_info = NULL;
	CM_RETURN_IF_FAIL(telephony_handle);

	info("event_type:[%s], sim_slot[%d]", noti_id, telephony_handle->active_sim_slot);

	modem_info = &telephony_handle->modem_info[telephony_handle->active_sim_slot];
	/* Process TAPI events */
	if ((g_strcmp0(noti_id, TAPI_NOTI_VOICE_CALL_STATUS_IDLE) == 0) || (g_strcmp0(noti_id, TAPI_NOTI_VIDEO_CALL_STATUS_IDLE) == 0)) {
		TelCallStatusIdleNoti_t *callIdleInfo = (TelCallStatusIdleNoti_t *)data;
		cm_telephony_call_data_t *held_call = NULL;
		cm_telephony_call_data_t *call_data = NULL;
		CM_RETURN_IF_FAIL(callIdleInfo);

		_callmgr_telephony_get_call_by_call_id(telephony_handle, callIdleInfo->id, &call_data);
		if (call_data) {
			cm_telephony_end_cause_type_e end_cause_type = CM_TELEPHONY_ENDCAUSE_MAX;
			__callmgr_telephony_get_end_cause_type(TAPI_CAUSE_SUCCESS, callIdleInfo->cause, &end_cause_type);
			__callmgr_telephony_set_end_cause(call_data, end_cause_type);

			if (end_cause_type == CM_TELEPHONY_ENDCAUSE_REJ_SAT_CALL_CTRL) {
				cm_telephony_call_data_t *call = NULL;
				if (_callmgr_telephony_get_sat_originated_call(telephony_handle, &call) == 0) {
					_callmgr_telephony_send_sat_response(telephony_handle,
							CM_TELEPHONY_SAT_EVENT_SETUP_CALL, CM_TELEPHONY_SAT_RESPONSE_ME_CONTROL_PERMANENT_PROBLEM, CM_TELEPHONY_SIM_UNKNOWN);
				}
			}
		}

		telephony_handle->cb_fn(CM_TELEPHONY_EVENT_IDLE, GUINT_TO_POINTER(callIdleInfo->id), telephony_handle->user_data);

		__callmgr_telephony_get_call_to_be_retreived(telephony_handle, &held_call);
		if (held_call) {
			_callmgr_telephony_active_call(telephony_handle);
		}
	} else if ((g_strcmp0(noti_id, TAPI_NOTI_VOICE_CALL_STATUS_ACTIVE) == 0) || (g_strcmp0(noti_id, TAPI_NOTI_VIDEO_CALL_STATUS_ACTIVE) == 0)) {
		cm_telephony_call_data_t *call_data = NULL;
		TelCallStatusActiveNoti_t *callActiveInfo = (TelCallStatusActiveNoti_t *)data;
		CM_RETURN_IF_FAIL(callActiveInfo);
		if (_callmgr_telephony_get_call_by_call_id(telephony_handle, callActiveInfo->id, &call_data) < 0) {
			dbg("No call_data exists for this event...");
			return;
		}
		__callmgr_telephony_set_call_state(call_data, CM_TEL_CALL_STATE_ACTIVE);
		__callmgr_telephony_set_start_time(call_data);
		__callmgr_telephony_remove_wait_call(telephony_handle, call_data);
		__callmgr_telephony_set_retreive_flag(call_data, FALSE);

		/*If all wait calls are updated*/
		if (modem_info->wait_call_list == NULL) {
			__callmgr_telephony_get_all_call_list(telephony_handle);
			telephony_handle->cb_fn(modem_info->wait_event, GUINT_TO_POINTER(callActiveInfo->id), telephony_handle->user_data);
		}
	} else if (g_strcmp0(noti_id, TAPI_NOTI_VOICE_CALL_STATUS_HELD) == 0) {
		cm_telephony_call_data_t *call_data = NULL;
		TelCallStatusHeldNoti_t *callHeldInfo = (TelCallStatusHeldNoti_t *)data;
		CM_RETURN_IF_FAIL(callHeldInfo);
		if (_callmgr_telephony_get_call_by_call_id(telephony_handle, callHeldInfo->id, &call_data) < 0) {
			dbg("No call_data exists for this event...");
			return;
		}
		__callmgr_telephony_set_call_state(call_data, CM_TEL_CALL_STATE_HELD);
		__callmgr_telephony_remove_wait_call(telephony_handle, call_data);

		/*If all wait calls are updated*/
		if (modem_info->wait_call_list == NULL) {
			__callmgr_telephony_get_all_call_list(telephony_handle);
			telephony_handle->cb_fn(modem_info->wait_event, GUINT_TO_POINTER(callHeldInfo->id), telephony_handle->user_data);

			call_data = NULL;
			_callmgr_telephony_get_call_by_call_id(telephony_handle, NO_CALL_HANDLE, &call_data);
			if (call_data) {
				info("Pending call exist.");
				_callmgr_telephony_dial(telephony_handle, call_data);
			}
		}
	} else if ((g_strcmp0(noti_id, TAPI_NOTI_VOICE_CALL_STATUS_DIALING) == 0) || (g_strcmp0(noti_id, TAPI_NOTI_VIDEO_CALL_STATUS_DIALING) == 0)) {
		TelCallStatusDialingNoti_t *callDialingInfo = NULL;
		cm_telephony_call_data_t *call_data = NULL;

		callDialingInfo = (TelCallStatusDialingNoti_t *)data;
		CM_RETURN_IF_FAIL(callDialingInfo);
		if (_callmgr_telephony_get_call_by_call_id(telephony_handle, NO_CALL_HANDLE, &call_data) < 0) {
			dbg("No call_data exists for this event...");
			return;
		}
		__callmgr_telephony_set_call_state(call_data, CM_TEL_CALL_STATE_DIALING);
		__callmgr_telephony_set_call_id(call_data, callDialingInfo->id);
		telephony_handle->cb_fn(CM_TELEPHONY_EVENT_DIALING, GUINT_TO_POINTER(callDialingInfo->id), telephony_handle->user_data);
	} else if ((g_strcmp0(noti_id, TAPI_NOTI_VOICE_CALL_STATUS_ALERT) == 0) || (g_strcmp0(noti_id, TAPI_NOTI_VIDEO_CALL_STATUS_ALERT) == 0)) {
		TelCallStatusAlertNoti_t *callAlertInfo = NULL;
		cm_telephony_call_data_t *call_data = NULL;

		callAlertInfo = (TelCallStatusAlertNoti_t *)data;
		CM_RETURN_IF_FAIL(callAlertInfo);

		if (_callmgr_telephony_get_call_by_call_id(telephony_handle, callAlertInfo->id, &call_data) < 0) {
			dbg("No call_data exists for this event...");
			return;
		}
		__callmgr_telephony_set_call_state(call_data, CM_TEL_CALL_STATE_ALERT);
		telephony_handle->cb_fn(CM_TELEPHONY_EVENT_ALERT, GUINT_TO_POINTER(callAlertInfo->id), telephony_handle->user_data);
	} else if ((g_strcmp0(noti_id, TAPI_NOTI_VOICE_CALL_STATUS_INCOMING) == 0) || (g_strcmp0(noti_id, TAPI_NOTI_VIDEO_CALL_STATUS_INCOMING) == 0)) {
		TelCallIncomingCallInfo_t callIncomingInfo = {0,};
		cm_telephony_call_data_t *call = NULL;
		cm_telephony_call_type_e call_type = CM_TEL_CALL_TYPE_INVALID;
		cm_telephony_name_mode_e name_mode = CM_TEL_NAME_MODE_NONE;
		char *number = NULL;

		if (data != NULL) {
			memcpy(&callIncomingInfo, data, sizeof(TelCallIncomingCallInfo_t));
			call_handle = callIncomingInfo.CallHandle;
		}

		if (telephony_handle->multi_handles[CM_TELEPHONY_SIM_1] == handle) {
			_callmgr_telephony_set_active_sim_slot(telephony_handle, CM_TELEPHONY_SIM_1);
		} else {
			_callmgr_telephony_set_active_sim_slot(telephony_handle, CM_TELEPHONY_SIM_2);
		}
		sec_dbg("calling party number : [%s]", callIncomingInfo.szCallingPartyNumber);
		sec_dbg("calling party name mode : [%d]", callIncomingInfo.CallingNameInfo.NameMode);
		sec_dbg("calling party name : [%s]", callIncomingInfo.CallingNameInfo.szNameData);

		__callmgr_telephony_get_name_mode(callIncomingInfo.szCallingPartyNumber, callIncomingInfo.CliMode, callIncomingInfo.CliCause, &name_mode);
		if (name_mode == CM_TEL_NAME_MODE_NONE) {
			number = callIncomingInfo.szCallingPartyNumber;
		}
		if ((g_strcmp0(noti_id, TAPI_NOTI_VOICE_CALL_STATUS_INCOMING) == 0)) {
			call_type = CM_TEL_CALL_TYPE_CS_VOICE;
		} else {
			call_type = CM_TEL_CALL_TYPE_CS_VIDEO;
		}
		_callmgr_telephony_call_new(telephony_handle, call_type, CM_TEL_CALL_DIRECTION_MT, number, &call);
		CM_RETURN_IF_FAIL(call);
		if (strlen(callIncomingInfo.CallingNameInfo.szNameData) > 0) {
			_callmgr_telephony_set_calling_name(call, callIncomingInfo.CallingNameInfo.szNameData);
		}
		__callmgr_telephony_set_call_state(call, CM_TEL_CALL_STATE_INCOMING);
		__callmgr_telephony_set_call_id(call, call_handle);
		__callmgr_telephony_check_special_string(callIncomingInfo.szCallingPartyNumber, &name_mode);
		__callmgr_telephony_set_name_mode(call, name_mode);
		ret_val = __callmgr_telephony_create_wait_call_list(telephony_handle, CM_TELEPHONY_EVENT_ACTIVE, call, NULL);
		if (ret_val < 0) {
			err("__callmgr_telephony_create_wait_call_list() failed");
			return;
		}
		info("Incoming call[%d]", call_handle);
		telephony_handle->cb_fn(CM_TELEPHONY_EVENT_INCOMING, (void *)call, telephony_handle->user_data);
	} else {
		err("ERROR!! Noti is not defined");
	}

	info("tapi noti(%s) processed done.", noti_id);

	return;
}

static void __callmgr_telephony_handle_info_tapi_events(TapiHandle *handle, const char *noti_id, void *data, void *user_data)
{
	callmgr_telephony_t telephony_handle = (callmgr_telephony_t)user_data;
	CM_RETURN_IF_FAIL(telephony_handle);

	info("event_type:[%s]", noti_id);
	if (g_strcmp0(noti_id, TAPI_NOTI_CALL_INFO_FORWARDED) == 0) {
		telephony_handle->cb_fn(CM_TELEPHONY_EVENT_SS_INFO, (void *)CM_MO_CODE_CALL_FORWARDED, telephony_handle->user_data);
	} else if (g_strcmp0(noti_id, TAPI_NOTI_CALL_INFO_BARRED_INCOMING) == 0) {
		telephony_handle->cb_fn(CM_TELEPHONY_EVENT_SS_INFO, (void *)CM_MO_CODE_INCOMING_CALLS_BARRED, telephony_handle->user_data);
	} else if (g_strcmp0(noti_id, TAPI_NOTI_CALL_INFO_BARRED_OUTGOING) == 0) {
		telephony_handle->cb_fn(CM_TELEPHONY_EVENT_SS_INFO, (void *)CM_MO_CODE_OUTGOING_CALLS_BARRED, telephony_handle->user_data);
	} else if (g_strcmp0(noti_id, TAPI_NOTI_CALL_INFO_DEFLECTED) == 0) {
		telephony_handle->cb_fn(CM_TELEPHONY_EVENT_SS_INFO, (void *)CM_MO_CODE_CALL_DEFLECTED, telephony_handle->user_data);
	} else if (g_strcmp0(noti_id, TAPI_NOTI_CALL_INFO_CLIR_SUPPRESSION_REJECT) == 0) {
		telephony_handle->cb_fn(CM_TELEPHONY_EVENT_SS_INFO, (void *)CM_MO_CODE_CLIR_SUPPRESSION_REJECTED, telephony_handle->user_data);
	} else if (g_strcmp0(noti_id, TAPI_NOTI_CALL_INFO_FORWARD_UNCONDITIONAL) == 0) {
		telephony_handle->cb_fn(CM_TELEPHONY_EVENT_SS_INFO, (void *)CM_MO_CODE_UNCONDITIONAL_CF_ACTIVE, telephony_handle->user_data);
	} else if (g_strcmp0(noti_id, TAPI_NOTI_CALL_INFO_FORWARD_CONDITIONAL) == 0) {
		/* Some customers don't want to display this information, We need to check requirement */
		telephony_handle->cb_fn(CM_TELEPHONY_EVENT_SS_INFO, (void *)CM_MO_CODE_SOME_CF_ACTIVE, telephony_handle->user_data);
	} else if (g_strcmp0(noti_id, TAPI_NOTI_CALL_INFO_FORWARDED_CALL) == 0) {
		telephony_handle->cb_fn(CM_TELEPHONY_EVENT_SS_INFO, (void *)CM_MT_CODE_FORWARDED_CALL, telephony_handle->user_data);
	} else if (g_strcmp0(noti_id, TAPI_NOTI_CALL_INFO_DEFLECTED_CALL) == 0) {
		telephony_handle->cb_fn(CM_TELEPHONY_EVENT_SS_INFO, (void *)CM_MT_CODE_DEFLECTED_CALL, telephony_handle->user_data);
	} else if (g_strcmp0(noti_id, TAPI_NOTI_CALL_INFO_HELD) == 0) {
		telephony_handle->cb_fn(CM_TELEPHONY_EVENT_SS_INFO, (void *)CM_MT_CODE_CALL_ON_HOLD, telephony_handle->user_data);
	} else if (g_strcmp0(noti_id, TAPI_NOTI_CALL_INFO_ACTIVE) == 0) {
		telephony_handle->cb_fn(CM_TELEPHONY_EVENT_SS_INFO, (void *)CM_MT_CODE_CALL_RETRIEVED, telephony_handle->user_data);
	} else if (g_strcmp0(noti_id, TAPI_NOTI_CALL_INFO_JOINED) == 0) {
		telephony_handle->cb_fn(CM_TELEPHONY_EVENT_SS_INFO, (void *)CM_MT_CODE_MULTI_PARTY_CALL, telephony_handle->user_data);
	} else if (g_strcmp0(noti_id, TAPI_NOTI_CALL_INFO_TRANSFERED_CALL) == 0) {
	} else if (g_strcmp0(noti_id, TAPI_NOTI_CALL_INFO_WAITING) == 0) {
		telephony_handle->cb_fn(CM_TELEPHONY_EVENT_SS_INFO, (void *)CM_MO_CODE_CALL_IS_WAITING, telephony_handle->user_data);
	} else if (g_strcmp0(noti_id, TAPI_NOTI_CALL_INFO_REC) == 0) {
		/* ToDo : We need to handle info record (CDMA feature) */
	} else {
		err("ERROR!! Noti is not defined");
	}
}

static void __callmgr_telephony_handle_sound_tapi_events(TapiHandle *handle, const char *noti_id, void *data, void *user_data)
{
	callmgr_telephony_t telephony_handle = (callmgr_telephony_t)user_data;
	CM_RETURN_IF_FAIL(telephony_handle);

	info("event_type:[%s]", noti_id);
	if (g_strcmp0(noti_id, TAPI_NOTI_CALL_SOUND_WBAMR) == 0) {
		dbg("TAPI_NOTI_CALL_SOUND_WBAMR");
		TelCallSoundWbamrNoti_t wbamrInfo = TAPI_CALL_SOUND_WBAMR_STATUS_OFF;
		int wbamr_status = FALSE;

		if (data != NULL) {
			memset(&wbamrInfo, 0, sizeof(TelCallSoundWbamrNoti_t));
			memcpy(&wbamrInfo, data, sizeof(TelCallSoundWbamrNoti_t));
		}

		dbg("WBAMR is %d", wbamrInfo);
		/* Temporary value for wbamr it needs to be updated */
		if (wbamrInfo == TAPI_CALL_SOUND_WBAMR_STATUS_ON
			|| wbamrInfo == TAPI_CALL_SOUND_WBAMR_STATUS_ON_8K) {
			wbamr_status = TRUE;
		}

		telephony_handle->cb_fn(CM_TELEPHONY_EVENT_SOUND_WBAMR, GINT_TO_POINTER(wbamr_status), telephony_handle->user_data);
	} else if (g_strcmp0(noti_id, TAPI_NOTI_CALL_SOUND_CLOCK_STATUS) == 0) {
		dbg("TAPI_NOTI_CALL_SOUND_CLOCK_STATUS");
		/* ToDo : We have to handle this event for 1st MO call */

		telephony_handle->cb_fn(CM_TELEPHONY_EVENT_SOUND_CLOCK_STATUS, NULL, telephony_handle->user_data);
	} else if (g_strcmp0(noti_id, TAPI_NOTI_CALL_SOUND_RINGBACK_TONE) == 0) {
		TelCallSoundRingbackToneNoti_t ringbacktone_info = TAPI_CALL_SOUND_RINGBACK_TONE_END;

		memset(&ringbacktone_info, 0, sizeof(TelCallSoundRingbackToneNoti_t));
		memcpy(&ringbacktone_info, data, sizeof(TelCallSoundRingbackToneNoti_t));

		dbg("TAPI_NOTI_CALL_SOUND_RINGBACK_TONE : %d", ringbacktone_info);

		telephony_handle->cb_fn(CM_TELEPHONY_EVENT_SOUND_RINGBACK_TONE, (void *)ringbacktone_info, telephony_handle->user_data);
	} else {
		dbg("ERROR!! Noti is not defined");
	}
}

static int __callmgr_telephony_check_no_sim_state(callmgr_telephony_t telephony_handle, int slot_id, gboolean *is_no_sim)
{
	int ret = -1;
	int sim_changed = 0;
	TelSimCardStatus_t sim_status = TAPI_SIM_STATUS_UNKNOWN;
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(slot_id < telephony_handle->modem_cnt, -1);
	dbg("__callmgr_telephony_check_no_sim_state()");

	ret = tel_get_sim_init_info(telephony_handle->multi_handles[slot_id], &sim_status, &sim_changed);
	if (TAPI_API_SUCCESS != ret) {
		err("tel_get_sim_init_info() failed with err[%d]", ret);
		*is_no_sim = FALSE;
		return -1;
	} else {
		dbg("card_status = %d", sim_status);
		switch (sim_status) {
		case TAPI_SIM_STATUS_CARD_NOT_PRESENT:
		case TAPI_SIM_STATUS_CARD_REMOVED:
		case TAPI_SIM_STATUS_CARD_ERROR:
		case TAPI_SIM_STATUS_CARD_BLOCKED:
		case TAPI_SIM_STATUS_CARD_CRASHED:
			*is_no_sim = TRUE;
			break;
		default:
			warn("Unknown Card_status[%d]", sim_status);
			*is_no_sim = FALSE;
			break;
		}
	}

	return 0;
}

static int __callmgr_telephony_get_card_type(callmgr_telephony_t telephony_handle, int slot_id, cm_telephony_card_type_e *card_type_out)
{
	int ret = -1;
	cm_telephony_card_type_e cm_card_type = CM_CARD_TYPE_UNKNOWN;
	TelSimCardType_t card_type = TAPI_SIM_CARD_TYPE_UNKNOWN;
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(slot_id < telephony_handle->modem_cnt, -1);
	CM_RETURN_VAL_IF_FAIL(card_type_out, -1);
	dbg("__callmgr_telephony_get_card_type()");

	ret = tel_get_sim_type(telephony_handle->multi_handles[slot_id], &card_type);
	if (TAPI_API_SUCCESS != ret) {
		err("tel_get_sim_type() failed with error[%d]", ret);
		return -1;
	}

	switch (card_type) {
		case TAPI_SIM_CARD_TYPE_GSM:
			cm_card_type = CM_CARD_TYPE_GSM;
			break;
		case TAPI_SIM_CARD_TYPE_USIM:
			cm_card_type = CM_CARD_TYPE_USIM;
			break;
		case TAPI_SIM_CARD_TYPE_RUIM:
		case TAPI_SIM_CARD_TYPE_NVSIM:
		case TAPI_SIM_CARD_TYPE_IMS:
		default:
			cm_card_type = CM_CARD_TYPE_UNKNOWN;
			break;
	}
	dbg("card_type = %d", cm_card_type);
	*card_type_out = cm_card_type;
	return 0;
}

static void __callmgr_telephony_handle_sim_status_events(TapiHandle *handle, const char *noti_id, void *data, void *user_data)
{
	TelSimCardStatus_t *status = data;
	callmgr_telephony_t telephony_handle = (callmgr_telephony_t)user_data;
	CM_RETURN_IF_FAIL(telephony_handle);
	int idx = 0;
	int sim_slot = -1;
	struct __modem_info *modem_info = NULL;

	for (idx = 0; idx < telephony_handle->modem_cnt; idx++) {
		if ((telephony_handle->multi_handles[idx] != NULL) && (telephony_handle->multi_handles[idx] == handle)) {
			sim_slot = idx;
			break;
		}
	}

	if (sim_slot != -1) {
		TapiResult_t tapi_err = TAPI_API_SUCCESS;
		modem_info = &telephony_handle->modem_info[sim_slot];
		if ((*status == TAPI_SIM_STATUS_CARD_NOT_PRESENT) || (*status == TAPI_SIM_STATUS_CARD_REMOVED)
			|| (*status == TAPI_SIM_STATUS_CARD_ERROR) || (*status == TAPI_SIM_STATUS_CARD_BLOCKED)
			|| (*status == TAPI_SIM_STATUS_CARD_CRASHED)) {
			info("SIM slot[%d] is empty", sim_slot);
			modem_info->is_no_sim = TRUE;
			modem_info->is_sim_initialized = FALSE;
		} else {
			modem_info->is_no_sim = FALSE;
			if (*status == TAPI_SIM_STATUS_SIM_INIT_COMPLETED) {
				modem_info->is_sim_initialized = TRUE;
			} else {
				modem_info->is_sim_initialized = FALSE;
			}

			/* Get Card type from Telephony */
			info("SIM slot[%d] state changed. state[%d]", idx, *status);
			__callmgr_telephony_get_card_type(telephony_handle, idx, &(modem_info->card_type));
		}

		if (modem_info->sim_ecc_list.ecc_count == 0) {
			dbg("Fetching tel_get_sim_ecc for SIM slot[%d]", idx);
			memset(&modem_info->sim_ecc_list, 0x00, sizeof(TelSimEccList_t));
			tapi_err = tel_get_sim_ecc(telephony_handle->multi_handles[idx], &modem_info->sim_ecc_list);
			if (TAPI_API_SUCCESS != tapi_err) {
				warn("tel_get_sim_ecc failed, tapi_err=%d", tapi_err);
			}
		} else {
			dbg("Already Fetched tel_get_sim_ecc for SIM slot[%d]", idx);
		}
	}
	return;
}

static void __callmgr_telephony_handle_modem_power_status_events(TapiHandle *handle, const char *noti_id, void *data, void *user_data)
{
	int *status = data;
	callmgr_telephony_t telephony_handle = (callmgr_telephony_t)user_data;
	CM_RETURN_IF_FAIL(telephony_handle);
	int idx = 0;
	int sim_slot = -1;
	struct __modem_info *modem_info = NULL;

	for (idx = 0; idx < telephony_handle->modem_cnt; idx++) {
		if ((telephony_handle->multi_handles[idx] != NULL) && (telephony_handle->multi_handles[idx] == handle)) {
			sim_slot = idx;
			break;
		}
	}

	if (sim_slot != -1) {
		modem_info = &telephony_handle->modem_info[sim_slot];
		if (1 /* offline */ == *status || 2 /* Error */== *status) {
			err("Tapi not initialized");
			modem_info->is_phone_initialized = FALSE;
		} else {
			modem_info->is_phone_initialized = TRUE;
		}
	} else {
		err("Invalid handle");
	}
	return;
}

static void __callmgr_telephony_handle_network_events(TapiHandle *handle, const char *noti_id, void *data, void *user_data)
{
	callmgr_telephony_t telephony_handle = (callmgr_telephony_t)user_data;
	CM_RETURN_IF_FAIL(telephony_handle);
	int slot_id = -1;
	int idx = -1;
	struct __modem_info *modem_info = NULL;
	info("event_type:[%s]", noti_id);

	for (idx = 0; idx < telephony_handle->modem_cnt; idx++) {
		if (telephony_handle->multi_handles[idx] == handle) {
			slot_id = idx;
			break;
		}
	}

	if (slot_id != -1) {
		modem_info = &telephony_handle->modem_info[slot_id];
		if (g_strcmp0(noti_id, TAPI_PROP_NETWORK_PLMN) == 0) {
			char *plmn = (char *)data;
			char mcc_value[4] = {0,};
			char mnc_value[4] = {0,};
			dbg("plmn_value = [%s]", plmn);

			g_strlcpy(mcc_value, plmn, 4);
			g_strlcpy(mnc_value, plmn+3, 4);

			modem_info->net_mcc = (unsigned long)atoi(mcc_value);
			modem_info->net_mnc = (unsigned long)atoi(mnc_value);
		} else if (g_strcmp0(noti_id, TAPI_PROP_NETWORK_SERVICE_TYPE) == 0) {
			TelNetworkServiceType_t *service_type = (TelNetworkServiceType_t *)data;
			info("Service Type: [%d]", *service_type);
			switch (*service_type) {
				case TAPI_NETWORK_SERVICE_TYPE_UNKNOWN:
				case TAPI_NETWORK_SERVICE_TYPE_NO_SERVICE:
				case TAPI_NETWORK_SERVICE_TYPE_SEARCH:
					modem_info->network_status = CM_NETWORK_STATUS_OUT_OF_SERVICE;
					break;
				case TAPI_NETWORK_SERVICE_TYPE_EMERGENCY:
					modem_info->network_status = CM_NETWORK_STATUS_EMERGENCY_ONLY;
					break;
				case TAPI_NETWORK_SERVICE_TYPE_2G:
				case TAPI_NETWORK_SERVICE_TYPE_2_5G:
				case TAPI_NETWORK_SERVICE_TYPE_2_5G_EDGE:
					modem_info->network_status = CM_NETWORK_STATUS_IN_SERVICE_2G;
					break;
				case TAPI_NETWORK_SERVICE_TYPE_3G:
				case TAPI_NETWORK_SERVICE_TYPE_HSDPA:
				case TAPI_NETWORK_SERVICE_TYPE_LTE:
					modem_info->network_status = CM_NETWORK_STATUS_IN_SERVICE_3G;
					break;
				default:
					modem_info->network_status = CM_NETWORK_STATUS_IN_SERVICE_2G; /* Please check default value*/
					break;
			}
		} else if (g_strcmp0(noti_id, TAPI_PROP_NETWORK_CIRCUIT_STATUS) == 0) {
			int *cs_status = (int *)data;
			if (*cs_status == VCONFKEY_TELEPHONY_SVC_CS_ON) {
				modem_info->is_cs_available = TRUE;
			} else {
				modem_info->is_cs_available = FALSE;
			}
		} else {
			err("Invalid noti id[%s]", noti_id);
		}
	} else {
		err("Invalid handle");
	}
	return;
}

static void __callmgr_telephony_handle_sat_events(TapiHandle *handle, const char *noti_id, void *data, void *user_data)
{
	callmgr_telephony_t telephony_handle = (callmgr_telephony_t)user_data;
	CM_RETURN_IF_FAIL(telephony_handle);
	cm_telepony_sim_slot_type_e active_sim_slot = CM_TELEPHONY_SIM_UNKNOWN;
	cm_telepony_sim_slot_type_e request_sim_slot = CM_TELEPHONY_SIM_UNKNOWN;
	struct __cm_tel_sat_data *sat_data = NULL;
	info("event_type:[%s]", noti_id);

	_callmgr_telephony_get_active_sim_slot(telephony_handle, &active_sim_slot);
	if (telephony_handle->multi_handles[CM_TELEPHONY_SIM_1] == handle) {
		request_sim_slot = CM_TELEPHONY_SIM_1;
	} else {
		request_sim_slot = CM_TELEPHONY_SIM_2;
	}

	dbg("active_sim_slot: %d sat_req_for_sim_slot: %d", active_sim_slot, request_sim_slot);

	if(CM_TELEPHONY_SIM_UNKNOWN == active_sim_slot) {
		_callmgr_telephony_set_active_sim_slot(telephony_handle, request_sim_slot);
		active_sim_slot = request_sim_slot;
	}

	sat_data = &telephony_handle->modem_info[request_sim_slot].sat_data;

	if (g_strcmp0(noti_id, TAPI_NOTI_SAT_SETUP_CALL) == 0) {
		TelSatSetupCallIndCallData_t *ind_data = data;

		dbg("cmdId[%d], dispText[%s], callNumber[%s], duration[%d], icon[%d]",
				ind_data->commandId, ind_data->dispText.string, ind_data->callNumber.string,
				ind_data->duration, ind_data->iconId.bIsPresent);

		sat_data->event_type = CM_TELEPHONY_SAT_EVENT_SETUP_CALL;
		if ((sat_data->setup_call = g_malloc0(sizeof(TelSatSetupCallIndCallData_t) + 1)))
			memcpy(sat_data->setup_call, ind_data, sizeof(TelSatSetupCallIndCallData_t));
		else
			err("g_malloc0() failed.");

		if(request_sim_slot == active_sim_slot) {
			telephony_handle->cb_fn(CM_TELEPHONY_EVENT_SAT_SETUP_CALL, NULL, telephony_handle->user_data);
		} else {
			err("SAT request came for wrong sim_slot");
			_callmgr_telephony_send_sat_response(telephony_handle,
					CM_TELEPHONY_SAT_EVENT_SETUP_CALL, CM_TELEPHONY_SAT_RESPONSE_ME_UNABLE_TO_PROCESS_COMMAND, request_sim_slot);
		}

	} else if (g_strcmp0(noti_id, TAPI_NOTI_SAT_SEND_DTMF) == 0) {
		TelSatSendDtmfIndDtmfData_t *ind_data = data;

		dbg("cmdId[%d], bIsHiddenMode[%d], dtmfString[%s]",
				ind_data->commandId, ind_data->bIsHiddenMode, ind_data->dtmfString.string);

		sat_data->event_type = CM_TELEPHONY_SAT_EVENT_SEND_DTMF;
		if ((sat_data->send_dtmf = g_malloc0(sizeof(TelSatSendDtmfIndDtmfData_t) + 1)))
			memcpy(sat_data->send_dtmf, ind_data, sizeof(TelSatSendDtmfIndDtmfData_t));
		else
			err("g_malloc0() failed.");
		if(request_sim_slot == active_sim_slot) {
			telephony_handle->cb_fn(CM_TELEPHONY_EVENT_SAT_SEND_DTMF, (void *)ind_data->dtmfString.string, telephony_handle->user_data);
		} else {
			err("SAT request came for wrong sim_slot");
			_callmgr_telephony_send_sat_response(telephony_handle,
					CM_TELEPHONY_EVENT_SAT_SEND_DTMF, CM_TELEPHONY_SAT_RESPONSE_ME_UNABLE_TO_PROCESS_COMMAND, request_sim_slot);
		}

	} else if (g_strcmp0(noti_id, TAPI_NOTI_SAT_CALL_CONTROL_RESULT) == 0) {
		TelSatCallCtrlIndData_t *ind_data = data;
		gboolean b_ui_update = FALSE;

		dbg("callCtrlCnfType[%d], callCtrlResult[%d], dispData[%s], bIsUserInfoDisplayEnabled[%d]",
				ind_data->callCtrlCnfType, ind_data->callCtrlResult, ind_data->dispData.string,
				ind_data->bIsUserInfoDisplayEnabled);

		sat_data->event_type = CM_TELEPHONY_SAT_EVENT_CALL_CONTROL_RESULT;
		if ((sat_data->call_control_result = g_malloc0(sizeof(TelSatCallCtrlIndData_t) + 1))) {
			memcpy(sat_data->call_control_result, ind_data, sizeof(TelSatCallCtrlIndData_t));
			if (sat_data->call_control_result->callCtrlResult == TAPI_SAT_CALL_CTRL_R_ALLOWED_WITH_MOD)
				b_ui_update = TRUE;
		}
		else
			err("g_malloc0() failed.");

		if(request_sim_slot == active_sim_slot) {
			telephony_handle->cb_fn(CM_TELEPHONY_EVENT_SAT_CALL_CONTROL_RESULT, &b_ui_update, telephony_handle->user_data);
		} else {
			err("SAT request came for wrong sim_slot");
			_callmgr_telephony_send_sat_response(telephony_handle,
					CM_TELEPHONY_EVENT_SAT_CALL_CONTROL_RESULT, TAPI_SAT_CALL_CTRL_R_NOT_ALLOWED, request_sim_slot);
		}
	}

	return;
}

void _callmgr_telephony_send_sat_response(callmgr_telephony_t telephony_handle, cm_telephony_sat_event_type_e event_type,
		cm_telephony_sat_response_type_e resp_type, cm_telepony_sim_slot_type_e active_sim_slot)
{
	struct __cm_tel_sat_data *sat_data = NULL;
	cm_telepony_sim_slot_type_e current_active_sim_slot = CM_TELEPHONY_SIM_UNKNOWN;
	CM_RETURN_IF_FAIL(telephony_handle);

	info("event_type[%d], resp_type[%d]", event_type, resp_type);

	if(CM_TELEPHONY_SIM_UNKNOWN == active_sim_slot) {
		current_active_sim_slot = telephony_handle->active_sim_slot;
	} else {
		current_active_sim_slot = active_sim_slot;
	}
	sat_data = &telephony_handle->modem_info[current_active_sim_slot].sat_data;

	switch (event_type) {
	case CM_TELEPHONY_SAT_EVENT_SETUP_CALL:
		{
			TelSatCallRetInfo_t call_resp_data = {0,};
			TelSatAppsRetInfo_t resp_data = {0,};
			TapiResult_t error_code;
			CM_RETURN_IF_FAIL(sat_data->setup_call);

			memset(&resp_data, 0, sizeof(TelSatAppsRetInfo_t));

			switch (resp_type) {
			case CM_TELEPHONY_SAT_RESPONSE_ME_UNABLE_TO_PROCESS_COMMAND:
				call_resp_data.resp = TAPI_SAT_R_ME_UNABLE_TO_PROCESS_COMMAND;
				call_resp_data.bIsTapiCauseExist = FALSE;
				call_resp_data.tapiCause = TAPI_CAUSE_UNKNOWN;
				call_resp_data.meProblem = TAPI_SAT_ME_PROBLEM_ME_BUSY_ON_CALL;
				call_resp_data.bIsOtherInfoExist = FALSE;
				call_resp_data.permanentCallCtrlProblem = TAPI_SAT_CC_PROBLEM_NO_SPECIFIC_CAUSE;
				break;
			case CM_TELEPHONY_SAT_RESPONSE_NETWORK_UNABLE_TO_PROCESS_COMMAND:
				call_resp_data.resp = TAPI_SAT_R_NETWORK_UNABLE_TO_PROCESS_COMMAND;
				call_resp_data.bIsTapiCauseExist = TRUE;
				call_resp_data.tapiCause = TAPI_CAUSE_BUSY;
				call_resp_data.meProblem = TAPI_SAT_ME_PROBLEM_NO_SERVICE;
				call_resp_data.bIsOtherInfoExist = FALSE;
				call_resp_data.permanentCallCtrlProblem = TAPI_SAT_CC_PROBLEM_NO_SPECIFIC_CAUSE;
				break;
			case CM_TELEPHONY_SAT_RESPONSE_NETWORK_UNABLE_TO_PROCESS_COMMAND_WITHOUT_CAUSE:
				call_resp_data.resp = TAPI_SAT_R_NETWORK_UNABLE_TO_PROCESS_COMMAND;
				call_resp_data.bIsTapiCauseExist = FALSE;
				call_resp_data.tapiCause = TAPI_CAUSE_UNKNOWN;
				call_resp_data.meProblem = TAPI_SAT_ME_PROBLEM_NO_SPECIFIC_CAUSE;
				call_resp_data.bIsOtherInfoExist = FALSE;
				call_resp_data.permanentCallCtrlProblem = TAPI_SAT_CC_PROBLEM_NO_SPECIFIC_CAUSE;
				break;
			case CM_TELEPHONY_SAT_RESPONSE_ME_CONTROL_PERMANENT_PROBLEM:
				call_resp_data.resp = TAPI_SAT_R_INTRCTN_WITH_CC_OR_SMS_CTRL_PRMNT_PRBLM;
				call_resp_data.bIsTapiCauseExist = FALSE;
				call_resp_data.tapiCause = TAPI_CAUSE_UNKNOWN;
				call_resp_data.meProblem = TAPI_SAT_ME_PROBLEM_ACCESS_CONTROL_CLASS_BAR;
				call_resp_data.bIsOtherInfoExist = FALSE;
				call_resp_data.permanentCallCtrlProblem = TAPI_SAT_CC_PROBLEM_ACTION_NOT_ALLOWED;
				break;
			case CM_TELEPHONY_SAT_RESPONSE_ME_CLEAR_DOWN_BEFORE_CONN:
				call_resp_data.resp = TAPI_SAT_R_USER_CLEAR_DOWN_CALL_BEFORE_CONN;
				call_resp_data.bIsTapiCauseExist = FALSE;
				call_resp_data.tapiCause = TAPI_CAUSE_UNKNOWN;
				call_resp_data.meProblem = TAPI_SAT_ME_PROBLEM_ME_BUSY_ON_CALL;
				call_resp_data.bIsOtherInfoExist = FALSE;
				call_resp_data.permanentCallCtrlProblem = TAPI_SAT_CC_PROBLEM_NO_SPECIFIC_CAUSE;
				break;
			case CM_TELEPHONY_SAT_RESPONSE_ME_RET_SUCCESS:
				call_resp_data.resp = TAPI_SAT_R_SUCCESS;
				call_resp_data.bIsTapiCauseExist = TRUE;
				call_resp_data.tapiCause = TAPI_CAUSE_SUCCESS;
				call_resp_data.meProblem = TAPI_SAT_ME_PROBLEM_NO_SPECIFIC_CAUSE;
				call_resp_data.bIsOtherInfoExist = FALSE;
				call_resp_data.permanentCallCtrlProblem = TAPI_SAT_CC_PROBLEM_NO_SPECIFIC_CAUSE;
				break;
			default:
				warn("unhandled resp_type[%d].", resp_type);
				break;
			}
			resp_data.commandType = TAPI_SAT_CMD_TYPE_SETUP_CALL;
			resp_data.commandId = sat_data->setup_call->commandId;
			memcpy(&(resp_data.appsRet.setupCall), &call_resp_data, sizeof(resp_data.appsRet.setupCall));

			error_code = tel_send_sat_app_exec_result(telephony_handle->multi_handles[current_active_sim_slot], &resp_data);
			if (error_code != TAPI_API_SUCCESS) {
				err("tel_send_sat_app_exec_result() is failed[%d].", error_code);
			}

			sat_data->event_type = CM_TELEPHONY_SAT_EVENT_NONE;
			g_free(sat_data->setup_call);
			sat_data->setup_call = NULL;
		}
		break;
	case CM_TELEPHONY_SAT_EVENT_SEND_DTMF:
		{
			TelSatDtmfRetInfo_t dtmf_resp_data = {0,};
			TelSatAppsRetInfo_t resp_data = {0,};
			TapiResult_t error_code;
			CM_RETURN_IF_FAIL(sat_data->send_dtmf);

			memset(&resp_data, 0, sizeof(TelSatAppsRetInfo_t));

			switch (resp_type) {
			case CM_TELEPHONY_SAT_RESPONSE_ME_UNABLE_TO_PROCESS_COMMAND:
				dtmf_resp_data.resp = TAPI_SAT_R_ME_UNABLE_TO_PROCESS_COMMAND;
				break;
			case CM_TELEPHONY_SAT_RESPONSE_NETWORK_UNABLE_TO_PROCESS_COMMAND:
				dtmf_resp_data.resp = TAPI_SAT_R_NETWORK_UNABLE_TO_PROCESS_COMMAND;
				break;
			case CM_TELEPHONY_SAT_RESPONSE_ME_CONTROL_PERMANENT_PROBLEM:
				dtmf_resp_data.resp = TAPI_SAT_R_INTRCTN_WITH_CC_OR_SMS_CTRL_PRMNT_PRBLM;
				break;
			case CM_TELEPHONY_SAT_RESPONSE_ME_CLEAR_DOWN_BEFORE_CONN:
				dtmf_resp_data.resp = TAPI_SAT_R_USER_CLEAR_DOWN_CALL_BEFORE_CONN;
				break;
			case CM_TELEPHONY_SAT_RESPONSE_ME_RET_SUCCESS:
				dtmf_resp_data.resp = TAPI_SAT_R_SUCCESS;
				break;
			default:
				warn("unhandled resp_type[%d].", resp_type);
				break;
			}
			resp_data.commandType = TAPI_SAT_CMD_TYPE_SEND_DTMF;
			resp_data.commandId = sat_data->send_dtmf->commandId;
			memcpy(&(resp_data.appsRet.sendDtmf), &dtmf_resp_data, sizeof(resp_data.appsRet.sendDtmf));

			error_code = tel_send_sat_app_exec_result(telephony_handle->multi_handles[current_active_sim_slot], &resp_data);
			if (error_code != TAPI_API_SUCCESS) {
				err("tel_send_sat_app_exec_result() is failed[%d].", error_code);
			}

			sat_data->event_type = CM_TELEPHONY_SAT_EVENT_NONE;
			g_free(sat_data->send_dtmf);
			sat_data->send_dtmf = NULL;
		}
		break;
	case CM_TELEPHONY_SAT_EVENT_CALL_CONTROL_RESULT:
		{
			cm_telephony_call_data_t *sat_dial_active_call = NULL;
			CM_RETURN_IF_FAIL(sat_data->call_control_result);

			switch(sat_data->call_control_result->callCtrlResult) {
			case TAPI_SAT_CALL_CTRL_R_ALLOWED_NO_MOD:
				break;
			case TAPI_SAT_CALL_CTRL_R_NOT_ALLOWED:
				_callmgr_telephony_send_sat_response(telephony_handle,
						CM_TELEPHONY_SAT_EVENT_SETUP_CALL, CM_TELEPHONY_SAT_RESPONSE_ME_CONTROL_PERMANENT_PROBLEM, current_active_sim_slot);
				break;
			case TAPI_SAT_CALL_CTRL_R_ALLOWED_WITH_MOD:
				/* Fetch the Sat / Dial / Active call data for */
				_callmgr_telephony_get_sat_originated_call(telephony_handle, &sat_dial_active_call);
				if (!sat_dial_active_call)
					_callmgr_telephony_get_call_by_state(telephony_handle, CM_TEL_CALL_STATE_DIALING, &sat_dial_active_call);
				if (!sat_dial_active_call)
					_callmgr_telephony_get_call_by_state(telephony_handle, CM_TEL_CALL_STATE_ACTIVE, &sat_dial_active_call);

				_callmgr_telephony_set_call_number(sat_dial_active_call, sat_data->call_control_result->u.callCtrlCnfCallData.address.string);
				_callmgr_telephony_set_calling_name(sat_dial_active_call, sat_data->call_control_result->dispData.string);
				break;
			default:
				warn("unhandled resp_type[%d].", sat_data->call_control_result->callCtrlResult);
				break;

			}

			sat_data->event_type = CM_TELEPHONY_SAT_EVENT_NONE;
			g_free(sat_data->call_control_result);
			sat_data->call_control_result = NULL;
		}
		break;
	default:
		warn("invalid event_type[%d]", event_type);
		break;
	}

	return;
}


static void __callmgr_telephony_handle_preferred_sim_events(TapiHandle *handle, const char *noti_id, void *data, void *user_data)
{
	TelCallPreferredVoiceSubs_t *preferred_sim = (TelCallPreferredVoiceSubs_t *)data;
	callmgr_telephony_t telephony_handle = (callmgr_telephony_t)user_data;
	CM_RETURN_IF_FAIL(telephony_handle);
	warn("preferred SIM changed to [%d]", *preferred_sim);
	telephony_handle->cb_fn(CM_TELEPHONY_EVENT_PREFERRED_SIM_CHANGED, (void *)*preferred_sim, telephony_handle->user_data);
	return;
}

static int __callmgr_telephony_register_events(callmgr_telephony_t telephony_handle)
{
	int sim_idx = 0;
	int evt_idx = 0;
	TapiResult_t api_err = TAPI_API_SUCCESS;
	int num_event = 0;
	int num_info_event = 0;
	int num_sound_event = 0;
	int num_net_event = 0;
	int num_sat_event = 0;
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);

	num_event = sizeof(g_call_event_list) / sizeof(char *);
	num_info_event = sizeof(g_info_event_list) / sizeof(char *);
	num_sound_event = sizeof(g_sound_event_list) / sizeof(char *);
	num_net_event = sizeof(g_network_event_list) / sizeof(char *);
	num_sat_event = sizeof(g_sat_event_list) / sizeof(char *);

	for (sim_idx = 0; sim_idx < telephony_handle->modem_cnt; sim_idx++) {
		info("Register events for SIM%d", sim_idx);
		for (evt_idx = 0; evt_idx < num_event; evt_idx++) {
			api_err = tel_register_noti_event(telephony_handle->multi_handles[sim_idx], g_call_event_list[evt_idx], __callmgr_telephony_handle_tapi_events, telephony_handle);
			if (api_err != TAPI_API_SUCCESS) {
				err("tel_register_noti_event() failed.. event id:[%s], api_err:[%d]", g_call_event_list[evt_idx], api_err);
			}
		}

		for (evt_idx = 0; evt_idx < num_info_event; evt_idx++) {
			api_err = tel_register_noti_event(telephony_handle->multi_handles[sim_idx], g_info_event_list[evt_idx], __callmgr_telephony_handle_info_tapi_events, telephony_handle);
			if (api_err != TAPI_API_SUCCESS) {
				err("tel_register_noti_event() failed.. event id:[%s], api_err:[%d]", g_info_event_list[evt_idx], api_err);
			}
		}

		for (evt_idx = 0; evt_idx < num_sound_event; evt_idx++) {
			api_err = tel_register_noti_event(telephony_handle->multi_handles[sim_idx], g_sound_event_list[evt_idx], __callmgr_telephony_handle_sound_tapi_events, telephony_handle);
			if (api_err != TAPI_API_SUCCESS) {
				err("tel_register_noti_event() failed.. event id:[%s], api_err:[%d]", g_sound_event_list[evt_idx], api_err);
			}
		}

		api_err = tel_register_noti_event(telephony_handle->multi_handles[sim_idx], TAPI_NOTI_SIM_STATUS, __callmgr_telephony_handle_sim_status_events, telephony_handle);
		if (api_err != TAPI_API_SUCCESS) {
			err("tel_register_noti_event() failed.. event id:[%s], api_err:[%d]", TAPI_NOTI_SIM_STATUS, api_err);
		}

		api_err = tel_register_noti_event(telephony_handle->multi_handles[sim_idx], TAPI_PROP_MODEM_POWER, __callmgr_telephony_handle_modem_power_status_events, telephony_handle);
		if (api_err != TAPI_API_SUCCESS) {
			err("tel_register_noti_event() failed.. event id:[%s], api_err:[%d]", TAPI_PROP_MODEM_POWER, api_err);
		}

		for (evt_idx = 0; evt_idx < num_net_event; evt_idx++) {
			api_err = tel_register_noti_event(telephony_handle->multi_handles[sim_idx], g_network_event_list[evt_idx], __callmgr_telephony_handle_network_events, telephony_handle);
			if (api_err != TAPI_API_SUCCESS) {
				err("tel_register_noti_event() failed.. event id:[%s], api_err:[%d]", g_network_event_list[evt_idx], api_err);
			}
		}

		for (evt_idx = 0; evt_idx < num_sat_event; evt_idx++) {
			api_err = tel_register_noti_event(telephony_handle->multi_handles[sim_idx], g_sat_event_list[evt_idx], __callmgr_telephony_handle_sat_events, telephony_handle);
			if (api_err != TAPI_API_SUCCESS) {
				err("tel_register_noti_event() failed.. event id:[%s], api_err:[%d]", g_sat_event_list[evt_idx], api_err);
			}
		}
	}

	api_err = tel_register_noti_event(telephony_handle->multi_handles[0], TAPI_NOTI_CALL_PREFERRED_VOICE_SUBSCRIPTION, __callmgr_telephony_handle_preferred_sim_events, telephony_handle);
	if (api_err != TAPI_API_SUCCESS) {
		err("tel_register_noti_event() failed.. event id:[%s], api_err:[%d]", TAPI_NOTI_CALL_PREFERRED_VOICE_SUBSCRIPTION, api_err);
	}

	return 0;
}

static int __callmgr_telephony_is_telephony_ready(gboolean *o_is_ready)
{
	gboolean is_ready = FALSE;
	int ret = -1;

	ret = vconf_get_bool(VCONFKEY_TELEPHONY_READY, &is_ready);
	if (ret < 0) {
		err("vconf_get_bool failed.. ret_val: %d", ret);
		return -1;
	}
	if (is_ready) {
		*o_is_ready = TRUE;
	} else {
		warn("telephony is not ready yet.");
		*o_is_ready = FALSE;
	}

	return 0;
}

static int __callmgr_telephony_check_phone_initialized_status(callmgr_telephony_t telephony_handle, int slot_id, gboolean *is_initialized)
{
	int ret = -1;
	int status = -1;
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(slot_id < telephony_handle->modem_cnt, -1);
	dbg("__callmgr_telephony_is_phone_initialized()");

	ret = tel_check_modem_power_status(telephony_handle->multi_handles[slot_id], &status);
	if (TAPI_API_SUCCESS != ret || 1 /* offline */ == status || 2 /* Error */== status) {
		err("Tapi not initialized");
		*is_initialized = FALSE;
	} else {
		*is_initialized = TRUE;
	}
	return 0;
}

static int __callmgr_telephony_check_service_status(callmgr_telephony_t telephony_handle, int  slot_id, cm_network_status_e *cm_svc_type)
{
	int svc_type = -1;
	gboolean ret = FALSE;
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(slot_id < telephony_handle->modem_cnt, -1);
	CM_RETURN_VAL_IF_FAIL(cm_svc_type, -1);

	ret = tel_get_property_int(telephony_handle->multi_handles[slot_id], TAPI_PROP_NETWORK_SERVICE_TYPE, &svc_type);
	if (TAPI_API_SUCCESS == ret) {
		dbg("svc_type = [%d]", svc_type);
		switch (svc_type) {
			case TAPI_NETWORK_SERVICE_TYPE_UNKNOWN:
			case TAPI_NETWORK_SERVICE_TYPE_NO_SERVICE:
			case TAPI_NETWORK_SERVICE_TYPE_SEARCH:
				*cm_svc_type = CM_NETWORK_STATUS_OUT_OF_SERVICE;
				break;
			case TAPI_NETWORK_SERVICE_TYPE_EMERGENCY:
				*cm_svc_type = CM_NETWORK_STATUS_EMERGENCY_ONLY;
				break;
			case TAPI_NETWORK_SERVICE_TYPE_2G:
			case TAPI_NETWORK_SERVICE_TYPE_2_5G:
			case TAPI_NETWORK_SERVICE_TYPE_2_5G_EDGE:
				*cm_svc_type = CM_NETWORK_STATUS_IN_SERVICE_2G;
				break;
			case TAPI_NETWORK_SERVICE_TYPE_3G:
			case TAPI_NETWORK_SERVICE_TYPE_HSDPA:
			case TAPI_NETWORK_SERVICE_TYPE_LTE:
				*cm_svc_type = CM_NETWORK_STATUS_IN_SERVICE_3G;
				break;
			default:
				*cm_svc_type = CM_NETWORK_STATUS_IN_SERVICE_2G; /* Please check default value*/
				break;
		}
		return 0;
	} else {
		err("tel_get_property_int() failed..[%d]", ret);
		return -1;
	}
}

static int __callmgr_telephony_check_sim_initialized_status(callmgr_telephony_t telephony_handle, cm_telepony_sim_slot_type_e sim_slot, gboolean *is_initialized)
{
	int ret = -1;
	int sim_changed = 0;
	TelSimCardStatus_t sim_status = TAPI_SIM_STATUS_UNKNOWN;
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(sim_slot < telephony_handle->modem_cnt, -1);
	dbg("_callmgr_telephony_is_sim_initialized()");

	ret = tel_get_sim_init_info(telephony_handle->multi_handles[sim_slot], &sim_status, &sim_changed);
	if (TAPI_API_SUCCESS != ret) {
		err("tel_get_sim_init_info() failed with err[%d]", ret);
		*is_initialized = FALSE;
		return -1;

	} else {
		info("card[%d]status = %d", sim_slot, sim_status);
		switch (sim_status) {
		case TAPI_SIM_STATUS_SIM_INIT_COMPLETED:
			*is_initialized = TRUE;
			break;
		default:
			*is_initialized = FALSE;
			break;
		}
	}

	return 0;
}

static int __callmgr_telephony_check_cs_status(callmgr_telephony_t telephony_handle, int  slot_id, gboolean *is_available)
{
	gboolean is_cs_on = FALSE;
	int cs_type = VCONFKEY_TELEPHONY_SVC_CS_UNKNOWN;
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(slot_id < telephony_handle->modem_cnt, -1);

	if (TAPI_API_SUCCESS != tel_get_property_int(telephony_handle->multi_handles[slot_id], TAPI_PROP_NETWORK_CIRCUIT_STATUS, &cs_type)) {
		err("tel_get_property_int() failed");
		return -1;
	} else {
		dbg("CS network state : %d", cs_type);
		if (cs_type == VCONFKEY_TELEPHONY_SVC_CS_ON) {
			is_cs_on = TRUE;
		}
	}
	*is_available = is_cs_on;
	return 0;
}


static int __callmgr_telephony_get_imsi_mcc_mnc(callmgr_telephony_t telephony_handle, int slot_id, unsigned long *mcc, unsigned long *mnc)
{
	int ret = 0;
	TelSimImsiInfo_t sim_imsi_info;

	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(slot_id < telephony_handle->modem_cnt, -1);
	CM_RETURN_VAL_IF_FAIL((mcc != NULL), -1);
	CM_RETURN_VAL_IF_FAIL((mnc != NULL), -1);

	ret = tel_get_sim_imsi(telephony_handle->multi_handles[slot_id], &sim_imsi_info);
	if (0 == ret) {
		dbg("mcc = [%s], mnc = [%s]", sim_imsi_info.szMcc, sim_imsi_info.szMnc);
		*mcc = (unsigned long)atoi(sim_imsi_info.szMcc);
		*mnc = (unsigned long)atoi(sim_imsi_info.szMnc);
	} else {
		err("tel_get_sim_imsi failed..[%d]", ret);
		*mcc = CALL_NETWORK_MCC_UK;
		*mnc = CALL_NETWORK_MNC_01;
	}

	return 0;
}

static int __callmgr_telephony_get_network_mcc_mnc(callmgr_telephony_t telephony_handle, int slot_id, unsigned long *mcc, unsigned long *mnc)
{
	int ret = 0;
	char *plmn = NULL;

	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(slot_id < telephony_handle->modem_cnt, -1);
	CM_RETURN_VAL_IF_FAIL((mcc != NULL), -1);
	CM_RETURN_VAL_IF_FAIL((mnc != NULL), -1);

	ret = tel_get_property_string(telephony_handle->multi_handles[slot_id], TAPI_PROP_NETWORK_PLMN, &plmn);
	if (ret != TAPI_API_SUCCESS) {
		err("tel_get_property_string");
		return -1;
	}

	if (0 == ret && plmn != NULL) {
		char mcc_value[4] = {0,};
		char mnc_value[4] = {0,};

		dbg("plmn_value = [%s]", plmn);

		g_strlcpy(mcc_value, plmn, 4);
		g_strlcpy(mnc_value, plmn+3, 4);

		*mcc = (unsigned long)atoi(mcc_value);
		*mnc = (unsigned long)atoi(mnc_value);

		g_free(plmn);
	} else {
		err("vconf_get_int failed..[%d]", ret);
		*mcc = CALL_NETWORK_MCC_UK;
		*mnc = CALL_NETWORK_MNC_01;
	}
	dbg("mcc = %ld,mnc = %ld", *mcc, *mnc);

	return 0;
}

static int __callmgr_telephony_init(callmgr_telephony_t telephony_handle)
{
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	int modem_num = 0;
	int idx = 0;
	char** cp_list = NULL;

	cp_list = tel_get_cp_name_list();
	CM_RETURN_VAL_IF_FAIL(cp_list, -1);

	while (cp_list[modem_num]) {
		if (modem_num < CALLMGR_TAPI_HANDLE_MAX) {
			telephony_handle->multi_handles[modem_num] = tel_init(cp_list[modem_num]);
			dbg("multi_handles[%d] = 0x%x", modem_num, telephony_handle->multi_handles[modem_num]);
		} else {
			err("Couldn't supprot this handle. cp_list[%d]:%s", modem_num, cp_list[modem_num]);
			break;
		}
		modem_num++;
	}
	telephony_handle->multi_handles[modem_num] = NULL;
	telephony_handle->modem_cnt = modem_num;
	g_strfreev(cp_list);

	__callmgr_telephony_register_events(telephony_handle);

	for (idx = 0; idx < modem_num; idx++) {
		if (telephony_handle->multi_handles[idx] != NULL) {
			warn("tel_init() for sim slot [%d] is success.", idx);
			struct __modem_info *modem_info = &telephony_handle->modem_info[idx];
			TapiResult_t tapi_err = TAPI_API_SUCCESS;

			/* Get modem power status */
			__callmgr_telephony_check_phone_initialized_status(telephony_handle, idx, &modem_info->is_phone_initialized);
			/* Get No SIM status from Telephony */
			__callmgr_telephony_check_no_sim_state(telephony_handle, idx, &modem_info->is_no_sim);
			/* Get SIM initialized status */
			__callmgr_telephony_check_sim_initialized_status(telephony_handle, idx, &modem_info->is_sim_initialized);
			/* Get Card type from Telephony */
			__callmgr_telephony_get_card_type(telephony_handle, idx, &(modem_info->card_type));
			/* Get SIM MCC/MNC */
			__callmgr_telephony_get_imsi_mcc_mnc(telephony_handle, idx, &modem_info->sim_mcc, &modem_info->sim_mnc);
			/* Get Network MCC/MNC */
			__callmgr_telephony_get_network_mcc_mnc(telephony_handle, idx, &modem_info->net_mcc, &modem_info->net_mnc);
			/* Get CS status */
			__callmgr_telephony_check_cs_status(telephony_handle, idx, &modem_info->is_cs_available);
			/* Get network status */
			__callmgr_telephony_check_service_status(telephony_handle, idx, &modem_info->network_status);

			memset(&modem_info->sim_ecc_list, 0x00, sizeof(TelSimEccList_t));
			tapi_err = tel_get_sim_ecc(telephony_handle->multi_handles[idx], &modem_info->sim_ecc_list);
			if (TAPI_API_SUCCESS != tapi_err) {
				warn("tel_get_sim_ecc failed, tapi_err=%d", tapi_err);
			} else {
				dbg("tel_get_sim_ecc success");
			}
		} else {
			err("tel_init() for sim slot [%d] is failed.", idx);
		}
	}

	if (telephony_handle->modem_cnt > 1) {		/* Multi SIM */
		telephony_handle->active_sim_slot = CM_TELEPHONY_SIM_UNKNOWN;
	} else {				/* Single SIM */
		telephony_handle->active_sim_slot = CM_TELEPHONY_SIM_1;
	}

	return 0;
}

static void __callmgr_telephony_telephony_ready_cb(keynode_t *key, void *data)
{
	gboolean is_ready = FALSE;
	callmgr_telephony_t telephony_handle = (callmgr_telephony_t)data;
	CM_RETURN_IF_FAIL(telephony_handle);

	dbg("__callmgr_telephony_telephony_ready_cb");

	is_ready = vconf_keynode_get_bool(key);
	if (is_ready) {
		__callmgr_telephony_init(telephony_handle);
		vconf_ignore_key_changed(VCONFKEY_TELEPHONY_READY, __callmgr_telephony_telephony_ready_cb);
	} else {
		err("telephony is not ready yet");
	}
	return;
}

int _callmgr_telephony_init(callmgr_telephony_t *telephony_handle, telephony_event_cb cb_fn, void *user_data)
{
	struct __callmgr_telephony *handle = NULL;
	gboolean is_telephony_ready = FALSE;

	handle = calloc(1, sizeof(struct __callmgr_telephony));
	CM_RETURN_VAL_IF_FAIL(handle, -1);

	handle->cb_fn = cb_fn;
	handle->user_data = user_data;
	*telephony_handle = handle;

	__callmgr_telephony_is_telephony_ready(&is_telephony_ready);
	if (is_telephony_ready) {
		__callmgr_telephony_init(handle);
	} else {
		vconf_notify_key_changed(VCONFKEY_TELEPHONY_READY, __callmgr_telephony_telephony_ready_cb, handle);
	}

	return 0;
}

int _callmgr_telephony_deinit(callmgr_telephony_t telephony_handle)
{
	int sim_idx = 0;
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);

	/*Unsubscribe Events */
	dbg("Unsubscribing Events");

	while (telephony_handle->multi_handles[sim_idx]) {
		tel_deinit(telephony_handle->multi_handles[sim_idx]);
		telephony_handle->multi_handles[sim_idx] = NULL;
		sim_idx++;
	}

	g_free(telephony_handle);
	telephony_handle = NULL;

	return 0;
}

int _callmgr_telephony_call_new(callmgr_telephony_t telephony_handle, cm_telephony_call_type_e type,
		cm_telephony_call_direction_e direction, const char* number, cm_telephony_call_data_t **call_data_out)
{
	struct __modem_info *modem_info = NULL;
	cm_telephony_call_data_t *call = NULL;
	int ret = -1;
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(call_data_out, -1);

	ret = _callmgr_telephony_get_call_by_call_id(telephony_handle, NO_CALL_HANDLE, &call);
	if ((ret == 0) && (call != NULL)) {
		warn("Deleting call data with Invalid handle...");
		_callmgr_telephony_call_delete(telephony_handle, NO_CALL_HANDLE);
		call = NULL;
	} else {
		dbg("No call data exists...");
	}

	call = (cm_telephony_call_data_t*)calloc(1, sizeof(cm_telephony_call_data_t));
	CM_RETURN_VAL_IF_FAIL(call, -1);

	call->call_id = NO_CALL_HANDLE;
	call->call_direction = direction;
	if (number) {
		_callmgr_util_extract_call_number_without_cli(number, &call->call_number, &call->dtmf_number);
	}
	call->call_state = CM_TEL_CALL_STATE_IDLE;
	call->is_sat_originated_call = FALSE;	// default value is FALSE. (use _callmgr_telephony_set_is_sat_originated_call() to update.)
	call->call_type = type;
	call->start_time = 0;

	modem_info = &telephony_handle->modem_info[telephony_handle->active_sim_slot];
	modem_info->call_list = g_slist_append(modem_info->call_list, call);

	*call_data_out = call;
	return 0;
}

int _callmgr_telephony_call_delete(callmgr_telephony_t telephony_handle, unsigned int call_id)
{
	struct __modem_info *modem_info = NULL;
	cm_telephony_call_data_t *call = NULL;
	int list_len = -1;
	int idx = -1;
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);

	modem_info = &telephony_handle->modem_info[telephony_handle->active_sim_slot];
	list_len = g_slist_length(modem_info->call_list);
	for (idx = 0; idx < list_len; idx++) {
		call = g_slist_nth_data(modem_info->call_list, idx);
		if ((call) && (call->call_id == call_id)) {
			__callmgr_telephony_remove_wait_call(telephony_handle, call);
			modem_info->call_list = g_slist_remove(modem_info->call_list, call);
			g_free(call->call_number);
			g_free(call->dtmf_number);
			g_free(call);
			info("Call(which has ID %d) is removed from the call list", call_id);
			return 0;
		}
	}
	return -1;
}

int _callmgr_telephony_set_active_sim_slot(callmgr_telephony_t telephony_handle, cm_telepony_sim_slot_type_e active_slot)
{
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	warn("_callmgr_telephony_set_active_sim_slot(), active_slot[%d]", active_slot);
	telephony_handle->active_sim_slot = active_slot;

	return 0;
}

int _callmgr_telephony_get_active_sim_slot(callmgr_telephony_t telephony_handle, cm_telepony_sim_slot_type_e *active_slot)
{
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	dbg("_callmgr_telephony_get_active_sim_slot()");
	*active_slot = telephony_handle->active_sim_slot;
	dbg("active_slot[%d]", *active_slot);
	return 0;
}

static void __callmgr_telephony_dial_call_resp_cb(TapiHandle *handle, int result, void *tapi_data, void *user_data)
{
	callmgr_telephony_t telephony_handle = NULL;
	info("__callmgr_telephony_dial_call_resp_cb() Result : %d", result);

	telephony_handle = (callmgr_telephony_t)user_data;

	if (TAPI_CAUSE_SUCCESS != result) {
		dbg("MO Call Dial call Failed with error cause: %d", result);

		if (result == TAPI_CAUSE_FIXED_DIALING_NUMBER_ONLY) {
			telephony_handle->cb_fn(CM_TELEPHONY_EVENT_NETWORK_ERROR, (void *)CM_TEL_CALL_ERR_FDN_ONLY, telephony_handle->user_data);
		} else {
			telephony_handle->cb_fn(CM_TELEPHONY_EVENT_NETWORK_ERROR, (void *)CM_TEL_CALL_ERR_DIAL_FAIL, telephony_handle->user_data);
		}

		__callmgr_telephony_remove_wait_call_list(telephony_handle);
		_callmgr_telephony_call_delete(telephony_handle, NO_CALL_HANDLE);
	} else {
		telephony_handle->cb_fn(CM_TELEPHONY_EVENT_NETWORK_ERROR, (void *)CM_TEL_CALL_ERR_NONE, telephony_handle->user_data);
	}

	return;
}

int _callmgr_telephony_dial(callmgr_telephony_t telephony_handle, cm_telephony_call_data_t *call)
{
	int ret = -1;
	TelCallDial_t dialCallInfo = {0,};
	cm_telephony_call_data_t *active_call = NULL;
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(telephony_handle->active_sim_slot < telephony_handle->modem_cnt, -1);
	CM_RETURN_VAL_IF_FAIL(call, -1);
	dbg("_callmgr_telephony_dial");

	_callmgr_telephony_get_call_by_state(telephony_handle, CM_TEL_CALL_STATE_ACTIVE, &active_call);
	if (active_call) {
		info("Active call[%d] exist. Make it on hold", active_call->call_id);
		ret = _callmgr_telephony_hold_call(telephony_handle);
		if (ret < 0) {
			err("_callmgr_telephony_hold_call() failed");
			return -1;
		}
	} else {
		ret = __callmgr_telephony_create_wait_call_list(telephony_handle, CM_TELEPHONY_EVENT_ACTIVE, call, NULL);
		if (ret < 0) {
			err("__callmgr_telephony_create_wait_call_list() failed");
			return -1;
		}

		if (call->call_type == CM_TEL_CALL_TYPE_E911) {
			dialCallInfo.CallType = TAPI_CALL_TYPE_E911;
			dialCallInfo.Ecc = call->ecc_category;
		}
		/*else if (call->call_domain == CM_TEL_CALL_DOMAIN_PS) {
			dialCallInfo.CallType = TAPI_CALL_TYPE_DATA; ToDO: Domain should be known here
		}*/
		else if (call->call_type == CM_TEL_CALL_TYPE_CS_VOICE) {
			dialCallInfo.CallType = TAPI_CALL_TYPE_VOICE;
		} else {
			dialCallInfo.CallType = TAPI_CALL_TYPE_DATA;
		}

		g_strlcpy(dialCallInfo.szNumber, call->call_number, TAPI_CALL_DIALDIGIT_LEN_MAX);
		ret = tel_dial_call(telephony_handle->multi_handles[telephony_handle->active_sim_slot], &dialCallInfo, __callmgr_telephony_dial_call_resp_cb, telephony_handle);
		if (ret != TAPI_API_SUCCESS) {
			err("tel_dial_call get failed with err: %d", ret);
			__callmgr_telephony_remove_wait_call_list(telephony_handle);
			return -1;
		}
	}

	return 0;
}

static void __callmgr_telephony_hold_call_resp_cb(TapiHandle *handle, int result, void *tapi_data, void *user_data)
{
	callmgr_telephony_t telephony_handle = (callmgr_telephony_t)user_data;
	dbg("__callmgr_telephony_hold_call_resp_cb() Result : %d", result);

	if (TAPI_CAUSE_SUCCESS != result) {
		err("tel_hold_call() failed");
		cm_telephony_call_data_t *call_data = NULL;
		_callmgr_telephony_get_call_by_call_id(telephony_handle, NO_CALL_HANDLE, &call_data);
		if (call_data) {
			info("Pending call exist.");
			_callmgr_telephony_call_delete(telephony_handle, NO_CALL_HANDLE);
		}
		_callmgr_util_launch_popup(CALL_POPUP_CALL_ERR, CALL_ERR_HOLD_CALL_FAILED, NULL, 0, NULL, NULL);
		__callmgr_telephony_remove_wait_call_list(telephony_handle);
	} else {
		dbg("tel_hold_call success");
	}
	return;
}

int _callmgr_telephony_hold_call(callmgr_telephony_t telephony_handle)
{
	int ret = FALSE;
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(telephony_handle->active_sim_slot < telephony_handle->modem_cnt, -1);
	cm_telephony_call_data_t *active_call_data = NULL;

	/* Fetch the Active call data */
	ret = _callmgr_telephony_get_call_by_state(telephony_handle, CM_TEL_CALL_STATE_ACTIVE, &active_call_data);
	CM_RETURN_VAL_IF_FAIL(active_call_data, -1);

	ret = __callmgr_telephony_create_wait_call_list(telephony_handle, CM_TELEPHONY_EVENT_HELD, active_call_data, NULL);
	if (ret < 0) {
		err("__callmgr_telephony_create_wait_call_list() failed");
		return -1;
	}

	ret = tel_hold_call(telephony_handle->multi_handles[telephony_handle->active_sim_slot], active_call_data->call_id, __callmgr_telephony_hold_call_resp_cb, telephony_handle);
	if (TAPI_API_SUCCESS != ret) {
		err("tel_hold_call failed Error_code :%d", ret);
		__callmgr_telephony_remove_wait_call_list(telephony_handle);
		return -1;
	}
	return 0;
}

static void __callmgr_telephony_join_call_resp_cb(TapiHandle *handle, int result, void *tapi_data, void *user_data)
{
	callmgr_telephony_t telephony_handle = (callmgr_telephony_t)user_data;
	dbg("__callmgr_telephony_join_call_resp_cb() Result : %d", result);

	if (TAPI_CAUSE_SUCCESS != result) {
		err("_callmgr_telephony_join_call() get failed ");
		__callmgr_telephony_remove_wait_call_list(telephony_handle);
		_callmgr_util_launch_popup(CALL_POPUP_CALL_ERR, CALL_ERR_JOIN_CALL_FAILED, NULL, 0, NULL, NULL);
	} else {
		dbg("tel_join_call success");
	}

	return;
}

int _callmgr_telephony_join_call(callmgr_telephony_t telephony_handle)
{
	gboolean ret = -1;
	cm_telephony_call_data_t *active_call_data = NULL;
	cm_telephony_call_data_t *held_call_data = NULL;

	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(telephony_handle->active_sim_slot < telephony_handle->modem_cnt, -1);
	dbg("_callmgr_telephony_join_call()");

	/* Fetch the Active call data */
	ret = _callmgr_telephony_get_call_by_state(telephony_handle, CM_TEL_CALL_STATE_ACTIVE, &active_call_data);
	CM_RETURN_VAL_IF_FAIL(active_call_data, -1);

	/* Fetch the Held call data */
	ret = _callmgr_telephony_get_call_by_state(telephony_handle, CM_TEL_CALL_STATE_HELD, &held_call_data);
	CM_RETURN_VAL_IF_FAIL(held_call_data, -1);

	 ret = __callmgr_telephony_create_wait_call_list(telephony_handle, CM_TELEPHONY_EVENT_JOIN, active_call_data, held_call_data);
	 if (ret < 0) {
		 err("__callmgr_telephony_create_wait_call_list() failed");
		 return -1;
	 }

	ret = tel_join_call(telephony_handle->multi_handles[telephony_handle->active_sim_slot], active_call_data->call_id, held_call_data->call_id, __callmgr_telephony_join_call_resp_cb, telephony_handle);
	if (ret != TAPI_API_SUCCESS) {
		err("tel_join_call get failed with err: %d", ret);
		__callmgr_telephony_remove_wait_call_list(telephony_handle);
		return -1;
	}
	return 0;
}

static void __callmgr_telephony_split_call_resp_cb(TapiHandle *handle, int result, void *tapi_data, void *user_data)
{
	callmgr_telephony_t telephony_handle = (callmgr_telephony_t)user_data;
	dbg("__callmgr_telephony_split_call_resp_cb() Result : %d", result);

	if (TAPI_CAUSE_SUCCESS != result) {
		err("tel_split_call() is failed");
		__callmgr_telephony_remove_wait_call_list(telephony_handle);
	} else {
		info("tel_split_call() is success");
	}

	return;
}

int _callmgr_telephony_split_call(callmgr_telephony_t telephony_handle, unsigned int call_id)
{
	int ret = -1;
	cm_telephony_call_data_t *call_data = NULL;
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(telephony_handle->active_sim_slot < telephony_handle->modem_cnt, -1);
	dbg("_callmgr_telephony_split_call");

	ret = _callmgr_telephony_get_call_by_call_id(telephony_handle, call_id, &call_data);
	CM_RETURN_VAL_IF_FAIL(call_data, -1);

	ret = __callmgr_telephony_create_wait_call_list(telephony_handle, CM_TELEPHONY_EVENT_SPLIT, call_data, NULL);
	if (ret < 0) {
		err("__callmgr_telephony_create_wait_call_list() failed");
		return -1;
	}

	ret = tel_split_call(telephony_handle->multi_handles[telephony_handle->active_sim_slot], call_id, __callmgr_telephony_split_call_resp_cb, telephony_handle);
	if (ret != TAPI_API_SUCCESS) {
		err("_callmgr_telephony_split_call failed with err: %d", ret);
		__callmgr_telephony_remove_wait_call_list(telephony_handle);
		return -1;
	}
	return 0;
}

static void __callmgr_telephony_set_answer_requesting(callmgr_telephony_t telephony_handle, gboolean is_requesting)
{
	if (telephony_handle) {
		warn("set Answer requesting : %d", is_requesting);
		telephony_handle->ans_requesting = is_requesting;
	}

	return;
}

int _callmgr_telephony_set_answer_request_type(callmgr_telephony_t telephony_handle, cm_telephony_answer_request_type_e is_active_end)
{
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);

	warn("set end_active_after_held_end : %d", is_active_end);
	telephony_handle->end_active_after_held_end = is_active_end;

	return 0;
}

static void __callmgr_telephony_answer_call_resp_cb(TapiHandle *handle, int result, void *tapi_data, void *user_data)
{
	dbg("__callmgr_telephony_answer_call_resp_cb() Result : %d", result);

	callmgr_telephony_t telephony_handle = (callmgr_telephony_t)user_data;

	__callmgr_telephony_set_answer_requesting(telephony_handle, FALSE);

	if (TAPI_CAUSE_SUCCESS != result) {
		err("tel_answer_call() failed");
		_callmgr_util_launch_popup(CALL_POPUP_CALL_ERR, CALL_ERR_ANSWER_CALL_FAILED, NULL, 0, NULL, NULL);
	}
	return;
}

int _callmgr_telephony_answer_call(callmgr_telephony_t telephony_handle, int ans_type)
{
	int ret = -1;
	cm_telephony_call_data_t *incoming_call_data = NULL;
	unsigned int incoming_call_id = NO_CALL_HANDLE;
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(telephony_handle->active_sim_slot < telephony_handle->modem_cnt, -1);
	dbg("_callmgr_telephony_answer_call() with answer type[%d]", ans_type);

	/* Fetch the Incoming call data */
	ret = _callmgr_telephony_get_call_by_state(telephony_handle, CM_TEL_CALL_STATE_INCOMING, &incoming_call_data);
	if (ret < 0 || incoming_call_data == NULL) {
		err("_callmgr_telephony_answer_call() failed to fetch Incoming call data");
		return -1;
	}

	incoming_call_id = incoming_call_data->call_id;

	switch (ans_type) {
	case CM_TEL_CALL_ANSWER_TYPE_NORMAL:
		{
			ret = tel_answer_call(telephony_handle->multi_handles[telephony_handle->active_sim_slot], incoming_call_id, TAPI_CALL_ANSWER_ACCEPT, __callmgr_telephony_answer_call_resp_cb, telephony_handle);
			if (ret != TAPI_API_SUCCESS) {
				err("_callmgr_telephony_answer_call failed with err: %d", ret);
				return -1;
			}
			__callmgr_telephony_set_answer_requesting(telephony_handle, TRUE);
		}
		break;

	case CM_TEL_CALL_ANSWER_TYPE_HOLD_ACTIVE_AND_ACCEPT:
		{
			ret = tel_answer_call(telephony_handle->multi_handles[telephony_handle->active_sim_slot], incoming_call_id, TAPI_CALL_ANSWER_HOLD_AND_ACCEPT, __callmgr_telephony_answer_call_resp_cb, telephony_handle);
			if (ret != TAPI_API_SUCCESS) {
				err("_callmgr_telephony_answer_call failed with err: %d", ret);
				return -1;
			}
			__callmgr_telephony_set_answer_requesting(telephony_handle, TRUE);

		}
		break;

	case CM_TEL_CALL_ANSWER_TYPE_RELEASE_ACTIVE_AND_ACCEPT:
		{
			ret = tel_answer_call(telephony_handle->multi_handles[telephony_handle->active_sim_slot], incoming_call_id, TAPI_CALL_ANSWER_REPLACE, __callmgr_telephony_answer_call_resp_cb, telephony_handle);
			if (ret != TAPI_API_SUCCESS) {
				err("_callmgr_telephony_answer_call failed with err: %d", ret);
				return -1;
			}
			__callmgr_telephony_set_answer_requesting(telephony_handle, TRUE);
		}
		break;

	case CM_TEL_CALL_ANSWER_TYPE_RELEASE_HOLD_AND_ACCEPT:
		{
			__callmgr_telephony_end_all_call_by_state(telephony_handle, CM_TEL_CALL_STATE_HELD);
			_callmgr_telephony_set_answer_request_type(telephony_handle, CM_TEL_HOLD_ACTIVE_AFTER_HELD_END);
			__callmgr_telephony_set_answer_requesting(telephony_handle, TRUE);
		}
		break;

	case CM_TEL_CALL_ANSWER_TYPE_RELEASE_ALL_AND_ACCEPT:
		__callmgr_telephony_end_all_call_by_state(telephony_handle, CM_TEL_CALL_STATE_HELD);
		_callmgr_telephony_set_answer_request_type(telephony_handle, CM_TEL_END_ACTIVE_AFTER_HELD_END);
		__callmgr_telephony_set_answer_requesting(telephony_handle, TRUE);
		break;

	default:
		err("_callmgr_telephony_answer_call : answer_type not supported");
		return -1;
	}

	return 0;
}

int _callmgr_telephony_reject_call(callmgr_telephony_t telephony_handle)
{
	int ret = -1;
	cm_telephony_call_data_t *incoming_call_data = NULL;
	unsigned int incoming_call_id = NO_CALL_HANDLE;
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	dbg("_callmgr_telephony_reject_call");

	/* Fetch the Incoming call data */
	ret = _callmgr_telephony_get_call_by_state(telephony_handle, CM_TEL_CALL_STATE_INCOMING, &incoming_call_data);
	if (ret < 0 || incoming_call_data == NULL) {
		err("_callmgr_telephony_reject_call() failed to fetch Incoming call data");
		return -1;
	}

	incoming_call_id = incoming_call_data->call_id;

	ret = tel_answer_call(telephony_handle->multi_handles[telephony_handle->active_sim_slot], incoming_call_id, TAPI_CALL_ANSWER_REJECT, __callmgr_telephony_answer_call_resp_cb, telephony_handle);
	if (ret != TAPI_API_SUCCESS) {
		err("tel_answer_call failed with err: %d", ret);
		return -1;
	}

	return 0;
}

static void __callmgr_telephony_active_call_resp_cb(TapiHandle *handle, int result, void *tapi_data, void *user_data)
{
	callmgr_telephony_t telephony_handle = (callmgr_telephony_t)user_data;
	dbg("__callmgr_telephony_active_call_resp_cb() Result : %d", result);

	if (TAPI_CAUSE_SUCCESS != result) {
		err("tel_activate_call() failed");
		__callmgr_telephony_remove_wait_call_list(telephony_handle);
		_callmgr_util_launch_popup(CALL_POPUP_CALL_ERR, CALL_ERR_RETRIEVE_CALL_FAILED, NULL, 0, NULL, NULL);
	} else {
		dbg("tel_activate_call() success");
	}
	return;
}

int _callmgr_telephony_active_call(callmgr_telephony_t telephony_handle)
{
	int ret = FALSE;
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(telephony_handle->active_sim_slot < telephony_handle->modem_cnt, -1);
	cm_telephony_call_data_t *held_call_data = NULL;

	/* Fetch the Held call data */
	ret = _callmgr_telephony_get_call_by_state(telephony_handle, CM_TEL_CALL_STATE_HELD, &held_call_data);
	CM_RETURN_VAL_IF_FAIL(held_call_data, -1);

	ret = __callmgr_telephony_create_wait_call_list(telephony_handle, CM_TELEPHONY_EVENT_RETRIEVED, NULL, held_call_data);
	if (ret < 0) {
		err("__callmgr_telephony_create_wait_call_list() failed");
		return -1;
	}

	dbg("_callmgr_telephony_active_call (call_id :%d)", held_call_data->call_id);
	ret = tel_active_call(telephony_handle->multi_handles[telephony_handle->active_sim_slot], held_call_data->call_id, __callmgr_telephony_active_call_resp_cb, telephony_handle);
	if (TAPI_API_SUCCESS != ret) {
		err("tel_activate_call failed Error_code :%d", ret);
		__callmgr_telephony_remove_wait_call_list(telephony_handle);
		return -1;
	}
	return 0;
}

static void __callmgr_telephony_swap_call_resp_cb(TapiHandle *handle, int result, void *tapi_data, void *user_data)
{
	callmgr_telephony_t telephony_handle = (callmgr_telephony_t)user_data;
	dbg("__callmgr_telephony_swap_call_resp_cb() Result : %d", result);

	if (TAPI_CAUSE_SUCCESS != result) {
		err("tel_swap_call() failed");
		_callmgr_util_launch_popup(CALL_POPUP_CALL_ERR, CALL_ERR_SWAP_CALL_FAILED, NULL, 0, NULL, NULL);
		__callmgr_telephony_remove_wait_call_list(telephony_handle);
	} else {
		info("tel_swap_call() is success");
	}
	return;
}

int _callmgr_telephony_swap_call(callmgr_telephony_t telephony_handle)
{
	gboolean ret = FALSE;
	cm_telephony_call_data_t *active_call_data = NULL;
	cm_telephony_call_data_t *held_call_data = NULL;
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(telephony_handle->active_sim_slot < telephony_handle->modem_cnt, -1);
	dbg("_callmgr_telephony_swap_call()");

	/* Fetch the Active and Held call data */
	ret = _callmgr_telephony_get_call_by_state(telephony_handle, CM_TEL_CALL_STATE_ACTIVE, &active_call_data);
	CM_RETURN_VAL_IF_FAIL(active_call_data, -1);
	ret = _callmgr_telephony_get_call_by_state(telephony_handle, CM_TEL_CALL_STATE_HELD, &held_call_data);
	CM_RETURN_VAL_IF_FAIL(held_call_data, -1);

	ret = __callmgr_telephony_create_wait_call_list(telephony_handle, CM_TELEPHONY_EVENT_SWAP, active_call_data, held_call_data);
	if (ret < 0) {
		err("__callmgr_telephony_create_wait_call_list() failed");
		return -1;
	}

	ret = tel_swap_call(telephony_handle->multi_handles[telephony_handle->active_sim_slot],
						active_call_data->call_id, held_call_data->call_id,
						__callmgr_telephony_swap_call_resp_cb, telephony_handle);
	if (TAPI_API_SUCCESS != ret) {
		err("tel_swap_call() failed");
		__callmgr_telephony_remove_wait_call_list(telephony_handle);
		return -1;
	}
	return 0;
}

static void __callmgr_telephony_transfer_call_resp_cb(TapiHandle *handle, int result, void *tapi_data, void *user_data)
{
	dbg("__callmgr_telephony_transfer_call_resp_cb() Result : %d", result);
	if (TAPI_CAUSE_SUCCESS != result) {
		err("tel_transfer_call() failed");
		_callmgr_util_launch_popup(CALL_POPUP_CALL_ERR, CALL_ERR_TRANSFER_CALL_FAILED, NULL, 0, NULL, NULL);
	} else {
		info("tel_transfer_call() is success");
	}
	return;
}

int _callmgr_telephony_transfer_call(callmgr_telephony_t telephony_handle, cm_telephony_call_data_t *call_data)
{
	gboolean ret = FALSE;
	unsigned int call_id = NO_CALL_HANDLE;
	int call_count = -1;

	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(telephony_handle->active_sim_slot < telephony_handle->modem_cnt, -1);
	CM_RETURN_VAL_IF_FAIL(call_data, -1);
	dbg("_callmgr_telephony_transfer_call()");

	ret = _callmgr_telephony_get_call_count(telephony_handle, &call_count);
	if (ret || call_count <= 1) {
		err("callmgr_core_process_transfer_call() not possible as 2 calls not exists");
		return -1;
	}

	_callmgr_telephony_get_call_id(call_data, &call_id);

	ret = tel_transfer_call(telephony_handle->multi_handles[telephony_handle->active_sim_slot], call_id, __callmgr_telephony_transfer_call_resp_cb, telephony_handle);
	if (ret != TAPI_API_SUCCESS) {
		err("tel_transfer_call get failed with err: %d", ret);
		return -1;
	}

	return 0;
}

static void __callmgr_telephony_end_call_resp_cb(TapiHandle *handle, int result, void *tapi_data, void *user_data)
{
	callmgr_telephony_t telephony_handle = user_data;
	CM_RETURN_IF_FAIL(tapi_data);
	CM_RETURN_IF_FAIL(telephony_handle);
	dbg("Result : %d", result);
	if (TAPI_CAUSE_SUCCESS != result) {
		err("tel_end_call() failed");
	}

	return;
}

int _callmgr_telephony_end_call(callmgr_telephony_t telephony_handle, unsigned int call_id, cm_telephony_call_release_type_e release_type)
{
	int ret = -1;
	TapiResult_t ret_tapi = TAPI_API_SUCCESS;

	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(telephony_handle->active_sim_slot < telephony_handle->modem_cnt, -1);
	dbg("release_type: %d", release_type);

	switch (release_type) {
	case CM_TEL_CALL_RELEASE_TYPE_BY_CALL_HANDLE:
		{
			/* To do
			 * Check whether passed handle is of incoming, outgoing or connected call...and accordingly change the state.
			 */
			ret_tapi = tel_end_call(telephony_handle->multi_handles[telephony_handle->active_sim_slot], call_id, TAPI_CALL_END, __callmgr_telephony_end_call_resp_cb, telephony_handle);
			if (ret_tapi != TAPI_API_SUCCESS) {
				err("tel_end_call() get failed with err: %d", ret);
				return -1;
			}
		}
		break;
	case CM_TEL_CALL_RELEASE_TYPE_ALL_CALLS:
		{
			ret_tapi = tel_end_call(telephony_handle->multi_handles[telephony_handle->active_sim_slot], call_id, TAPI_CALL_END_ALL, __callmgr_telephony_end_call_resp_cb, telephony_handle);
			if (ret_tapi != TAPI_API_SUCCESS) {
				err("tel_end_call() get failed with err: %d", ret);
				return -1;
			}
		}
		break;
	case CM_TEL_CALL_RELEASE_TYPE_ALL_HOLD_CALLS:
		{
			ret_tapi = tel_end_call(telephony_handle->multi_handles[telephony_handle->active_sim_slot], call_id, TAPI_CALL_END_HOLD_ALL, __callmgr_telephony_end_call_resp_cb, telephony_handle);
			if (ret_tapi != TAPI_API_SUCCESS) {
				err("tel_end_call() get failed with err: %d", ret);
				return -1;
			}
		}
		break;
	case CM_TEL_CALL_RELEASE_TYPE_ALL_ACTIVE_CALLS:
		{
			struct __modem_info *modem_info = NULL;
			GSList *call_list = NULL;
			cm_telephony_call_data_t *call_obj = NULL;
			cm_telephony_call_state_e call_state = CM_TEL_CALL_STATE_MAX;
			unsigned int call_id = 0;

			modem_info = &telephony_handle->modem_info[telephony_handle->active_sim_slot];
			call_list = modem_info->call_list;

			while (call_list) {
				call_obj = (cm_telephony_call_data_t*)call_list->data;
				if (!call_obj) {
					warn("[ error ] call object : 0");
					call_list = call_list->next;
					continue;
				}

				call_id = call_obj->call_id;
				call_state = call_obj->call_state;

				dbg("call_id = %d call_state = %d", call_id, call_state);
				call_list = g_slist_next(call_list);
				if (call_state == CM_TEL_CALL_STATE_ACTIVE) {
					ret_tapi = tel_end_call(telephony_handle->multi_handles[telephony_handle->active_sim_slot], call_id, TAPI_CALL_END, __callmgr_telephony_end_call_resp_cb, NULL);
					if (ret_tapi != TAPI_API_SUCCESS) {
						err("tel_end_call() get failed with err: %d", ret);
						return -1;
					}
				}
			}
		}
		break;
	default:
		err("wrong release_type");
		return -1;

	}
	return 0;
}

static void __callmgr_telephony_start_dtmf_cb(TapiHandle *handle, int result, void *data, void *user_data)
{
	callmgr_telephony_t telephony_handle = NULL;
	info("__callmgr_telephony_start_dtmf_cb() Result : %d", result);

	telephony_handle = (callmgr_telephony_t)user_data;

	if (TAPI_CAUSE_SUCCESS == result) {
		telephony_handle->cb_fn(CM_TELEPHON_EVENT_DTMF_START_ACK, (void *)CM_TEL_DTMF_SEND_SUCCESS, telephony_handle->user_data);
	} else {
		telephony_handle->cb_fn(CM_TELEPHON_EVENT_DTMF_START_ACK, (void *)CM_TEL_DTMF_SEND_FAIL, telephony_handle->user_data);
	}

	return;
}

int _callmgr_telephony_start_dtmf(callmgr_telephony_t telephony_handle, unsigned char dtmf_digit)
{
	int result = TAPI_API_SUCCESS;

	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(telephony_handle->active_sim_slot < telephony_handle->modem_cnt, -1);

	result = tel_start_call_cont_dtmf(telephony_handle->multi_handles[telephony_handle->active_sim_slot], dtmf_digit, __callmgr_telephony_start_dtmf_cb, telephony_handle);
	if (result != TAPI_API_SUCCESS) {
		err("tel_start_call_cont_dtmf error : %d", result);
		return -1;
	}

	return 0;
}

static void __callmgr_telephony_stop_dtmf_cb(TapiHandle *handle, int result, void *data, void *user_data)
{
	callmgr_telephony_t telephony_handle = NULL;
	info("__callmgr_telephony_stop_dtmf_cb() Result : %d", result);

	telephony_handle = (callmgr_telephony_t)user_data;

	if (TAPI_CAUSE_SUCCESS == result) {
		telephony_handle->cb_fn(CM_TELEPHON_EVENT_DTMF_STOP_ACK, (void *)CM_TEL_DTMF_SEND_SUCCESS, telephony_handle->user_data);
	} else {
		telephony_handle->cb_fn(CM_TELEPHON_EVENT_DTMF_STOP_ACK, (void *)CM_TEL_DTMF_SEND_FAIL, telephony_handle->user_data);
	}

	return;
}

int _callmgr_telephony_stop_dtmf(callmgr_telephony_t telephony_handle)
{
	int result = TAPI_API_SUCCESS;
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(telephony_handle->active_sim_slot < telephony_handle->modem_cnt, -1);

	result = tel_stop_call_cont_dtmf(telephony_handle->multi_handles[telephony_handle->active_sim_slot], __callmgr_telephony_stop_dtmf_cb, telephony_handle);
	if (result != TAPI_API_SUCCESS) {
		err("tel_stop_call_cont_dtmf error : %d", result);
		return -1;
	}
	return 0;
}

static void __callmgr_telephony_burst_dtmf_cb(TapiHandle *handle, int result, void *data, void *user_data)
{
	dbg("result : %d", result);
}

int _callmgr_telephony_burst_dtmf(callmgr_telephony_t telephony_handle, const char *dtmf_digits)
{
	int result = TAPI_API_SUCCESS;
	TelCallBurstDtmf_t info;
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(telephony_handle->active_sim_slot < telephony_handle->modem_cnt, -1);
	CM_RETURN_VAL_IF_FAIL(dtmf_digits, -1);

	memset(&info, 0x0, sizeof(info));
	g_strlcpy(info.dtmf_string, dtmf_digits, TAPI_CALL_BURST_DTMF_STRING_MAX + 1);
	result = tel_send_call_burst_dtmf(telephony_handle->multi_handles[telephony_handle->active_sim_slot], &info, __callmgr_telephony_burst_dtmf_cb, NULL);
	if (result != TAPI_API_SUCCESS) {
		err("tel_send_call_burst_dtmf error : %d", result);
		return -1;
	}

	return 0;
}

static void __callmgr_telephony_all_call_list_cb(TelCallStatus_t *out, void *user_data)
{
	dbg("__callmgr_telephony_get_all_call_list()");
	callmgr_telephony_t telephony_handle = (callmgr_telephony_t)user_data;
	CM_RETURN_IF_FAIL(telephony_handle);
	CM_RETURN_IF_FAIL(out);
	cm_telephony_call_type_e call_type = CM_TEL_CALL_TYPE_CS_VOICE;
	cm_telephony_call_direction_e call_direction = CM_TEL_CALL_DIRECTION_MO;
	cm_telephony_call_data_t *call_data = NULL;
	cm_telephony_call_state_e call_state = CM_TEL_CALL_STATE_MAX;

	_callmgr_telephony_get_call_by_call_id(telephony_handle, out->CallHandle, &call_data);
	if (call_data == NULL) {
		if (out->CallType == TAPI_CALL_TYPE_VOICE) {
			call_type = CM_TEL_CALL_TYPE_CS_VOICE;
		} else if (out->CallType == TAPI_CALL_TYPE_DATA) {
			call_type = CM_TEL_CALL_TYPE_CS_VIDEO;
		} else if (out->CallType == TAPI_CALL_TYPE_E911) {
			call_type = CM_TEL_CALL_TYPE_E911;
		} else {
			err("Unhandled Call type [%d]", out->CallType);
		}

		if (out->bMoCall == TRUE) {
			call_direction = CM_TEL_CALL_DIRECTION_MO;
		} else {
			call_direction = CM_TEL_CALL_DIRECTION_MT;
		}

		_callmgr_telephony_call_new(telephony_handle, call_type, call_direction, out->pNumber, &call_data);
	}

	CM_RETURN_IF_FAIL(call_data);
	__callmgr_telephony_set_call_id(call_data, out->CallHandle);
	__callmgr_telephony_set_conference_call_state(call_data, out->bConferenceState);

	dbg("call state [%d]", out->CallState);
	switch (out->CallState) {
		case TAPI_CALL_STATE_IDLE:
			call_state = CM_TEL_CALL_STATE_IDLE;
			break;
		case TAPI_CALL_STATE_ACTIVE:
			call_state = CM_TEL_CALL_STATE_ACTIVE;
			break;
		case TAPI_CALL_STATE_HELD:
			call_state = CM_TEL_CALL_STATE_HELD;
			break;
		case TAPI_CALL_STATE_DIALING:
			call_state = CM_TEL_CALL_STATE_DIALING;
			break;
		case TAPI_CALL_STATE_ALERT:
			call_state = CM_TEL_CALL_STATE_ALERT;
			break;
		case TAPI_CALL_STATE_INCOMING:
		case TAPI_CALL_STATE_WAITING:
			call_state = CM_TEL_CALL_STATE_INCOMING;
			break;
		default:
			err("should not reach here");
			break;
	}
	__callmgr_telephony_set_call_state(call_data, call_state);

	return;
}

static int __callmgr_telephony_get_all_call_list(callmgr_telephony_t telephony_handle)
{
	dbg("__callmgr_telephony_get_all_call_list()");
	int result = TAPI_API_SUCCESS;
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(telephony_handle->active_sim_slot < telephony_handle->modem_cnt, -1);

	/* Get call list from telephony */
	result = tel_get_call_status_all(telephony_handle->multi_handles[telephony_handle->active_sim_slot], __callmgr_telephony_all_call_list_cb, telephony_handle);
	if (result != TAPI_API_SUCCESS) {
		err("tel_get_call_status_all() get failed with err[%d]", result);
		return -1;
	}

	return 0;
}

static int __callmgr_telephony_set_call_id(cm_telephony_call_data_t *call, unsigned int call_id)
{
	dbg("__callmgr_telephony_set_call_id(), call ID[%d]", call_id);
	CM_RETURN_VAL_IF_FAIL(call, -1);
	call->call_id = call_id;
	return 0;
}
static int __callmgr_telephony_set_call_state(cm_telephony_call_data_t *call, cm_telephony_call_state_e state)
{
	dbg("__callmgr_telephony_set_call_state(), state[%d]", state);
	CM_RETURN_VAL_IF_FAIL(call, -1);
	call->call_state = state;
	return 0;
}

static int __callmgr_telephony_set_conference_call_state(cm_telephony_call_data_t *call, gboolean bConferenceState)
{
	dbg("__callmgr_telephony_set_conference_call(), bConferenceState[%d]", bConferenceState);
	CM_RETURN_VAL_IF_FAIL(call, -1);
	call->is_conference = bConferenceState;
	return 0;
}

static int __callmgr_telephony_set_name_mode(cm_telephony_call_data_t *call, cm_telephony_name_mode_e name_mode)
{
	dbg("__callmgr_telephony_set_name_mode(), name_mode[%d]", name_mode);
	CM_RETURN_VAL_IF_FAIL(call, -1);
	call->name_mode = name_mode;
	return 0;
}

int _callmgr_telephony_set_calling_name(cm_telephony_call_data_t *call, char * calling_name)
{
	CM_RETURN_VAL_IF_FAIL(call, -1);

	g_free(call->calling_name);
	call->calling_name = NULL;

	if (calling_name != NULL) {
		call->calling_name = g_strdup(calling_name);
	}
	return 0;
}

int _callmgr_telephony_get_calling_name(cm_telephony_call_data_t *call, char **calling_name)
{
	CM_RETURN_VAL_IF_FAIL(call, -1);
	CM_RETURN_VAL_IF_FAIL(calling_name, -1);
	*calling_name = g_strdup(call->calling_name);
	return 0;
}

int _callmgr_telephony_set_ecc_category(cm_telephony_call_data_t *call, int ecc_category)
{
	CM_RETURN_VAL_IF_FAIL(call, -1);
	call->ecc_category = ecc_category;
	return 0;
}

static int __callmgr_telephony_set_start_time(cm_telephony_call_data_t *call)
{
	CM_RETURN_VAL_IF_FAIL(call, -1);
	if (call->start_time == 0) {
		struct sysinfo start_time_info;
		if (sysinfo(&start_time_info) == 0) {
			info("StartTime: %ld secs", start_time_info.uptime);
			call->start_time = start_time_info.uptime;
			return 0;
		}
	}
	return -1;
}

static int __callmgr_telephony_set_retreive_flag(cm_telephony_call_data_t *call, gboolean is_set)
{
	CM_RETURN_VAL_IF_FAIL(call, -1);
	call->retreive_flag = is_set;
	warn("retreive flag [%d]", call->retreive_flag);
	return 0;
}

static int __callmgr_telephony_set_end_cause(cm_telephony_call_data_t *call, cm_telephony_end_cause_type_e cause)
{
	CM_RETURN_VAL_IF_FAIL(call, -1);
	call->end_cause = cause;
	warn("end_cause [%d]", call->end_cause);
	return 0;
}

int _callmgr_telephony_get_call_id(cm_telephony_call_data_t *call, unsigned int *call_id)
{
	CM_RETURN_VAL_IF_FAIL(call, -1);
	CM_RETURN_VAL_IF_FAIL(call_id, -1);
	*call_id = call->call_id;
	return 0;
}

int _callmgr_telephony_get_call_direction(cm_telephony_call_data_t *call, cm_telephony_call_direction_e *call_direction)
{
	CM_RETURN_VAL_IF_FAIL(call, -1);
	CM_RETURN_VAL_IF_FAIL(call_direction, -1);
	*call_direction = call->call_direction;
	return 0;
}

int _callmgr_telephony_set_call_number(cm_telephony_call_data_t *call, char *call_number)
{
	CM_RETURN_VAL_IF_FAIL(call, -1);

	g_free(call->call_number);
	call->call_number = NULL;

	if (call_number != NULL) {
		call->call_number = g_strdup(call_number);
	}
	return 0;
}

int _callmgr_telephony_get_call_number(cm_telephony_call_data_t *call, char **call_number)
{
	CM_RETURN_VAL_IF_FAIL(call, -1);
	CM_RETURN_VAL_IF_FAIL(call_number, -1);
	if (call->call_number) {
		*call_number = g_strdup(call->call_number);
	} else {
		*call_number = NULL;
	}
	return 0;
}

int _callmgr_telephony_set_dtmf_number(cm_telephony_call_data_t *call, char *dtmf_number)
{
	CM_RETURN_VAL_IF_FAIL(call, -1);

	g_free(call->dtmf_number);
	call->dtmf_number = NULL;

	if (dtmf_number != NULL) {
		call->dtmf_number = g_strdup(dtmf_number);
	}
	return 0;
}

int _callmgr_telephony_get_dtmf_number(cm_telephony_call_data_t *call, char **dtmf_number)
{
	CM_RETURN_VAL_IF_FAIL(call, -1);
	CM_RETURN_VAL_IF_FAIL(dtmf_number, -1);
	*dtmf_number = g_strdup(call->dtmf_number);
	return 0;
}

int _callmgr_telephony_get_call_type(cm_telephony_call_data_t *call, cm_telephony_call_type_e *call_type)
{
	CM_RETURN_VAL_IF_FAIL(call, -1);
	CM_RETURN_VAL_IF_FAIL(call_type, -1);
	*call_type = call->call_type;
	return 0;
}

int _callmgr_telephony_get_call_state(cm_telephony_call_data_t *call, cm_telephony_call_state_e *call_state)
{
	CM_RETURN_VAL_IF_FAIL(call, -1);
	CM_RETURN_VAL_IF_FAIL(call_state, -1);
	*call_state = call->call_state;
	return 0;
}

int _callmgr_telephony_get_is_conference(cm_telephony_call_data_t *call, gboolean *is_conference)
{
	CM_RETURN_VAL_IF_FAIL(call, -1);
	CM_RETURN_VAL_IF_FAIL(is_conference, -1);
	*is_conference = call->is_conference;
	return 0;
}

int _callmgr_telephony_get_conference_call_count(callmgr_telephony_t telephony_handle, int *member_cnt)
{
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(member_cnt, -1);
	GSList *call_list = telephony_handle->modem_info[telephony_handle->active_sim_slot].call_list;
	cm_telephony_call_data_t *call = NULL;
	int cnt = 0;

	while (call_list) {
		call = (cm_telephony_call_data_t*)call_list->data;
		if ((call) && (call->is_conference)) {
			cnt++;
		}
		call_list = g_slist_next(call_list);
	}
	*member_cnt = cnt;
	info("%d conference call exist", cnt);
	return 0;
}

int _callmgr_telephony_get_is_ecc(cm_telephony_call_data_t *call, gboolean *is_ecc)
{
	CM_RETURN_VAL_IF_FAIL(call, -1);
	CM_RETURN_VAL_IF_FAIL(is_ecc, -1);

	if (call->call_type == CM_TEL_CALL_TYPE_E911) {
		*is_ecc = TRUE;
	} else {
		*is_ecc = FALSE;
	}
	return 0;
}

int _callmgr_telephony_get_call_start_time(cm_telephony_call_data_t *call, long *start_time)
{
	CM_RETURN_VAL_IF_FAIL(call, -1);
	CM_RETURN_VAL_IF_FAIL(start_time, -1);
	*start_time = call->start_time;
	return 0;
}

int _callmgr_telephony_get_call_end_cause(cm_telephony_call_data_t *call, cm_telephony_end_cause_type_e *cause)
{
	CM_RETURN_VAL_IF_FAIL(call, -1);
	CM_RETURN_VAL_IF_FAIL(cause, -1);
	*cause = call->end_cause;
	return 0;
}

int _callmgr_telephony_get_call_name_mode(cm_telephony_call_data_t *call, cm_telephony_name_mode_e *name_mode)
{
	CM_RETURN_VAL_IF_FAIL(call, -1);
	CM_RETURN_VAL_IF_FAIL(name_mode, -1);
	*name_mode = call->name_mode;
	return 0;
}

int _callmgr_telephony_has_call_by_state(callmgr_telephony_t telephony_handle, cm_telephony_call_state_e state, gboolean *b_has_call)
{
	cm_telephony_call_data_t *call = NULL;
	GSList *call_list = NULL;
	int list_len = -1;
	int idx = -1;
	gboolean b_found_call = FALSE;
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);

	call_list = telephony_handle->modem_info[telephony_handle->active_sim_slot].call_list;
	list_len = g_slist_length(call_list);
	for (idx = 0; idx < list_len; idx++) {
		call = g_slist_nth_data(call_list, idx);
		if (call->call_state == state) {
			b_found_call = TRUE;
		}
	}
	*b_has_call = b_found_call;
	return 0;
}

int _callmgr_telephony_get_call_by_state(callmgr_telephony_t telephony_handle, cm_telephony_call_state_e state, cm_telephony_call_data_t **call_data_out)
{
	struct __modem_info *modem_info = NULL;
	cm_telephony_call_data_t *call = NULL;
	int list_len = -1;
	int idx = -1;
	dbg("_callmgr_telephony_get_call_by_state :%d", state);
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(call_data_out, -1);

	modem_info = &telephony_handle->modem_info[telephony_handle->active_sim_slot];
	list_len = g_slist_length(modem_info->call_list);
	for (idx = 0; idx < list_len; idx++) {
		call = g_slist_nth_data(modem_info->call_list, idx);
		if (call->call_state == state) {
			*call_data_out = call;
			return 0;
		}
	}
	return -1;
}

static int __callmgr_telephony_end_all_call_by_state(callmgr_telephony_t telephony_handle, cm_telephony_call_state_e state)
{
	struct __modem_info *modem_info = NULL;
	cm_telephony_call_data_t *call = NULL;
	int list_len = -1;
	int idx = -1;
	dbg("__callmgr_telephony_end_all_call_by_state :%d", state);
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);

	modem_info = &telephony_handle->modem_info[telephony_handle->active_sim_slot];
	list_len = g_slist_length(modem_info->call_list);
	for (idx = 0; idx < list_len; idx++) {
		call = g_slist_nth_data(modem_info->call_list, idx);
		if (call->call_state == state) {
			_callmgr_telephony_end_call(telephony_handle, call->call_id, CM_TEL_CALL_RELEASE_TYPE_BY_CALL_HANDLE);
		}
	}
	return -1;
}

static int __callmgr_telephony_get_call_to_be_retreived(callmgr_telephony_t telephony_handle, cm_telephony_call_data_t **call_data_out)
{
	struct __modem_info *modem_info = NULL;
	cm_telephony_call_data_t *call = NULL;
	int list_len = -1;
	int idx = -1;
	dbg("__callmgr_telephony_get_call_to_be_retreived()");
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(call_data_out, -1);

	modem_info = &telephony_handle->modem_info[telephony_handle->active_sim_slot];
	list_len = g_slist_length(modem_info->call_list);
	for (idx = 0; idx < list_len; idx++) {
		call = g_slist_nth_data(modem_info->call_list, idx);
		if (call->retreive_flag == TRUE) {
			*call_data_out = call;
			return 0;
		}
	}
	return -1;
}

int _callmgr_telephony_get_video_call(callmgr_telephony_t telephony_handle, cm_telephony_call_data_t **call_data_out)
{
	cm_telephony_call_data_t *call = NULL;
	GSList *call_list = NULL;
	int list_len = -1;
	int idx = -1;
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(call_data_out, -1);

	call_list = telephony_handle->modem_info[telephony_handle->active_sim_slot].call_list;
	list_len = g_slist_length(call_list);
	for (idx = 0; idx < list_len; idx++) {
		call = g_slist_nth_data(call_list, idx);
		if (call->call_type == CM_TEL_CALL_TYPE_CS_VIDEO || call->call_type == CM_TEL_CALL_TYPE_PS_VIDEO) {
			info("Video Call found. [%d]", call->call_id);
			*call_data_out = call;
			return 0;
		}
	}
	return -1;
}

int _callmgr_telephony_get_voice_call(callmgr_telephony_t telephony_handle, cm_telephony_call_data_t **call_data_out)
{
	cm_telephony_call_data_t *call = NULL;
	GSList *call_list = NULL;
	int list_len = -1;
	int idx = -1;
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(call_data_out, -1);

	call_list = telephony_handle->modem_info[telephony_handle->active_sim_slot].call_list;
	list_len = g_slist_length(call_list);
	for (idx = 0; idx < list_len; idx++) {
		call = g_slist_nth_data(call_list, idx);
		if (call->call_type == CM_TEL_CALL_TYPE_CS_VOICE || call->call_type == CM_TEL_CALL_TYPE_PS_VOICE || call->call_type == CM_TEL_CALL_TYPE_E911) {
			info("Voice Call found. [%d]", call->call_id);
			*call_data_out = call;
			return 0;
		}
	}
	return -1;
}

int _callmgr_telephony_get_sat_setup_call_number(callmgr_telephony_t telephony_handle, char **call_number)
{
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(call_number, -1);

	if (telephony_handle->modem_info[telephony_handle->active_sim_slot].sat_data.setup_call) {
		*call_number = g_strdup(telephony_handle->modem_info[telephony_handle->active_sim_slot].sat_data.setup_call->callNumber.string);
	} else {
		*call_number = NULL;
	}
	return 0;
}

int _callmgr_telephony_get_sat_setup_call_name(callmgr_telephony_t telephony_handle, char **call_name)
{
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(call_name, -1);
	if (telephony_handle->modem_info[telephony_handle->active_sim_slot].sat_data.setup_call) {
		*call_name = g_strdup(telephony_handle->modem_info[telephony_handle->active_sim_slot].sat_data.setup_call->dispText.string);
	} else {
		*call_name = NULL;
	}
	return 0;
}

int _callmgr_telephony_get_sat_event_type(callmgr_telephony_t telephony_handle, cm_telephony_sat_event_type_e *sat_event_type)
{
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	*sat_event_type = telephony_handle->modem_info[telephony_handle->active_sim_slot].sat_data.event_type;
	return 0;
}

int _callmgr_telephony_get_sat_originated_call(callmgr_telephony_t telephony_handle, cm_telephony_call_data_t **call_data_out)
{
	cm_telephony_call_data_t *call = NULL;
	GSList *call_list = NULL;
	int list_len = -1;
	int idx = -1;
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(call_data_out, -1);

	call_list = telephony_handle->modem_info[telephony_handle->active_sim_slot].call_list;
	list_len = g_slist_length(call_list);
	for (idx = 0; idx < list_len; idx++) {
		call = g_slist_nth_data(call_list, idx);
		if (call->is_sat_originated_call) {
			info("Call found. id[%d], name[%s], number[%s]", call->call_id, call->calling_name, call->call_number);
			*call_data_out = call;
			return 0;
		}
	}
	return -1;
}

int _callmgr_telephony_is_sat_originated_call(cm_telephony_call_data_t *call, gboolean *is_sat_originated_call)
{
	CM_RETURN_VAL_IF_FAIL(call, -1);
	CM_RETURN_VAL_IF_FAIL(is_sat_originated_call, -1);
	if (call->is_sat_originated_call)
		*is_sat_originated_call = TRUE;
	else
		*is_sat_originated_call = FALSE;
	return 0;
}

int _callmgr_telephony_set_sat_originated_call_flag(cm_telephony_call_data_t *call, gboolean is_sat_originated_call)
{
	CM_RETURN_VAL_IF_FAIL(call, -1);
	call->is_sat_originated_call = is_sat_originated_call;
	return 0;
}

int _callmgr_telephony_get_call_by_call_id(callmgr_telephony_t telephony_handle, unsigned int call_id, cm_telephony_call_data_t **call_data_out)
{
	cm_telephony_call_data_t *call = NULL;
	GSList *call_list = NULL;
	int list_len = -1;
	int idx = -1;
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(call_data_out, -1);

	call_list = telephony_handle->modem_info[telephony_handle->active_sim_slot].call_list;
	list_len = g_slist_length(call_list);
	for (idx = 0; idx < list_len; idx++) {
		call = g_slist_nth_data(call_list, idx);
		if (call->call_id == call_id) {
			info("Call found[%d]", call->call_id);
			*call_data_out = call;
			return 0;
		}
	}
	return -1;
}

int _callmgr_telephony_get_call_count(callmgr_telephony_t telephony_handle, int *call_count_out)
{
	GSList *call_list = NULL;
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(telephony_handle->active_sim_slot < telephony_handle->modem_cnt, -1);
	CM_RETURN_VAL_IF_FAIL(call_count_out, -1);

	call_list = telephony_handle->modem_info[telephony_handle->active_sim_slot].call_list;
	*call_count_out = g_slist_length(call_list);
	dbg("call_cnt :%d", *call_count_out);
	return 0;
}

int _callmgr_telephony_get_card_type(callmgr_telephony_t telephony_handle, cm_telephony_card_type_e *card_type_out)
{
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(card_type_out, -1);
	*card_type_out = telephony_handle->modem_info[telephony_handle->active_sim_slot].card_type;
	return 0;
}

int _callmgr_telephony_get_network_status(callmgr_telephony_t telephony_handle, cm_network_status_e *netk_status_out)
{
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(netk_status_out, -1);
	*netk_status_out = telephony_handle->modem_info[telephony_handle->active_sim_slot].network_status;
	return 0;
}

int _callmgr_telephony_get_modem_count(callmgr_telephony_t telephony_handle, int *modem_cnt_out)
{
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(modem_cnt_out, -1);
	*modem_cnt_out = telephony_handle->modem_cnt;
	return 0;
}

int _callmgr_telephony_get_preferred_sim_type(callmgr_telephony_t telephony_handle, cm_telepony_preferred_sim_type_e *preferred_sim)
{
	TelCallPreferredVoiceSubs_t preferred_subscription = TAPI_CALL_PREFERRED_VOICE_SUBS_UNKNOWN;
	int ret = TAPI_API_SUCCESS;
	dbg("_callmgr_telephony_get_preferred_sim_slot");
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);

	ret = tel_get_call_preferred_voice_subscription(telephony_handle->multi_handles[0], &preferred_subscription);
	if (ret != TAPI_API_SUCCESS) {
		err("tel_get_call_preferred_voice_subscription get failed with err(%d)", ret);\
		return -1;
	} else {
		info("preferred_subscription: %d", preferred_subscription);
		switch (preferred_subscription) {
			case TAPI_CALL_PREFERRED_VOICE_SUBS_CURRENT_NETWORK:
				*preferred_sim = CM_TEL_PREFERRED_CURRENT_NETWORK_E;
				break;
			case TAPI_CALL_PREFERRED_VOICE_SUBS_ASK_ALWAYS:
				*preferred_sim = CM_TEL_PREFERRED_ALWAYS_ASK_E;
				break;
			case TAPI_CALL_PREFERRED_VOICE_SUBS_SIM1:
				*preferred_sim = CM_TEL_PREFERRED_SIM_1_E;
				break;
			case TAPI_CALL_PREFERRED_VOICE_SUBS_SIM2:
				*preferred_sim = CM_TEL_PREFERRED_SIM_2_E;
				break;
			default:
				*preferred_sim = CM_TEL_PREFERRED_UNKNOWN_E;
				break;
		}
	}
	return 0;
}

int _callmgr_telephony_is_sim_initialized(callmgr_telephony_t telephony_handle, cm_telepony_sim_slot_type_e sim_slot, gboolean *is_initialized)
{
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(sim_slot < telephony_handle->modem_cnt, -1);
	dbg("_callmgr_telephony_is_sim_initialized()");
	*is_initialized = telephony_handle->modem_info[sim_slot].is_sim_initialized;
	return 0;
}

int _callmgr_telephony_is_in_service(callmgr_telephony_t telephony_handle, cm_telepony_sim_slot_type_e sim_slot, gboolean *is_in_service)
{
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(sim_slot < telephony_handle->modem_cnt, -1);
	dbg("_callmgr_telephony_is_in_service()");
	switch (telephony_handle->modem_info[sim_slot].network_status) {
		case CM_NETWORK_STATUS_OUT_OF_SERVICE:
		case CM_NETWORK_STATUS_EMERGENCY_ONLY:
			*is_in_service = FALSE;
			break;
		default:
			*is_in_service = TRUE;
			break;
	}
	return 0;
}

int _callmgr_telephony_is_no_sim(callmgr_telephony_t telephony_handle, gboolean *is_no_sim)
{
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(telephony_handle->active_sim_slot < telephony_handle->modem_cnt, -1);
	*is_no_sim = telephony_handle->modem_info[telephony_handle->active_sim_slot].is_no_sim;
	return 0;
}

int _callmgr_telephony_is_phone_initialized(callmgr_telephony_t telephony_handle, gboolean *is_initialized)
{
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(telephony_handle->active_sim_slot < telephony_handle->modem_cnt, -1);
	dbg("_callmgr_telephony_is_phone_initialized()");
	*is_initialized = telephony_handle->modem_info[telephony_handle->active_sim_slot].is_phone_initialized;
	return 0;
}

int _callmgr_telephony_is_flight_mode_enabled(callmgr_telephony_t telephony_handle, gboolean *is_enabled)
{
	int ret = -1;
	gboolean flight_mode = -1;
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	dbg("_callmgr_telephony_is_flight_mode_enabled()");

	ret = vconf_get_bool(VCONFKEY_TELEPHONY_FLIGHT_MODE, &flight_mode);
	if (ret < 0) {
		err("vconf_get_int failed.. ret_val: %d", ret);
		return -1;
	}

	*is_enabled = flight_mode;
	return 0;
}

int _callmgr_telephony_is_cs_available(callmgr_telephony_t telephony_handle, gboolean *is_available)
{
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(telephony_handle->active_sim_slot < telephony_handle->modem_cnt, -1);
	dbg("_callmgr_telephony_is_phone_initialized()");
	*is_available = telephony_handle->modem_info[telephony_handle->active_sim_slot].is_cs_available;
	return 0;
}

int _callmgr_telephony_get_call_list(callmgr_telephony_t telephony_handle, GSList **call_list)
{
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(call_list, -1);
	dbg("callmgr_telephony_get_call_list");

	*call_list = telephony_handle->modem_info[telephony_handle->active_sim_slot].call_list;
	return 0;
}

static void __callmgr_telephony_set_audio_path_resp_cb(TapiHandle *handle, int result, void *tapi_data, void *user_data)
{
	dbg("Result : %d", result);
}

int _callmgr_telephony_set_audio_path(callmgr_telephony_t telephony_handle, cm_telephony_audio_path_type_e path, gboolean is_extra_vol)
{
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(telephony_handle->active_sim_slot < telephony_handle->modem_cnt, -1);

	dbg(">>");

	TelCallSoundPathInfo_t tapi_sound_path;
	TapiResult_t tapi_error = TAPI_API_SUCCESS;

	tapi_sound_path.path = path;

	warn("tapi_sound_path: %d(extra voluem: %d)", tapi_sound_path.path, is_extra_vol);

	if ((is_extra_vol)
		&& ((tapi_sound_path.path == TAPI_SOUND_PATH_HANDSET)
		|| (tapi_sound_path.path == TAPI_SOUND_PATH_SPK_PHONE))) {
			tapi_sound_path.ex_volume = TAPI_SOUND_EX_VOLUME_ON;
	} else {
		tapi_sound_path.ex_volume = TAPI_SOUND_EX_VOLUME_OFF;
	}

	tapi_error = tel_set_call_sound_path(telephony_handle->multi_handles[telephony_handle->active_sim_slot], &tapi_sound_path, __callmgr_telephony_set_audio_path_resp_cb, NULL);
	if (tapi_error != TAPI_API_SUCCESS) {
		warn("tel_set_sound_path error: %d", tapi_error);
		return -1;
	}

	return 0;
}

static void __callmgr_telephony_set_audio_tx_mute_resp_cb(TapiHandle *handle, int result, void *tapi_data, void *user_data)
{
	dbg("Result : %d", result);
}

int _callmgr_telephony_set_audio_tx_mute(callmgr_telephony_t telephony_handle, gboolean is_mute_state)
{
	int ret = 0;
	TelSoundMuteStatus_t mute_set = TAPI_SOUND_MUTE_STATUS_OFF;
	mute_set = (TRUE == is_mute_state) ? TAPI_SOUND_MUTE_STATUS_ON : TAPI_SOUND_MUTE_STATUS_OFF;
	dbg("mute_set : %d", mute_set);

	ret = tel_set_call_mute_status(telephony_handle->multi_handles[telephony_handle->active_sim_slot], mute_set, TAPI_SOUND_MUTE_PATH_TX , __callmgr_telephony_set_audio_tx_mute_resp_cb, NULL);
	if (ret != TAPI_API_SUCCESS) {
		warn("tel_set_call_mute_status error: %d", ret);
		return -1;
	}

	return 0;
}

int _callmgr_telephony_reset_call_start_time(cm_telephony_call_data_t *call)
{
	struct sysinfo start_time_info;
	CM_RETURN_VAL_IF_FAIL(call, -1);

	if (sysinfo(&start_time_info) == 0) {
		info("ResetTime: %ld secs", start_time_info.uptime);
		call->start_time = start_time_info.uptime;
		return 0;
	}

	return -1;
}


int _callmgr_telephony_get_imsi_mcc_mnc(callmgr_telephony_t telephony_handle, unsigned long *mcc, unsigned long *mnc)
{
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(telephony_handle->active_sim_slot < telephony_handle->modem_cnt, -1);
	dbg("_callmgr_telephony_get_imsi_mcc_mnc()");
	*mcc = telephony_handle->modem_info[telephony_handle->active_sim_slot].sim_mcc;
	*mnc = telephony_handle->modem_info[telephony_handle->active_sim_slot].sim_mnc;
	return 0;
}

int _callmgr_telephony_get_mcc_mnc(callmgr_telephony_t telephony_handle, unsigned long *mcc, unsigned long *mnc)
{
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(telephony_handle->active_sim_slot < telephony_handle->modem_cnt, -1);
	dbg("_callmgr_telephony_get_mcc_mnc()");
	*mcc = telephony_handle->modem_info[telephony_handle->active_sim_slot].net_mcc;
	*mnc = telephony_handle->modem_info[telephony_handle->active_sim_slot].net_mnc;
	return 0;
}

static int __callmgr_telephony_set_sim_slot_for_ecc(callmgr_telephony_t telephony_handle)
{
	int sim_init_cnt = 0;
	cm_telepony_preferred_sim_type_e pref_sim_type = CM_TEL_PREFERRED_UNKNOWN_E;
	cm_telepony_sim_slot_type_e preferred_sim = CM_TELEPHONY_SIM_UNKNOWN;
	int idx = -1;
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);

	for (idx = 0; idx < telephony_handle->modem_cnt; idx++) {
		if (telephony_handle->modem_info[idx].is_sim_initialized == TRUE) {
			sim_init_cnt++;
		}
	}

	_callmgr_telephony_set_active_sim_slot(telephony_handle, CM_TELEPHONY_SIM_UNKNOWN);
	if (sim_init_cnt > 1) {
		_callmgr_telephony_get_preferred_sim_type(telephony_handle, &pref_sim_type);
		if (pref_sim_type == CM_TELEPHONY_SIM_2) {
			preferred_sim = CM_TELEPHONY_SIM_2;
		} else {
			preferred_sim = CM_TELEPHONY_SIM_1;
		}
	} else if (sim_init_cnt == 1) {
		if (telephony_handle->modem_info[CM_TELEPHONY_SIM_1].is_sim_initialized) {
			preferred_sim = CM_TELEPHONY_SIM_1;
		} else {
			preferred_sim = CM_TELEPHONY_SIM_2;
		}
	} else {
		err("SIM not initialized!!");
		preferred_sim = CM_TELEPHONY_SIM_1;
	}

	if (telephony_handle->modem_info[preferred_sim].network_status != CM_NETWORK_STATUS_OUT_OF_SERVICE) {
		_callmgr_telephony_set_active_sim_slot(telephony_handle, preferred_sim);
		return 0;
	}

	if (telephony_handle->modem_cnt > 1) {
		cm_telepony_sim_slot_type_e alternative_slot = CM_TELEPHONY_SIM_UNKNOWN;
		if (preferred_sim == CM_TELEPHONY_SIM_1) {
			alternative_slot = CM_TELEPHONY_SIM_2;
		} else {
			alternative_slot = CM_TELEPHONY_SIM_1;
		}
		if (telephony_handle->modem_info[alternative_slot].network_status != CM_NETWORK_STATUS_OUT_OF_SERVICE) {
			_callmgr_telephony_set_active_sim_slot(telephony_handle, alternative_slot);
			return 0;
		}
	}

	err("All modem is no service status! Use SIM 1 slot!!");
	_callmgr_telephony_set_active_sim_slot(telephony_handle, CM_TELEPHONY_SIM_1);
	return -1;
}


int _callmgr_telephony_check_ecc_number(callmgr_telephony_t telephony_handle, const char *pNumber, gboolean *is_ecc, int *ecc_category)
{
	gboolean is_no_sim = FALSE;
	gboolean is_sim_ecc = FALSE;
	int ret = 0;
	int salescode = 0;
	int call_count = 0;
	int ret_val = 0;

	unsigned long mcc = 0;	/* for checking Emergency number for each country */
	unsigned long mnc = 0;	/* for checking Emergency number for each operator */

	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(pNumber, -1);
	CM_RETURN_VAL_IF_FAIL(is_ecc, -1);
	CM_RETURN_VAL_IF_FAIL(ecc_category, -1);

	/* Init out parameters*/
	*is_ecc = FALSE;
	*ecc_category = 0;

	_callmgr_telephony_get_call_count(telephony_handle, &call_count);
	if (call_count == 0) {
		__callmgr_telephony_set_sim_slot_for_ecc(telephony_handle);
	}
	_callmgr_vconf_get_salescode(&salescode);
	if ((salescode == CALL_CSC_KT) || (salescode == CALL_CSC_SKT)) {
		warn("KOR");
		/* Need to remove below code. */
		/*
		ret = _callmgr_telephony_check_ecc_for_kor(telephony_handle, pNumber, is_ecc, ecc_category);
		ret_val = ret;
		goto FINISH;
		*/
	}

	_callmgr_telephony_is_no_sim(telephony_handle, &is_no_sim);
	if (is_no_sim == TRUE) {
		ret = _callmgr_telephony_check_ecc_nosim_mcc_mnc(telephony_handle, pNumber, is_ecc, ecc_category);
		if (ret == -1) {
			err("Check return value");
		} else if (ret == -2) {
			_callmgr_telephony_check_ecc_nosim_3gpp(telephony_handle, pNumber, is_ecc, ecc_category);
		}
		ret_val = 0;
		goto FINISH;
	}

	_callmgr_telephony_get_mcc_mnc(telephony_handle, &mcc, &mnc);

	_callmgr_telephony_check_sim_ecc(telephony_handle, pNumber, &is_sim_ecc, ecc_category);
	if (is_sim_ecc == TRUE) {
		*is_ecc = TRUE;
		ret_val = 0;
		goto FINISH;
	}

	/* There is no ecc number in the SIM card. */
	ret = _callmgr_telephony_check_ecc_mcc_mnc(telephony_handle, pNumber, is_ecc, ecc_category);
	if (ret == -1) {
		err("Check return value");
	} else if (ret == -2) {
		_callmgr_telephony_check_ecc_3gpp(telephony_handle, pNumber, is_ecc, ecc_category);
	}


FINISH:
	if ((*is_ecc == FALSE) && (call_count == 0)) {
		warn("reset active SIM slot");
		_callmgr_telephony_set_active_sim_slot(telephony_handle, CM_TELEPHONY_SIM_UNKNOWN);
	}
	return ret_val;
}


int _callmgr_telephony_check_sim_ecc(callmgr_telephony_t telephony_handle, const char *pNumber, gboolean *is_sim_ecc, int *ecc_category)
{
	cm_telephony_card_type_e simcard_type = CM_CARD_TYPE_UNKNOWN;
	struct __modem_info *modem_info = NULL;
	int i = 0;

	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(telephony_handle->active_sim_slot < telephony_handle->modem_cnt, -1);
	CM_RETURN_VAL_IF_FAIL(pNumber, -1);
	CM_RETURN_VAL_IF_FAIL(is_sim_ecc, -1);
	CM_RETURN_VAL_IF_FAIL(ecc_category, -1);

	*is_sim_ecc = FALSE;	/*Init*/
	modem_info = &telephony_handle->modem_info[telephony_handle->active_sim_slot];
	simcard_type = modem_info->card_type;
	dbg("simcard_type : %d", simcard_type);

	/*_callmgr_telephony_get_imsi_mcc_mnc(telephony_handle, &imsi_mcc, &imsi_mnc);*/

	switch (simcard_type) {
		case CM_CARD_TYPE_GSM:
			{
				dbg("[SimCardType=SIM_CARD_TYPE_GSM]");

				/*TAPI api Compliance */
				/*To get Emergency data of 2G */
				if (modem_info->sim_ecc_list.ecc_count > 0) {
					/*SEC_DBG("GSM pNumber ecc1(%s) ecc2(%s) ecc3(%s) ecc4(%s) ecc5(%s)",
							   sim_ecc_list.list[0].number, sim_ecc_list.list[1].number, sim_ecc_list.list[2].number, sim_ecc_list.list[3].number, sim_ecc_list.list[4].number);*/

					if ((g_strcmp0(pNumber, modem_info->sim_ecc_list.list[0].number) == 0)
						|| (g_strcmp0(pNumber, modem_info->sim_ecc_list.list[1].number) == 0)
						|| (g_strcmp0(pNumber, modem_info->sim_ecc_list.list[2].number) == 0)
						|| (g_strcmp0(pNumber, modem_info->sim_ecc_list.list[3].number) == 0)
						|| (g_strcmp0(pNumber, modem_info->sim_ecc_list.list[4].number) == 0)) {
						dbg("_callmgr_telephony_check_ecc_number: return TRUE");
						*is_sim_ecc = TRUE;
						return 0;
					}
				}
			}
			break;
		case CM_CARD_TYPE_USIM:
			{
				dbg("SimCardType=SIM_CARD_TYPE_USIM");

				if (modem_info->sim_ecc_list.ecc_count > 0) {
					for (i = 0; i < modem_info->sim_ecc_list.ecc_count; i++) {
						/*SEC_DBG("[ecc=%s, category:[%d]]", usim_ecc_list.list[i].number, usim_ecc_list.list[i].category);*/
						if (!g_strcmp0(pNumber, modem_info->sim_ecc_list.list[i].number)) {
							*ecc_category = modem_info->sim_ecc_list.list[i].category;
							dbg("uecc matched!!");
							*is_sim_ecc = TRUE;
							return 0;
						}
					}
				}
			}
			break;
		default:
			dbg("type : %d", simcard_type);
			break;
	}

	return -1;
}

static cm_telephony_incall_ss_type_e __callmgr_get_incall_ss_type(const char *number, unsigned int *selected_call_id)
{
	cm_telephony_incall_ss_type_e ss_type = CM_INCALL_SS_USSD_E;

	if (strlen(number) == 1) {
		switch (number[0]) {
		case '0':
			ss_type = CM_INCALL_SS_0_E;
			break;
		case '1':
			ss_type = CM_INCALL_SS_1_E;
			break;
		case '2':
			ss_type = CM_INCALL_SS_2_E;
			break;
		case '3':
			ss_type = CM_INCALL_SS_3_E;
			break;
		case '4':
			ss_type = CM_INCALL_SS_4_E;
			break;
		default:
			ss_type = CM_INCALL_SS_USSD_E;
			break;
		}
	} else if (strlen(number) == 2) {
		if ((number[0] == '1') && (number[1] > '0') && (number[1] < '8')) {
			*selected_call_id = atoi(number + 1);
			ss_type = CM_INCALL_SS_1X_E;
		}

		if ((number[0] == '2') && (number[1] > '0') && (number[1] < '8')) {
			*selected_call_id = atoi(number + 1);
			ss_type = CM_INCALL_SS_2X_E;
		}
	}

	info("type : %d, id : %d", ss_type, *selected_call_id);
	return ss_type;
}

int _callmgr_telephony_process_incall_ss(callmgr_telephony_t telephony_handle, const char *number)
{
	unsigned int selected_call_id = 0;
	int call_count = 0;
	cm_telephony_call_data_t *incom_call = NULL;
	cm_telephony_call_data_t *held_call = NULL;
	cm_telephony_call_data_t *active_call = NULL;
	cm_telephony_call_data_t *dial_call = NULL;
	cm_telephony_call_data_t *alert_call = NULL;

	cm_telephony_incall_ss_type_e ss_type = CM_INCALL_SS_USSD_E;

	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(number, -1);

	ss_type = __callmgr_get_incall_ss_type(number, &selected_call_id);

	if (_callmgr_telephony_get_call_by_state(telephony_handle, CM_TEL_CALL_STATE_INCOMING, &incom_call) < 0) {
		warn("_callmgr_telephony_get_call_by_state err");
	}

	if (_callmgr_telephony_get_call_by_state(telephony_handle, CM_TEL_CALL_STATE_DIALING, &dial_call) < 0) {
		warn("_callmgr_telephony_get_call_by_state err");
	}

	if (_callmgr_telephony_get_call_by_state(telephony_handle, CM_TEL_CALL_STATE_ALERT, &alert_call) < 0) {
		warn("_callmgr_telephony_get_call_by_state err");
	}

	if (_callmgr_telephony_get_call_by_state(telephony_handle, CM_TEL_CALL_STATE_ACTIVE, &active_call) < 0) {
		warn("_callmgr_telephony_get_call_by_state err");
	}

	if (_callmgr_telephony_get_call_by_state(telephony_handle, CM_TEL_CALL_STATE_HELD, &held_call) < 0) {
		warn("_callmgr_telephony_get_call_by_state err");
	}

	if (ss_type != CM_INCALL_SS_USSD_E) {
		switch (ss_type) {
			/* Releases all held calls or Set UDUB(User Determined User Busy) for a waiting call */
		case CM_INCALL_SS_0_E:
			{
				/* if an incoming call is activated, reject the incoming all  */
				if (incom_call) {
					_callmgr_telephony_reject_call(telephony_handle);
					return 0;
				} else if (held_call) {
					_callmgr_telephony_end_call(telephony_handle, held_call->call_id, CM_TEL_CALL_RELEASE_TYPE_ALL_HOLD_CALLS);
				} else {
					err("No hold calls");
					return -1;
				}
			}
			break;
		case CM_INCALL_SS_1_E:
			{
				if (incom_call) {
					/* Accept incoming call */
					_callmgr_telephony_answer_call(telephony_handle, CM_TEL_CALL_ANSWER_TYPE_RELEASE_ACTIVE_AND_ACCEPT);
					return 0;
				} else if (dial_call || alert_call) {
					if (dial_call) {
						selected_call_id = dial_call->call_id;
					} else {
						selected_call_id = alert_call->call_id;
					}
					_callmgr_telephony_end_call(telephony_handle, selected_call_id, CM_TEL_CALL_RELEASE_TYPE_BY_CALL_HANDLE);

					if (held_call) {
						__callmgr_telephony_set_retreive_flag(held_call, TRUE);
					}
				} else if (active_call) {
					_callmgr_telephony_end_call(telephony_handle, active_call->call_id, CM_TEL_CALL_RELEASE_TYPE_ALL_ACTIVE_CALLS);
					if (held_call) {
						__callmgr_telephony_set_retreive_flag(held_call, TRUE);
					}
				} else if (held_call) {
					if (_callmgr_telephony_active_call(telephony_handle) < 0) {
						err("retrieve err");
						return -1;
					}
					return 0;
				} else {
					err("No valid calls");
					return -1;
				}
			}
			break;
		case CM_INCALL_SS_1X_E:
			{
				if (incom_call) {
					err("No incom calls");
					return -1;
				} else if (active_call) {
					if (_callmgr_telephony_end_call(telephony_handle, selected_call_id, CM_TEL_CALL_RELEASE_TYPE_BY_CALL_HANDLE) < 0) {
						err("end call err");
						return -1;
					} else {
						return 0;
					}
				} else {
					err("No valid calls");
					return -1;
				}
			}
			break;
		case CM_INCALL_SS_2_E:
			{
				if (incom_call) {
					if (active_call && held_call) {
						err("No valid request");
						return -1;
					} else {
						_callmgr_telephony_answer_call(telephony_handle,  CM_TEL_CALL_ANSWER_TYPE_HOLD_ACTIVE_AND_ACCEPT);
					}
				} else if (alert_call || dial_call) {
					err("No valid request");
					return -1;
				} else if (active_call && held_call) {
					_callmgr_telephony_swap_call(telephony_handle);
				} else if (active_call) {
					_callmgr_telephony_hold_call(telephony_handle);
				} else if (held_call) {
					_callmgr_telephony_active_call(telephony_handle);
				} else {
					err("No valid request");
					return -1;
				}
			}
			break;
		case CM_INCALL_SS_2X_E:
			{
				if (incom_call || dial_call || alert_call) {
					info("No valid calls");
					return -1;
				} else if ((active_call != NULL) && (held_call == NULL)) {
					if (active_call->is_conference) {
						if (_callmgr_telephony_split_call(telephony_handle, selected_call_id) < 0) {
							err("_callmgr_telephony_split_call() failed");
							return -1;
						}
					}
				} else {
					info("No valid calls");
					return -1;
				}
			}
			break;
		case CM_INCALL_SS_3_E:
			{
				if (incom_call) {
					info("No valid calls");
					return -1;
				}

				if (active_call || held_call) {
					_callmgr_telephony_join_call(telephony_handle);
				} else {
					info("No valid calls");
					return -1;
				}
			}
			break;
		case CM_INCALL_SS_4_E:
			{
				cm_telephony_call_data_t *call_to_transfer = NULL;

				if (_callmgr_telephony_get_call_count(telephony_handle, &call_count) < 0) {
					err("get count err");
					return -1;
				}

				if (call_count != 2) {
					warn("Invalid call count");
					return -1;
				}

				if (held_call) {
					if (alert_call) {
						call_to_transfer = alert_call;
					} else if (active_call) {
						call_to_transfer = active_call;
					}
				} else {
					warn("no call in held state");
					return -1;
				}

				if (call_to_transfer) {
					if (_callmgr_telephony_transfer_call(telephony_handle, call_to_transfer) < 0) {
						info("Transfer err");
						return -1;
					} else {
						return 0;
					}
				} else {
					warn("Invalid call state");
					return -1;
				}
			}
			break;
		default:
			info("Invalid type");
			return -1;
		}

	} else {
		warn("USSD type");
		return -1;
	}

	return 0;
}

static gboolean __callmgr_telephony_disable_fm_timer_cb(gpointer data)
{
	callmgr_telephony_t telephony_handle = (callmgr_telephony_t)data;
	cm_network_status_e nw_status = CM_NETWORK_STATUS_OUT_OF_SERVICE;

	_callmgr_telephony_get_network_status(telephony_handle, &nw_status);

	dbg("nw_status(%d)", nw_status);

	if (nw_status == CM_NETWORK_STATUS_OUT_OF_SERVICE) {
		g_disable_fm_cnt++;
		if (g_disable_fm_cnt < 6) {
			return TRUE;
		}
	} else {
		cm_telephony_call_data_t *call = NULL;
		int ret = 0;
		ret = _callmgr_telephony_get_call_by_call_id(telephony_handle, NO_CALL_HANDLE, &call);

		if (call == NULL || ret < 0) {
			err("No valid call");
			if (telephony_handle->disable_fm_timer > 0) {
				g_source_remove(telephony_handle->disable_fm_timer);
				telephony_handle->disable_fm_timer = -1;
			}
			telephony_handle->cb_fn(CM_TELEPHONY_EVENT_FLIGHT_MODE_TIME_OVER_E, 0, telephony_handle->user_data);
			return FALSE;
		}

		if (nw_status == CM_NETWORK_STATUS_EMERGENCY_ONLY
			&& call->call_type != CM_TEL_CALL_TYPE_E911) {
			info("Not ECC");
			g_disable_fm_cnt++;
			if (g_disable_fm_cnt < 6) {
				return TRUE;
			}
		} else if (nw_status == CM_NETWORK_STATUS_IN_SERVICE_2G
			&& call->call_type == CM_TEL_CALL_TYPE_CS_VIDEO) {
			info("Video call");
			g_disable_fm_cnt++;
			if (g_disable_fm_cnt < 6) {
				return TRUE;
			}
		}
	}

	if (telephony_handle->disable_fm_timer > 0) {
		g_source_remove(telephony_handle->disable_fm_timer);
		telephony_handle->disable_fm_timer = -1;
	}

	telephony_handle->cb_fn(CM_TELEPHONY_EVENT_FLIGHT_MODE_TIME_OVER_E, 0, telephony_handle->user_data);

	return FALSE;
}

static void __callmgr_telephony_set_flight_mode(TapiHandle *handle, int result, void *data, void *user_data)
{
	callmgr_telephony_t telephony_handle = (callmgr_telephony_t)user_data;
	int ret = 0;
	cm_telephony_call_data_t *call = NULL;

	info("result : %d", result);
	if (result == TAPI_POWER_FLIGHT_MODE_RESP_OFF) {
		info("flight mode is disabled");
		telephony_handle->disable_fm_timer = g_timeout_add(__FLIGHT_MODE_TIMER, __callmgr_telephony_disable_fm_timer_cb, telephony_handle);
	} else {
		telephony_handle->cb_fn(CM_TELEPHONY_EVENT_NETWORK_ERROR, (void*)CM_TEL_CALL_ERR_FM_OFF_FAIL, telephony_handle->user_data);
		ret = _callmgr_telephony_get_call_by_call_id(telephony_handle, NO_CALL_HANDLE, &call);
		if ((ret == 0) && (call != NULL)) {
			warn("Since flight mode disable failed, hence delete the call data with invalid handle");
			_callmgr_telephony_call_delete(telephony_handle, NO_CALL_HANDLE);
		}
	}
}

int _callmgr_telephony_disable_flight_mode(callmgr_telephony_t telephony_handle)
{
	TapiResult_t tapi_err = TAPI_API_SUCCESS;
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);

	tapi_err = tel_set_flight_mode(telephony_handle->multi_handles[CM_TELEPHONY_SIM_1], TAPI_POWER_FLIGHT_MODE_LEAVE, __callmgr_telephony_set_flight_mode, telephony_handle);
	if (tapi_err != TAPI_API_SUCCESS) {
		err("disable flight mode err");
		return -1;
	}

	dbg("Request SUCCESS");

	g_disable_fm_cnt = 0;
	if (telephony_handle->disable_fm_timer > 0) {
		g_source_remove(telephony_handle->disable_fm_timer);
		telephony_handle->disable_fm_timer = -1;
	}

	return 0;
}

static void __callmgr_telephony_set_volume_resp_cb(TapiHandle *handle, int result, void *tapi_data, void *user_data)
{
	dbg("Result : %d", result);
}

int _callmgr_telephony_set_modem_volume(callmgr_telephony_t telephony_handle, cm_telephony_audio_path_type_e path_type, int volume)
{
	TapiResult_t tapi_err = TAPI_API_SUCCESS;
	TelCallVolumeInfo_t vol_info = {0, };
	int call_count = 0;

	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	if (volume > TAPI_SOUND_VOLUME_LEVEL_9) {
		err("invalid level");
		return -1;
	}

	call_count = g_slist_length(telephony_handle->modem_info[telephony_handle->active_sim_slot].call_list);
	if (call_count <= 0) {
		warn("No valid call");
		return -1;
	}

	vol_info.volume = volume;
	vol_info.type = TAPI_SOUND_TYPE_VOICE;
	switch (path_type) {
	case CM_TAPI_SOUND_PATH_HEADSET:
		vol_info.device = TAPI_SOUND_DEVICE_HEADSET;
		break;
	case CM_TAPI_SOUND_PATH_BLUETOOTH:
	case CM_TAPI_SOUND_PATH_STEREO_BLUETOOTH:
	case CM_TAPI_SOUND_PATH_BT_NSEC_OFF:
		vol_info.device = TAPI_SOUND_DEVICE_BLUETOOTH;
		break;
	case CM_TAPI_SOUND_PATH_SPK_PHONE:
		vol_info.device = TAPI_SOUND_DEVICE_SPEAKER_PHONE;
		break;
	default:
		vol_info.device = TAPI_SOUND_DEVICE_RECEIVER;
		break;
	}

	tapi_err = tel_set_call_volume_info(telephony_handle->multi_handles[telephony_handle->active_sim_slot], &vol_info, __callmgr_telephony_set_volume_resp_cb, NULL);

	if (tapi_err != TAPI_API_SUCCESS) {
		err("Tapi API Error: %d", tapi_err);
		return -1;
	}

	return 0;
}

int _callmgr_telephony_is_number_valid(const char *number, gboolean *b_number_valid)
{
	int len = 0;
	int i = 0;
	CM_RETURN_VAL_IF_FAIL(number, -1);
	CM_RETURN_VAL_IF_FAIL(b_number_valid, -1);

	len = strlen(number);
	if (len == 1) {
		err("It is wrong number.(1 digit number)");
		*b_number_valid = FALSE;
		return 0;
	}

	for (i = 0; i < len; i++) {
		/*'+' should be allowed only as first digit of the dialling number */
		if (!IS_DIGIT(number[i]) && number[i] != '+' && number[i] != '*' && number[i] != '#' && number[i] != 'P' && number[i] != 'p' && number[i] != ',') {
			*b_number_valid = FALSE;
		}
	}

	*b_number_valid = TRUE;
	return 0;
}

int _callmgr_telephony_is_ss_string(callmgr_telephony_t telephony_handle, int slot_id, const char *number, gboolean *o_is_ss)
{
	int len;
	CM_RETURN_VAL_IF_FAIL(number, -1);
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(slot_id < telephony_handle->modem_cnt, -1);

	len = strlen(number);

	if (len > 3) {
		if (number[len - 1] == '#') {
			if (number[0] == '*' || number[0] == '#') {
				*o_is_ss = TRUE;
				return 0;
			}
		} else {
			/*
			 * '*31#', '#31#' -> launch CISS
			 * '*31#nn..', '#31#nn...' -> launch Voice-call
			 */
			if (strncmp(number, CALLMGR_CLI_SHOW_ID, 4) == 0 || strncmp(number, CALLMGR_CLI_HIDE_ID, 4) == 0) {
				if (len > 4) {
					*o_is_ss = FALSE;
					return 0;
				}
				dbg("USSD");
				*o_is_ss = TRUE;
				return 0;
			}
		}
	}

	if ((len == 2)) {
		if ((number[0] == '0') && (IS1CHARUSSDDIGIT(number[1]))) {
			if (number[1] == '8') {
				/* 08 is emergency number */
				*o_is_ss = FALSE;
				return 0;
			} else {
				dbg("USSD string");
				*o_is_ss = TRUE;
				return 0;
			}
		} else if ((ISUSSDDIGIT(number[0]) && IS_DIGIT(number[1]))) {
			unsigned long mcc = 0;
			unsigned long mnc = 0;
			__callmgr_telephony_get_network_mcc_mnc(telephony_handle, slot_id, &mcc, &mnc);

			if ((mcc == CALL_NETWORK_MCC_CROATIA || mcc == CALL_NETWORK_MCC_SERBIA)
					&& (number[0] == '9' && ISECCDIGIT(number[1]))) {
				dbg("Emergency or Service number");
				*o_is_ss = FALSE;
				return 0;
			} else {
				dbg("USSD string");
				*o_is_ss = TRUE;
				return 0;
			}
		}
	}

	if ((len == 1) && (IS1CHARUSSDDIGIT(number[0])) && (number[0] != '0') && (number[0] != '1')) {
		dbg("1 character USSD string except '0' and '1'");
		*o_is_ss = TRUE;
		return 0;
	}

	if (number[len - 1] == '#') {
		dbg("USSD string");
		*o_is_ss = TRUE;
		return 0;
	}

	*o_is_ss = FALSE;
	return 0;
}


int _callmgr_telephony_is_answer_requesting(callmgr_telephony_t telephony_handle, gboolean *o_is_requesting)
{
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);

	if (telephony_handle->ans_requesting) {
		warn("Answer is requesting");
		*o_is_requesting = TRUE;
		return 0;
	}
	*o_is_requesting = FALSE;
	return 0;
}

int _callmgr_telephony_get_answer_request_type(callmgr_telephony_t telephony_handle, cm_telephony_answer_request_type_e *o_is_end_active_after_held_end)
{
	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);

	*o_is_end_active_after_held_end = telephony_handle->end_active_after_held_end;
	return 0;
}

int __callmgr_telephony_get_end_cause_type(TelCallCause_t call_cause, TelTapiEndCause_t cause, cm_telephony_end_cause_type_e *end_cause_type)
{
	dbg("call_cause [%d] end case [%d]", call_cause, cause);
	*end_cause_type = CM_TELEPHONY_ENDCAUSE_CALL_ENDED;
	if (TAPI_CAUSE_FIXED_DIALING_NUMBER_ONLY == call_cause) {
		/* This case is dial failed because of FDN */
		*end_cause_type = CM_TELEPHONY_ENDCAUSE_FIXED_DIALING_NUMBER_ONLY;
	} else if (TAPI_CAUSE_REJ_SAT_CALL_CTRL == call_cause) {
		*end_cause_type = CM_TELEPHONY_ENDCAUSE_REJ_SAT_CALL_CTRL;
	} else if (TAPI_CAUSE_NO_SERVICE_HERE == call_cause) {
		/* CS is not available case */
		*end_cause_type = CM_TELEPHONY_ENDCAUSE_CALL_NOT_ALLOWED;
	} else {
		switch (cause) {
			case TAPI_CC_CAUSE_NORMAL_UNSPECIFIED:
				*end_cause_type = CM_TELEPHONY_ENDCAUSE_CALL_DISCONNECTED;
				break;

			case TAPI_CC_CAUSE_FACILITY_REJECTED:
				*end_cause_type = CM_TELEPHONY_ENDCAUSE_CALL_NOT_ALLOWED;
				break;

			case TAPI_CC_CAUSE_QUALITY_OF_SERVICE_UNAVAILABLE:
			case TAPI_CC_CAUSE_ACCESS_INFORMATION_DISCARDED:
			case TAPI_CC_CAUSE_BEARER_CAPABILITY_NOT_AUTHORISED:
			case TAPI_CC_CAUSE_BEARER_CAPABILITY_NOT_PRESENTLY_AVAILABLE:
			case TAPI_CC_CAUSE_SERVICE_OR_OPTION_NOT_AVAILABLE:
			case TAPI_CC_CAUSE_BEARER_SERVICE_NOT_IMPLEMENTED:
			case TAPI_CC_CAUSE_PROTOCOL_ERROR_UNSPECIFIED:
				*end_cause_type = CM_TELEPHONY_ENDCAUSE_CALL_ENDED;
				break;

			case TAPI_CC_CAUSE_REQUESTED_FACILITY_NOT_SUBSCRIBED:
				*end_cause_type = CM_TELEPHONY_ENDCAUSE_CALL_SERVICE_NOT_ALLOWED;
				break;

			case TAPI_CC_CAUSE_OPERATOR_DETERMINED_BARRING:
				*end_cause_type = CM_TELEPHONY_ENDCAUSE_CALL_BARRED;
				break;
			case TAPI_REJECT_CAUSE_MM_REJ_NO_SERVICE:
				*end_cause_type = CM_TELEPHONY_ENDCAUSE_NO_SERVICE;
				break;

			case TAPI_REJECT_CAUSE_CONGESTTION:
			case TAPI_REJECT_CAUSE_CNM_REJ_NO_RESOURCES:
			case TAPI_CC_CAUSE_SWITCHING_EQUIPMENT_CONGESTION:	/* Match as NW_BUSY*/
				*end_cause_type = CM_TELEPHONY_ENDCAUSE_NW_BUSY;
				break;

			case TAPI_REJECT_CAUSE_NETWORK_FAILURE:
			case TAPI_REJECT_CAUSE_MSC_TEMPORARILY_NOT_REACHABLE:
				*end_cause_type = CM_TELEPHONY_ENDCAUSE_NW_FAILED;
				break;

			case TAPI_REJECT_CAUSE_IMEI_NOT_ACCEPTED:
				*end_cause_type = CM_TELEPHONY_ENDCAUSE_NW_FAILED;	/*Display Network unavailable*/
				break;

			case TAPI_CC_CAUSE_NO_ROUTE_TO_DEST:
			case TAPI_CC_CAUSE_TEMPORARY_FAILURE:
			case TAPI_CC_CAUSE_NETWORK_OUT_OF_ORDER:
			case TAPI_CC_CAUSE_REQUESTED_CIRCUIT_CHANNEL_NOT_AVAILABLE:
			case TAPI_CC_CAUSE_NO_CIRCUIT_CHANNEL_AVAILABLE:
			case TAPI_CC_CAUSE_DESTINATION_OUT_OF_ORDER:
				*end_cause_type = CM_TELEPHONY_ENDCAUSE_SERVICE_TEMP_UNAVAILABLE;
				break;
			case TAPI_CC_CAUSE_NO_USER_RESPONDING:
			case TAPI_CC_CAUSE_USER_ALERTING_NO_ANSWER:
				*end_cause_type = CM_TELEPHONY_ENDCAUSE_USER_DOESNOT_RESPOND;
				break;

			case TAPI_CC_CAUSE_ACM_GEQ_ACMMAX:
				*end_cause_type = CM_TELEPHONY_ENDCAUSE_NO_CREDIT;
				break;

			case TAPI_CC_CAUSE_CALL_REJECTED:
				*end_cause_type = CM_TELEPHONY_ENDCAUSE_USER_UNAVAILABLE;
				break;

			case TAPI_CC_CAUSE_USER_BUSY:
				*end_cause_type = CM_TELEPHONY_ENDCAUSE_USER_BUSY;
				break;

			case TAPI_CC_CAUSE_USER_NOT_MEMBER_OF_CUG:
				*end_cause_type = CM_TELEPHONY_ENDCAUSE_WRONG_GROUP;
				break;

			case TAPI_CC_CAUSE_INVALID_NUMBER_FORMAT:
				*end_cause_type = CM_TELEPHONY_ENDCAUSE_INVALID_NUMBER_FORMAT;
				break;

			case TAPI_CC_CAUSE_UNASSIGNED_NUMBER:
				*end_cause_type = CM_TELEPHONY_ENDCAUSE_UNASSIGNED_NUMBER;
				break;

			case TAPI_CC_CAUSE_NUMBER_CHANGED:
				*end_cause_type = CM_TELEPHONY_ENDCAUSE_NUMBER_CHANGED;
				break;

			case TAPI_CALL_END_NO_CAUSE:
			default:
				*end_cause_type = CM_TELEPHONY_ENDCAUSE_CALL_ENDED;

				dbg("Call Ended or Default Cause Value: %d", *end_cause_type);
				break;
			}
	}

	return 0;
}


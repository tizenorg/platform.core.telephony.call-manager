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

#include <vconf.h>
#include <bluetooth.h>
#include <bluetooth_internal.h>
#include <bluetooth_extension.h>
#include "callmgr-bt.h"
#include "callmgr-util.h"
#include "callmgr-log.h"

struct __bt_data {
	bt_call_list_h call_list;

	bt_event_cb cb_fn;
	void *user_data;
};

static void __callmgr_bt_ag_audio_connection_state_changed_cb(int result,
					bool connected, const char *remote_address,
					bt_audio_profile_type_e type, void *user_data)
{
	callmgr_bt_handle_h bt_handle = (callmgr_bt_handle_h)user_data;
	CM_RETURN_IF_FAIL(bt_handle);

	info("type : %d, connected : %d", type, connected);
	if (type == BT_AUDIO_PROFILE_TYPE_HSP_HFP) {
		/* Handle HFP connection */
		bt_handle->cb_fn(CM_BT_EVENT_CONNECTION_CHANGED_E, (void *)connected, bt_handle->user_data);
	}

}

static void __callmgr_bt_ag_call_handling_event_cb(bt_ag_call_handling_event_e event,
					unsigned int call_id, void *user_data)
{
	callmgr_bt_handle_h bt_handle = (callmgr_bt_handle_h)user_data;
	cm_bt_event_type_e bt_event = CM_BT_EVENT_CALL_HANDLE_ACCEPT_E;
	CM_RETURN_IF_FAIL(bt_handle);

	info("event : %d , id : %d", event, call_id);
	switch (event) {
		case BT_AG_CALL_HANDLING_EVENT_ANSWER:
			bt_event = CM_BT_EVENT_CALL_HANDLE_ACCEPT_E;
			break;
		case BT_AG_CALL_HANDLING_EVENT_RELEASE:
			bt_event = CM_BT_EVENT_CALL_HANDLE_RELEASE_E;
			break;
		case BT_AG_CALL_HANDLING_EVENT_REJECT:
			bt_event = CM_BT_EVENT_CALL_HANDLE_REJECT_E;
			break;
		default:
			warn("Invalid event");
			return;
		break;
	}

	bt_handle->cb_fn(bt_event, GUINT_TO_POINTER(call_id), bt_handle->user_data);
}

static void __callmgr_bt_ag_multi_call_handling_event_cb(bt_ag_multi_call_handling_event_e event, void *user_data)
{
	callmgr_bt_handle_h bt_handle = (callmgr_bt_handle_h)user_data;
	cm_bt_event_type_e bt_event = CM_BT_EVENT_CALL_HANDLE_0_SEND_E;
	CM_RETURN_IF_FAIL(bt_handle);

	info("event : %d", event);
	switch (event) {
		case BT_AG_MULTI_CALL_HANDLING_EVENT_RELEASE_HELD_CALLS:
			bt_event = CM_BT_EVENT_CALL_HANDLE_0_SEND_E;
			break;
		case BT_AG_MULTI_CALL_HANDLING_EVENT_RELEASE_ACTIVE_CALLS:
			bt_event = CM_BT_EVENT_CALL_HANDLE_1_SEND_E;
			break;
		case BT_AG_MULTI_CALL_HANDLING_EVENT_ACTIVATE_HELD_CALL:
			bt_event = CM_BT_EVENT_CALL_HANDLE_2_SEND_E;
			break;
		case BT_AG_MULTI_CALL_HANDLING_EVENT_MERGE_CALLS:
			bt_event = CM_BT_EVENT_CALL_HANDLE_3_SEND_E;
			break;
		case BT_AG_MULTI_CALL_HANDLING_EVENT_EXPLICIT_CALL_TRANSFER:
			bt_event = CM_BT_EVENT_CALL_HANDLE_4_SEND_E;
			break;
		default:
			warn("Invalid event");
			return;
		break;
	}

	bt_handle->cb_fn(bt_event, NULL, bt_handle->user_data);
}

static void __callmgr_bt_ag_dtmf_transmitted_cb(const char *dtmf, void *user_data)
{
	callmgr_bt_handle_h bt_handle = (callmgr_bt_handle_h)user_data;
	CM_RETURN_IF_FAIL(bt_handle);

	dbg("dtmf : %s", dtmf);
	bt_handle->cb_fn(CM_BT_EVENT_DTMF_TRANSMITTED_E, (void *)dtmf, bt_handle->user_data);
}

static void __callmgr_bt_ag_spk_gain_changed_cb(int gain, void *user_data)
{
	callmgr_bt_handle_h bt_handle = (callmgr_bt_handle_h)user_data;
	CM_RETURN_IF_FAIL(bt_handle);

	dbg("gain : %d", gain);
	bt_handle->cb_fn(CM_BT_EVENT_SPK_GAIN_CHANGED_E, GINT_TO_POINTER(gain), bt_handle->user_data);
}

static void __callmgr_bt_register_handler(callmgr_bt_handle_h bt_handle)
{
	int ret = 0;

	ret = bt_audio_initialize();
	if (BT_ERROR_NONE != ret) {
		warn("bt_audio_initialize - Failed");
	}

	ret = bt_audio_set_connection_state_changed_cb(__callmgr_bt_ag_audio_connection_state_changed_cb, bt_handle);
	if (BT_ERROR_NONE != ret) {
		warn("bt_audio_set_connection_state_changed_cb / error[%d]", ret);
	}

	ret = bt_ag_set_call_handling_event_cb(__callmgr_bt_ag_call_handling_event_cb, bt_handle);
	if (BT_ERROR_NONE != ret) {
		warn("bt_ag_set_call_handling_event_cb / error[%d]", ret);
	}

	ret = bt_ag_set_multi_call_handling_event_cb(__callmgr_bt_ag_multi_call_handling_event_cb, bt_handle);
	if (BT_ERROR_NONE != ret) {
		warn("bt_ag_set_multi_call_handling_event_cb / error[%d]", ret);
	}

	ret = bt_ag_set_dtmf_transmitted_cb(__callmgr_bt_ag_dtmf_transmitted_cb, bt_handle);
	if (BT_ERROR_NONE != ret) {
		warn("bt_ag_set_dtmf_transmitted_cb / error[%d]", ret);
	}

	ret = bt_ag_set_speaker_gain_changed_cb(__callmgr_bt_ag_spk_gain_changed_cb, bt_handle);
	if (BT_ERROR_NONE != ret) {
		warn("bt_ag_set_speaker_gain_changed_cb / error[%d]", ret);
	}
}

int _callmgr_bt_init(callmgr_bt_handle_h *o_bt_handle, bt_event_cb cb_fn, void *user_data)
{
	/* Todo: Set and get current BT status */
	struct __bt_data *handle = NULL;

	if (BT_ERROR_NONE != bt_initialize()) {
		err("bt_audio_initialize failed");
		return -1;
	}

	handle = calloc(1, sizeof(struct __bt_data));
	CM_RETURN_VAL_IF_FAIL(handle, -1);

	handle->cb_fn = cb_fn;
	handle->user_data = user_data;
	*o_bt_handle = handle;

	__callmgr_bt_register_handler(handle);

	dbg("BT init done");

	return 0;
}

static void __callmgr_bt_unregister_handler(void)
{
	int ret = 0;

	ret = bt_audio_unset_connection_state_changed_cb();
	if (BT_ERROR_NONE != ret) {
		warn("bt_audio_unset_connection_state_changed_cb / error[%d]", ret);
	}

	ret = bt_ag_unset_call_handling_event_cb();
	if (BT_ERROR_NONE != ret) {
		warn("bt_ag_unset_call_handling_event_cb / error[%d]", ret);
	}

	ret = bt_ag_unset_multi_call_handling_event_cb();
	if (BT_ERROR_NONE != ret) {
		warn("bt_ag_unset_multi_call_handling_event_cb / error[%d]", ret);
	}

	ret = bt_ag_unset_dtmf_transmitted_cb();
	if (BT_ERROR_NONE != ret) {
		warn("bt_ag_unset_dtmf_transmitted_cb / error[%d]", ret);
	}

	ret = bt_ag_unset_speaker_gain_changed_cb();
	if (BT_ERROR_NONE != ret) {
		warn("bt_ag_unset_speaker_gain_changed_cb / error[%d]", ret);
	}

	ret = bt_audio_deinitialize();
	if (BT_ERROR_NONE != ret) {
		warn("bt_audio_deinitialize / error[%d]", ret);
	}
}

int _callmgr_bt_deinit(callmgr_bt_handle_h bt_handle)
{
	CM_RETURN_VAL_IF_FAIL(bt_handle, -1);

	/* TODO: Release all handles */

	__callmgr_bt_unregister_handler();

	if (BT_ERROR_NONE != bt_deinitialize()) {
		err("bt_deinitialize failed");
			return -1;
	}

	g_free(bt_handle);

	return 0;
}

int _callmgr_bt_send_event(callmgr_bt_handle_h bt_handle, cm_bt_call_event_type_e call_event, int call_id, char *phone_num)
{
	int call_event_type = 0;
	int ret = 0;

	CM_RETURN_VAL_IF_FAIL(bt_handle, -1);

	switch (call_event) {
		case CM_BT_AG_RES_CALL_IDLE_E:
			call_event_type = BT_AG_CALL_EVENT_IDLE;
			break;
		case CM_BT_AG_RES_CALL_ACTIVE_E:
			call_event_type = BT_AG_CALL_EVENT_ANSWERED;
			break;
		case CM_BT_AG_RES_CALL_HELD_E:
			call_event_type = BT_AG_CALL_EVENT_HELD;
			break;
		case CM_BT_AG_RES_CALL_DIALING_E:
			call_event_type = BT_AG_CALL_EVENT_DIALING;
			break;
		case CM_BT_AG_RES_CALL_ALERT_E:
			call_event_type = BT_AG_CALL_EVENT_ALERTING;
			break;
		case CM_BT_AG_RES_CALL_INCOM_E:
			call_event_type = BT_AG_CALL_EVENT_INCOMING;
			break;
		default:
			warn("Invalid type : %d", call_event);
			return -1;
			break;
	}

	info("event : %d, id : %d", call_event_type, call_id);

	/* Can not send NULL number */
	if (phone_num) {
		ret = bt_ag_notify_call_event(call_event_type, call_id, phone_num);
	} else {
		ret = bt_ag_notify_call_event(call_event_type, call_id, "");
	}

	if (ret != BT_ERROR_NONE) {
		err("BT err : %d", ret);
		return -1;
	}

	return 0;
}

int _callmgr_bt_add_call_list(callmgr_bt_handle_h bt_handle, int call_id, int call_status, char *phone_num)
{
	int ret = 0;

	CM_RETURN_VAL_IF_FAIL(bt_handle, -1);

	if (bt_handle->call_list == NULL) {
		ret = bt_call_list_create(&(bt_handle->call_list));
		if (ret != BT_ERROR_NONE) {
			err("bt_call_list_create fail (%d)", ret);
			return -1;
		}
	}

	info("id : %d, state : %d", call_id, call_status);

	/* Can not send NULL number */
	if (phone_num) {
		bt_call_list_add(bt_handle->call_list, call_id, call_status, phone_num);
	} else {
		bt_call_list_add(bt_handle->call_list, call_id, call_status, "");
	}

	return 0;
}

int _callmgr_bt_send_call_list(callmgr_bt_handle_h bt_handle)
{
	dbg("..");
	CM_RETURN_VAL_IF_FAIL(bt_handle, -1);
	CM_RETURN_VAL_IF_FAIL(bt_handle->call_list, -1);

	bt_ag_notify_call_list(bt_handle->call_list);
	bt_call_list_destroy(bt_handle->call_list);
	bt_handle->call_list = NULL;

	return 0;
}

int _callmgr_bt_is_nrec_enabled(callmgr_bt_handle_h bt_handle, gboolean *o_is_nrec)
{
	CM_RETURN_VAL_IF_FAIL(bt_handle, -1);

	bool b_state = false;
	if (bt_ag_is_nrec_enabled(&b_state) != BT_ERROR_NONE) {
		err("bt_ag_is_nrec_enabled fail");
	}

	dbg("is nrec : %d", b_state);
	*o_is_nrec = (gboolean)b_state;
	return 0;
}

int _callmgr_bt_set_spk_gain(callmgr_bt_handle_h bt_handle, int volume)
{
	CM_RETURN_VAL_IF_FAIL(bt_handle, -1);

	if (bt_ag_notify_speaker_gain(volume) != BT_ERROR_NONE) {
		err("bt_ag_notify_speaker_gain fail");
	}

	return 0;
}

int _callmgr_bt_get_spk_gain(callmgr_bt_handle_h bt_handle, int *o_volume)
{
	CM_RETURN_VAL_IF_FAIL(bt_handle, -1);

	int bt_vol = 0;
	if (bt_ag_get_speaker_gain(&bt_vol) != BT_ERROR_NONE) {
		err("bt_ag_get_speaker_gain fail");
	}
	dbg("bt vol : %d", bt_vol);
	*o_volume = bt_vol;

	return 0;
}

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

#ifndef __CALLMGR_BT_H__
#define __CALLMGR_BT_H__

#include <glib.h>

// TODO:
//Add more events
typedef enum {
	CM_BT_EVENT_CONNECTION_CHANGED_E,
	CM_BT_EVENT_DTMF_TRANSMITTED_E,
	CM_BT_EVENT_SPK_GAIN_CHANGED_E,

	CM_BT_EVENT_CALL_HANDLE_ACCEPT_E,
	CM_BT_EVENT_CALL_HANDLE_RELEASE_E,
	CM_BT_EVENT_CALL_HANDLE_REJECT_E,

	CM_BT_EVENT_CALL_HANDLE_0_SEND_E,	/* Release held call */
	CM_BT_EVENT_CALL_HANDLE_1_SEND_E, /* Release active call */
	CM_BT_EVENT_CALL_HANDLE_2_SEND_E, /* Swap */
	CM_BT_EVENT_CALL_HANDLE_3_SEND_E, /* Join */
	CM_BT_EVENT_CALL_HANDLE_4_SEND_E, /* ECT */

} cm_bt_event_type_e;

typedef enum {
	CM_BT_AG_RES_CALL_IDLE_E,			/**< MO or MT call ended */
	CM_BT_AG_RES_CALL_ACTIVE_E,	    /**< Call connected */
	CM_BT_AG_RES_CALL_HELD_E,			/**< Call held */
	CM_BT_AG_RES_CALL_DIALING_E,	    /**< Phone originated a call.  */
	CM_BT_AG_RES_CALL_ALERT_E,		/**< Remote party call alerted */
	CM_BT_AG_RES_CALL_INCOM_E,	    /**< Incoming call notification to Headset */
} cm_bt_call_event_type_e;
/* Please check cm_telephony_call_state_e before modify it */

typedef struct __bt_data *callmgr_bt_handle_h;
typedef void (*bt_event_cb) (cm_bt_event_type_e event_type, void *event_data, void *user_data);

int _callmgr_bt_init(callmgr_bt_handle_h *o_bt_handle, bt_event_cb cb_fn, void *user_data);
int _callmgr_bt_deinit(callmgr_bt_handle_h bt_handle);

int _callmgr_bt_send_event(callmgr_bt_handle_h bt_handle, cm_bt_call_event_type_e call_event, int call_id, char *phone_num);
int _callmgr_bt_add_call_list(callmgr_bt_handle_h bt_handle, int call_id, int call_status, char *phone_num);
int _callmgr_bt_send_call_list(callmgr_bt_handle_h bt_handle);
int _callmgr_bt_open_sco(callmgr_bt_handle_h bt_handle);
int _callmgr_bt_close_sco(callmgr_bt_handle_h bt_handle);
int _callmgr_bt_is_nrec_enabled(callmgr_bt_handle_h bt_handle, gboolean *o_is_nrec);
int _callmgr_bt_set_spk_gain(callmgr_bt_handle_h bt_handle, int volume);
int _callmgr_bt_get_spk_gain(callmgr_bt_handle_h bt_handle, int *o_volume);

#endif	//__CALLMGR_BT_H__


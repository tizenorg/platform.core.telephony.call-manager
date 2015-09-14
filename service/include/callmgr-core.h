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

#ifndef __CALL_MANAGER_H__
#define __CALL_MANAGER_H__


#include <glib.h>
#include <gio/gio.h>
#include "callmgr-telephony.h"
#include "callmgr-audio.h"
#include "callmgr-ringer.h"
#include "callmgr-contact.h"
#include "callmgr-bt.h"
#include "callmgr-sensor.h"
#include "callmgr-vr.h"
#include "callmgr-answer-msg.h"

/*This enum is inline with cm_call_status_e, so update both enums together*/
typedef enum _call_status{
	CALL_MANAGER_CALL_STATUS_IDLE_E = 0,
	CALL_MANAGER_CALL_STATUS_RINGING_E,
	CALL_MANAGER_CALL_STATUS_OFFHOOK_E,
	CALL_MANAGER_CALL_STATUS_MAX_E
} callmgr_call_status_e;

/*This enum is inline with cm_call_event_e, so update both enums together*/
typedef enum _call_event{
	CALL_MANAGER_CALL_EVENT_IDLE_E = 0,
	CALL_MANAGER_CALL_EVENT_DIALING_E,
	CALL_MANAGER_CALL_EVENT_ACTIVE_E,
	CALL_MANAGER_CALL_EVENT_HELD_E,
	CALL_MANAGER_CALL_EVENT_ALERT_E,
	CALL_MANAGER_CALL_EVENT_INCOMING_E,
	CALL_MANAGER_CALL_EVENT_WAITING_E,
	CALL_MANAGER_CALL_EVENT_JOIN_E,
	CALL_MANAGER_CALL_EVENT_SPLIT_E,
	CALL_MANAGER_CALL_EVENT_SWAP_E,
	CALL_MANAGER_CALL_EVENT_RETRIEVED_E,
	CALL_MANAGER_CALL_EVENT_CALL_CONTROL_E,
} callmgr_call_event_e;

/* This enum is inline with cm_util_call_type_e. please change both together */
typedef enum _call_type {
	CALL_TYPE_VOICE_E = 0,
	CALL_TYPE_VIDEO_E,
	CALL_TYPE_INVALID_E,
} callmgr_call_type_e;

typedef enum {
	CALL_MANAGER_ERROR_NONE_E,
	CALL_MANAGER_ERROR_INVALID_PARAMS_E,
	CALL_MANAGER_ERROR_UNKNOWN_E,
} callmgr_error_type_e;

typedef enum {
	CALL_ERR_CAUSE_UNKNOWN_E,
	CALL_ERR_CAUSE_EMERGENCY_ONLY_E,
	CALL_ERR_CAUSE_NETWORK_2G_ONLY_E,
	CALL_ERR_CAUSE_IS_NOT_EMERGENCY_NUMBER_E,
	CALL_ERR_CAUSE_NOT_ALLOWED_E,
	CALL_ERR_CAUSE_WRONG_NUMBER_E,
	CALL_ERR_CAUSE_FLIGHT_MODE_E,
	CALL_ERR_CAUSE_RESTRICTED_BY_FDN_E,
	CALL_ERR_CAUSE_PHONE_NOT_INITIALIZED_E,
	CALL_ERR_CAUSE_NOT_REGISTERED_ON_NETWORK_E,
	CALL_ERR_CAUSE_VOICE_SERVICE_BLOCKED_E,
} callmgr_call_error_cause_e;

typedef enum {
	CALL_DTMF_INDI_IDLE_E = 0,
	CALL_DTMF_INDI_PROGRESSING_E,
	CALL_DTMF_INDI_WAIT_E,
} callmgr_dtmf_indi_type_e;

typedef enum  {
	CALL_AUDIO_PATH_NONE_E,
	CALL_AUDIO_PATH_SPEAKER_E,
	CALL_AUDIO_PATH_RECEIVER_E,
	CALL_AUDIO_PATH_EARJACK_E,
	CALL_AUDIO_PATH_BT_E,
} callmgr_path_type_e;

typedef enum {
	CALL_DTMF_RESP_CANCEL_E = 0,
	CALL_DTMF_RESP_CONTINUE_E,
} callmgr_dtmf_resp_type_e;

typedef enum {
	CALL_MANAGER_SIM_SLOT_1_E = 0,
	CALL_MANAGER_SIM_SLOT_2_E,
	CALL_MANAGER_SIM_SLOT_DEFAULT_E,
} callmgr_sim_slot_type_e;

typedef enum {
	CALL_MANAGER_DIAL_SUCCESS = 0,
	CALL_MANAGER_DIAL_CANCEL,
	CALL_MANAGER_DIAL_FAIL,
	CALL_MANAGER_DIAL_FAIL_SS,
	CALL_MANAGER_DIAL_FAIL_FDN,
	CALL_MANAGER_DIAL_FAIL_FLIGHT_MODE,
} callmgr_dial_status_e;

typedef enum {
	CALL_SIGNAL_NONE_E,
	CALL_SIGNAL_USER_BUSY_TONE_E,
	CALL_SIGNAL_WRONG_NUMBER_TONE_E,
	CALL_SIGNAL_CALL_FAIL_TONE_E,
	CALL_SIGNAL_NW_CONGESTION_TONE_E,
}callmgr_play_signal_type_e;

typedef enum {
	CALL_NAME_MODE_NONE,			/**<  None */
	CALL_NAME_MODE_UNKNOWN,			/**<  Unknown*/
	CALL_NAME_MODE_PRIVATE,		/**<  Private*/
	CALL_NAME_MODE_PAYPHONE,			/**<  Payphone*/
	CALL_NAME_MODE_MAX,
} callmgr_name_mode_e;

typedef struct __cm_core_call_data {
	unsigned int call_id;							/**< Unique call id*/
	int call_direction;						/**< 0 : MO, 1 : MT*/
	char *call_number;							/**< call number*/
	char *calling_name;							/**< calling_name*/

	callmgr_call_type_e call_type;			/**< Specifies type of call (0:VOICE, 1:VIDEO, 2:VSHARE_RX, 3:VSHARE_TX) */
	int call_domain;			/**< Specifies domain of call (0:CS, 1:PS, 2:HFP, 3:SAP) */
	int call_state;		/**< Current Call state */

	long start_time;	/**< start time of the Call*/

	gboolean is_conference;						/**< Whether Call is in Conference or not*/
	gboolean is_ecc;						/**< Whether Call is ecc or not*/
	gboolean is_voicemail_number;					/**<Whether is a voicemail number*/

	callmgr_name_mode_e name_mode;
}callmgr_call_data_t;

typedef struct __cm_watch_client_data {
	guint watch_id;
	gchar *appid;
	gboolean is_appeared;
}callmgr_watch_client_data_t;


typedef struct _callmgr_core_data {
	GDBusObjectManagerServer *gdoms;
	GDBusConnection *dbus_conn;
	GDBusInterfaceSkeleton *dbus_skeleton_interface;
	GSList *watch_list;

	callmgr_telephony_t telephony_handle;
	callmgr_audio_handle_h audio_handle;
	callmgr_ringer_handle_h ringer_handle;
	callmgr_bt_handle_h bt_handle;
	callmgr_sensor_handle_h sensor_handle;
	callmgr_vr_handle_h vr_handle;
	callmgr_answer_msg_h answer_msg_handle;

	callmgr_call_data_t *incom;
	callmgr_call_data_t *active_dial;
	callmgr_call_data_t *held;

	callmgr_call_status_e call_status;
	gint dtmf_pause_timer;
	gint auto_answer_timer;
	gboolean is_auto_answered;

	gboolean is_mute_on;
} callmgr_core_data_t;

int _callmgr_core_init(callmgr_core_data_t **o_core_data);
int _callmgr_core_deinit(callmgr_core_data_t *core_data);
int _callmgr_core_process_dial(callmgr_core_data_t *core_data, const char *number, callmgr_call_type_e call_type, callmgr_sim_slot_type_e sim_slot,
		int disable_fm, gboolean is_emergency_contact, gboolean is_sat_originated_call);
int _callmgr_core_process_end_call(callmgr_core_data_t *core_data, unsigned int call_id, int release_type);
int _callmgr_core_process_reject_call(callmgr_core_data_t *core_data);
int _callmgr_core_process_swap_call(callmgr_core_data_t *core_data);
int _callmgr_core_process_hold_call(callmgr_core_data_t *core_data);
int _callmgr_core_process_unhold_call(callmgr_core_data_t *core_data);
int _callmgr_core_process_join_call(callmgr_core_data_t *core_data);
int _callmgr_core_process_split_call(callmgr_core_data_t *core_data, unsigned int call_id);
int _callmgr_core_process_transfer_call(callmgr_core_data_t *core_data);
int _callmgr_core_process_answer_call(callmgr_core_data_t *core_data, int ans_type);
int _callmgr_core_get_audio_state(callmgr_core_data_t *core_data, callmgr_path_type_e *o_audio_state);

/* Audio API*/
int _callmgr_core_process_spk_on(callmgr_core_data_t *core_data);
int _callmgr_core_process_spk_off(callmgr_core_data_t *core_data);
int _callmgr_core_process_bluetooth_on(callmgr_core_data_t *core_data);
int _callmgr_core_process_bluetooth_off(callmgr_core_data_t *core_data);
int _callmgr_core_process_record_start(callmgr_core_data_t *core_data, const char *call_num, gboolean is_answering_message);
int _callmgr_core_process_record_stop(callmgr_core_data_t *core_data);
int _callmgr_core_process_set_extra_vol(callmgr_core_data_t *core_data, gboolean is_extra_vol);
int _callmgr_core_process_set_noise_reduction(callmgr_core_data_t *core_data, gboolean is_noise_reduction);
int _callmgr_core_process_set_mute_state(callmgr_core_data_t *core_data, gboolean is_mute_state);

int _callmgr_core_process_dtmf_resp(callmgr_core_data_t *core_data, callmgr_dtmf_resp_type_e resp_type);
int _callmgr_core_process_stop_alert(callmgr_core_data_t *core_data);
int _callmgr_core_process_start_alert(callmgr_core_data_t *core_data);

/*End of cm_call_data get api's*/

#endif	//__CALL_MANAGER_H__

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

#ifndef __CALLMGR_UTIL_H__
#define __CALLMGR_UTIL_H__

#include <glib.h>
#include <gio/gio.h>

#include <cynara-client.h>

#define AC_CALL_MANAGER	"callmgr::api_call"

struct custom_data {
	cynara *p_cynara;
};

typedef enum {
	CALL_POPUP_FLIGHT_MODE = 0,

	CALL_POPUP_FLIGHT_MODE_DISABLING = 1,
	CALL_POPUP_SIM_SELECT = 2,
	CALL_POPUP_SS_INFO = 3,
	CALL_POPUP_CALL_ERR = 4,
	CALL_POPUP_TOAST = 5,
	CALL_POPUP_OUT_OF_3G_TRY_VOICE_CALL = 6,

	CALL_POPUP_REC_STATUS = 11,

	CALL_POPUP_HIDE = 100,
} call_popup_type;

typedef enum {
	CALL_ERR_FAILED,
	CALL_ERR_NOT_ALLOWED,
	CALL_ERR_EMERGENCY_ONLY,
	CALL_ERR_IS_NOT_EMERGENCY,
	CALL_ERR_WRONG_NUMBER,
	CALL_ERR_FDN_CALL_ONLY,
	CALL_ERR_OUT_OF_SERVICE,
	CALL_ERR_FAILED_TO_ADD_CALL,
	CALL_ERR_NOT_REGISTERED_ON_NETWORK,
	CALL_ERR_PHONE_NOT_INITIALIZED,
	CALL_ERR_TRANSFER_CALL_FAILED,
	CALL_ERR_LOW_BATTERY_INCOMING,
	CALL_ERR_JOIN_CALL_FAILED,
	CALL_ERR_HOLD_CALL_FAILED,
	CALL_ERR_SWAP_CALL_FAILED,
	CALL_ERR_RETRIEVE_CALL_FAILED,
	CALL_ERR_ANSWER_CALL_FAILED,
	CALL_ERR_NO_SIM,
	CALL_ERR_NONE,
} call_err_type;

typedef enum {
	CALL_POPUP_TOAST_SWAP_CALL,
	CALL_POPUP_TOAST_CUSTOM,
	CALL_POPUP_TOAST_MAX,
} call_popup_toast_type_e;

typedef enum {
	CM_UTIL_POPUP_RESP_CANCEL,
	CM_UTIL_POPUP_RESP_OK,
} cm_util_popup_result_e;

/* This enum is inline with callmgr_call_type_e. please change both together */
typedef enum cm_util_call_type {
	CM_UTIL_CALL_TYPE_VOICE_E = 0,
	CM_UTIL_CALL_TYPE_VIDEO_E,
	CM_UTIL_CALL_TYPE_INVALID_E,
} cm_util_call_type_e;

/* This enum is inline with callmgr_popup_rec_status_sub_info_e. please change both together */
typedef enum {
	CM_UTIL_REC_STATUS_NONE_E = 0,
	CM_UTIL_REC_STATUS_STOP_BY_NORMAL_E,
	CM_UTIL_REC_STATUS_STOP_BY_MEMORY_FULL_E,
	CM_UTIL_REC_STATUS_STOP_BY_TIME_SHORT,
	CM_UTIL_REC_STATUS_STOP_BY_NO_ENOUGH_MEMORY,
} cm_util_rec_status_sub_info_e;

typedef void (*callmgr_util_popup_result_cb) (call_popup_type popup_type, void *result, void *user_data);

int _callmgr_util_is_ringtone_playable(char *ringtone_path, gboolean *is_playable);
int _callmgr_util_is_silent_ringtone(char *ringtone_path, gboolean *is_silent_ringtone);
int _callmgr_util_is_callui_running(gboolean *is_callui_running);
int _callmgr_util_launch_callui(gint call_id, int sim_slot, int call_type);
int _callmgr_util_launch_callui_by_sat(int sim_slot);
int _callmgr_util_launch_popup(call_popup_type popup_type, int info, const char* number, int active_sim, callmgr_util_popup_result_cb cb, void *user_data);
int _callmgr_util_get_time_diff(long start_time, int *call_duration_in_sec);
int _callmgr_util_check_block_mode_num(const char *str_num, gboolean *is_blocked);

/**
 * This function removes invalid characters from given call number.
 *
 * @Return		Returns 0 on success and -1 on failure
 * @Param[in]	src		original call number
 * @Param[out]	dst		Pointer to the buffer to store the number removed invalid characters
 * @Remark		must be free out param after use
 */
int _callmgr_util_remove_invalid_chars_from_call_number(const char *src, char **dst);

/**
 * This function extracts call number and dtmf number from the number which is given by input param.
 * And returned call number will not include cli code even if orig_num has it.
 *
 * @Return		Returns 0 on success and -1 on failure
 * @Param[in]	orig_num	original number
 * @Param[out]	call_num	Pointer to the buffer to store the call number.
 * @Param[out]	dtmf_num	Pointer to the buffer to store the dtmf number.
 * @Remark		must be free out params after use
 */
int _callmgr_util_extract_call_number(const char *orig_num, char **call_num, char **dtmf_num);

/**
 * This function extracts call number and dtmf number from the number which is given by input param.
 * And returned call number will include cli code if orig_num has it.
 *
 * @Return		Returns 0 on success and -1 on failure
 * @Param[in]	orig_num	original number
 * @Param[out]	call_num	Pointer to the buffer to store the call number.
 * @Param[out]	dtmf_num	Pointer to the buffer to store the dtmf number.
 * @Remark		must be free out params after use
 */
int _callmgr_util_extract_call_number_without_cli(const char *orig_num, char **call_num, char **dtmf_num);

int _callmgr_util_is_incall_ss_string(const char *number, gboolean *o_is_incall_ss);

int _callmgr_util_launch_ciss(const char* number, int sim_slot);

int _callmgr_util_is_recording_progress(gboolean *is_recording);
int _callmgr_util_is_video_recording_progress(gboolean *is_recording);

int _callmgr_util_check_disturbing_setting(gboolean *is_do_not_disturb);

gboolean _callmgr_util_check_access_control(cynara *p_cynara, GDBusMethodInvocation *invoc, const char *label, const char *perm);

int _callmgr_util_service_check_voice_mail(char *phonenumber, int sim_slot, gboolean *is_voicemail_number);

#endif //__CALLMGR_UTIL_H__


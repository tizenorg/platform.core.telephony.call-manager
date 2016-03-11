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

#ifndef _CALLMGR_COMMON_H_
#define _CALLMGR_COMMON_H_

#include <Elementary.h>
#include <glib.h>
#include <app.h>
#include <vconf.h>
#include <gio/gio.h>
#include <tzplatform_config.h>

#include "callmgr-popup-debug.h"

#ifndef CALLMRG_POPUP_MODULE_EXPORT
#define CALLMRG_POPUP_MODULE_EXPORT __attribute__ ((visibility("default")))
#endif

#define PATH_EDJ tzplatform_mkpath(TZ_SYS_RO_APP, "org.tizen.callmgr-popup/res/edje/popup_custom.edj")

#define CALL_DOMAIN "call"
#define SYS_DOMAIN "sys_string"

#define PACKAGE "callmgr-popup"

#define DEF_BUF_LEN_LONG (256)

typedef enum {
	CALLMGR_POPUP_FLIGHT_MODE_E = 0,

	CALLMGR_POPUP_FLIGHT_MODE_DISABLING_E = 1,
	CALLMGR_POPUP_SIM_SELECT_E = 2,
	CALLMGR_POPUP_SS_INFO_E = 3,
	CALLMGR_POPUP_CALL_ERR_E = 4,
	CALLMGR_POPUP_TOAST_E = 5,
	CALLMGR_POPUP_OUT_OF_3G_TRY_VOICE_CALL = 6,

	CALLMGR_POPUP_REC_STATUS_E = 11,

	CALLMGR_POPUP_HIDE_E = 100,
} callmgr_popup_type_e;

typedef enum {
	SS_INFO_MO_CODE_UNCONDITIONAL_CF_ACTIVE_E,
	SS_INFO_MO_CODE_SOME_CF_ACTIVE_E,
	SS_INFO_MO_CODE_CALL_FORWARDED_E,
	SS_INFO_MO_CODE_CALL_IS_WAITING_E,
	SS_INFO_MO_CODE_CUG_CALL_E,
	SS_INFO_MO_CODE_OUTGOING_CALLS_BARRED_E,
	SS_INFO_MO_CODE_INCOMING_CALLS_BARRED_E,
	SS_INFO_MO_CODE_CLIR_SUPPRESSION_REJECTED_E,
	SS_INFO_MO_CODE_CALL_DEFLECTED_E,

	SS_INFO_MT_CODE_FORWARDED_CALL_E,
	SS_INFO_MT_CODE_CUG_CALL_E,
	SS_INFO_MT_CODE_CALL_ON_HOLD_E,
	SS_INFO_MT_CODE_CALL_RETRIEVED_E,
	SS_INFO_MT_CODE_MULTI_PARTY_CALL_E,
	SS_INFO_MT_CODE_ON_HOLD_CALL_RELEASED_E,
	SS_INFO_MT_CODE_FORWARD_CHECK_RECEIVED_E,
	SS_INFO_MT_CODE_CALL_CONNECTING_ECT_E,
	SS_INFO_MT_CODE_CALL_CONNECTED_ECT_E,
	SS_INFO_MT_CODE_DEFLECTED_CALL_E,
	SS_INFO_MT_CODE_ADDITIONAL_CALL_FORWARDED_E,
} callmgr_ss_info_type_e;

typedef enum {
	CALLMGR_POPUP_TOAST_SWAP_CALL,
	CALLMGR_POPUP_TOAST_CUSTOM,
	CALLMGR_POPUP_TOAST_MAX,
} callmgr_call_popup_toast_type_e;

/* This enum is inline with cm_util_rec_status_sub_info_e. please change both together */
typedef enum {
	CALLMGR_POPUP_REC_STATUS_NONE_E = 0,
	CALLMGR_POPUP_REC_STATUS_STOP_BY_NORMAL_E,
	CALLMGR_POPUP_REC_STATUS_STOP_BY_MEMORY_FULL_E,
	CALLMGR_POPUP_REC_STATUS_STOP_BY_TIME_SHORT_E,
	CALLMGR_POPUP_REC_STATUS_STOP_BY_NO_ENOUGH_MEMORY_E,
} callmgr_popup_rec_status_sub_info_e;

typedef enum {
	ERR_CALL_FAILED_E,
	ERR_CALL_NOT_ALLOWED_E,
	ERR_EMERGENCY_ONLY_E,
	ERR_IS_NOT_EMERGENCY_E,
	ERR_WRONG_NUMBER_E,
	ERR_FDN_CALL_ONLY_E,
	ERR_OUT_OF_SERVICE_E,
	ERR_FAILED_TO_ADD_CALL_E,
	ERR_NOT_REGISTERED_ON_NETWORK_E,
	ERR_PHONE_NOT_INITIALIZED_E,
	ERR_TRANSFER_CALL_FAIL_E,
	ERR_LOW_BATTERY_INCOMING_E,
	ERR_JOIN_CALL_FAILED_E,
	ERR_HOLD_CALL_FAILED_E,
	ERR_SWAP_CALL_FAILED_E,
	ERR_RETREIVE_CALL_FAILED_E,
	ERR_ANSWER_CALL_FAILED_E,
	ERR_NO_SIM_E,
} callmgr_call_err_type_e;

typedef struct appdata {
	Evas_Object *win_main;
	Evas_Object *popup;

	app_control_h request;
	char *dial_num;
	callmgr_popup_type_e popup_type;
	int call_type;
	int active_sim;

	Ecore_Event_Handler *downkey_handler;
	Ecore_Event_Handler *upkey_handler;

	GDBusConnection *dbus_conn;
	GCancellable *ca;
	GHashTable *evt_list;

	Ecore_Idler *exit_idler;
} CallMgrPopAppData_t;

CALLMRG_POPUP_MODULE_EXPORT int main(int argc, char *argv[]);

#endif	/* _CALLMGR_COMMON_H_ */


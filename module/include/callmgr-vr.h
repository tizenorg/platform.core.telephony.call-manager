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

#ifndef __CALLMGR_VR_H__
#define __CALLMGR_VR_H__

#include <glib.h>

typedef enum {
	CALLMGR_VR_NONE, /**< Recorder is IDLE */
	CALLMGR_VR_INITIALIZED, /**< Recorder created and ready to record */
	CALLMGR_VR_RECORD_REQUESTED, /**< Record Process Requested */
	CALLMGR_VR_RECORDING, /**< Recorder currently recording */
	CALLMGR_VR_PAUSED /**< Recorder is currently paused */
} callmgr_vr_state_e;

typedef enum {
	CALLMGR_VR_STATUS_EVENT_STARTED,	/**< Record is started */
	CALLMGR_VR_STATUS_EVENT_STOPPED,	/**< Record is stopped */
	//Above Events must be sent to UI


	//Add more events
} callmgr_vr_status_event_e;


/**
 * This enum defines call recording types & the command type sent by recorder to the client view through update signal
 */
typedef enum {
	CALLMGR_VR_STATUS_EXTRA_START_TYPE = 0x00,
	CALLMGR_VR_STATUS_EXTRA_START_TYPE_NORMAL,		/**< Normal recording*/
	CALLMGR_VR_STATUS_EXTRA_START_TYPE_ANSWER_MSG,		/**< Answering message*/
	CALLMGR_VR_STATUS_EXTRA_START_TYPE_MAX = 0x0f,

	CALLMGR_VR_STATUS_EXTRA_STOP_TYPE = 0x10,
	CALLMGR_VR_STATUS_EXTRA_STOP_TYPE_BY_NORMAL,						/**< BY_NORMAL*/
	CALLMGR_VR_STATUS_EXTRA_STOP_TYPE_BY_MAX_SIZE,					/**< by MAX_SIZE*/
	CALLMGR_VR_STATUS_EXTRA_STOP_TYPE_BY_NO_FREE_SPACE,		/**< BY_NO_FREE_SPACE*/
	CALLMGR_VR_STATUS_EXTRA_STOP_TYPE_BY_TIME_LIMIT,				/**< BY_TIME_LIMIT*/
	CALLMGR_VR_STATUS_EXTRA_STOP_TYPE_BY_TIME_SHORT,
	CALLMGR_VR_STATUS_EXTRA_STOP_TYPE_ERROR,								/**< ERROR*/
	CALLMGR_VR_STATUS_EXTRA_STOP_TYPE_MAX = 0x1f,
} callmgr_vr_status_extra_type_e;


typedef enum {
	CALLMGR_VR_STORAGE_PHONE,	/**< Phone storage */
	CALLMGR_VR_STORAGE_MMC,	/**< MMC storage */
} callmgr_vr_store_type_e;

typedef struct {
	long capacity;			/**< The capacity of system memory */
	long used;			/**< The used space of system memory */
} callmgr_vr_fex_memory_status_t;

typedef struct __vr_data *callmgr_vr_handle_h;
typedef void (*vr_event_cb) (callmgr_vr_status_event_e status, callmgr_vr_status_extra_type_e extra_data, void *user_data);

int _callmgr_vr_init(callmgr_vr_handle_h *vr_handle, vr_event_cb cb_fn, void *user_data);
int _callmgr_vr_deinit(callmgr_vr_handle_h vr_handle);

int _callmgr_vr_start_record(callmgr_vr_handle_h vr_handle, const char* call_num, const char* call_name, gboolean is_answering_machine);
int _callmgr_vr_stop_record(callmgr_vr_handle_h vr_handle);
int _callmgr_vr_get_recording_state(callmgr_vr_handle_h vr_handle, callmgr_vr_state_e *state);

#endif	//__CALLMGR_VR_H__
